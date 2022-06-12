/* rhash_main.c - main() and other top-level functions
 *
 * rhash is a small utility written in C that computes various message
 * digests of files. The message digests include CRC32, MD5, SHA1, TTH,
 * ED2K, GOST and many other.
 */

#include "rhash_main.h"
#include "calc_sums.h"
#include "file_mask.h"
#include "find_file.h"
#include "hash_print.h"
#include "hash_update.h"
#include "output.h"
#include "parse_cmdline.h"
#include "win_utils.h"
#include "librhash/rhash.h"
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h> /* free() */
#include <string.h>


struct rhash_t rhash_data;

/**
 * Check if the file must be skipped. Returns 1 if the file path
 * is the same as the output or the log file path.
 *
 * @param file the file to check
 * @param mask the mask of accepted files
 * @return 1 if the file should be skipped, 0 otherwise
 */
static int must_skip_file(file_t* file)
{
	/* check if the file path is the same as the output or the log file path */
	return (opt.output && are_paths_equal(file->real_path, &rhash_data.out_file)) ||
		(opt.log && are_paths_equal(file->real_path, &rhash_data.log_file)) ||
		(opt.update_file && are_paths_equal(file->real_path, &rhash_data.upd_file));
}

/**
 * Callback function to process files while recursively traversing a directory.
 * It hashes, checks or updates a file according to the current work mode.
 *
 * @param file the file to process
 * @param preprocess non-zero when preprocessing files, zero for actual processing
 */
static int scan_files_callback(file_t* file, int preprocess)
{
	int res = 0;
	assert(!FILE_ISDIR(file));
	assert(opt.search_data);

	if (rhash_data.stop_flags) {
		opt.search_data->options |= FIND_CANCEL;
		return 0;
	}
	errno = 0; /* start processing each file with clear errno */

	if (preprocess) {
		if (FILE_ISDATA(file) ||
				!file_mask_match(opt.files_accept, file) ||
				(opt.files_exclude && file_mask_match(opt.files_exclude, file)) ||
				must_skip_file(file))
			return 0;

		if (rhash_data.is_sfv && print_sfv_header_line(rhash_data.out, rhash_data.out_file.mode, file) < 0) {
			log_error_file_t(&rhash_data.out_file);
			res = -2;
		}

		rhash_data.batch_size += file->size;
	} else {
		int not_root = !(file->mode & FileIsRoot);

		if (!FILE_ISSPECIAL(file)) {
			if (not_root) {
				if (opt.mode == MODE_CHECK) {
					/* use crc_accept list in the plain recursive check mode, but not in -uc mode */
					if (!file_mask_match(opt.crc_accept, file)) {
						return 0;
					}
				} else {
					if (!file_mask_match(opt.files_accept, file) ||
						(opt.files_exclude && file_mask_match(opt.files_exclude, file))) {
						return 0;
					}
				}
			}
			if (must_skip_file(file))
				return 0;
		} else if (FILE_ISDATA(file) && IS_MODE(MODE_CHECK | MODE_CHECK_EMBEDDED | MODE_UPDATE | MODE_TORRENT)) {
			log_warning(_("skipping: %s\n"), file_get_print_path(file, FPathUtf8 | FPathNotNull));
			return 0;
		}

		if (IS_MODE(MODE_UPDATE)) {
			res = update_ctx_update(rhash_data.update_context, file);
		} else if (IS_MODE(MODE_CHECK)) {
			res = check_hash_file(file, not_root);
		} else if (IS_MODE(MODE_CHECK_EMBEDDED)) {
			res = check_embedded_crc32(file);
		} else {
			/* default mode: calculate hash */
			res = calculate_and_print_sums(rhash_data.out, &rhash_data.out_file, file);
			if (rhash_data.stop_flags) {
				opt.search_data->options |= FIND_CANCEL;
				return 0;
			}
			rhash_data.processed++;
		}
	}
	if (res < -1) {
		rhash_data.stop_flags |= FatalErrorFlag;
		opt.search_data->options |= FIND_CANCEL;
		return 0;
	} else if (res < 0)
		rhash_data.non_fatal_error = 1;
	return 1;
}

/* previous SIGINT handler */
void (*prev_sigint_handler)(int) = NULL;

/**
 * Handler for the SIGINT signal, sent when user press Ctrl+C.
 * The handler prints message and exits the program.
 *
 * @param signum the processed signal identifier SIGINT
 */
static void ctrl_c_handler(int signum)
{
	(void)signum;
	rhash_data.stop_flags |= InterruptedFlag;
	if (rhash_data.rctx) {
		rhash_cancel(rhash_data.rctx);
	}
}

#define MAX_TEMPLATE_SIZE 65536

/**
 * Load printf-template from file, specified by options or config.
 */
static int load_printf_template(void)
{
	FILE* fd;
	file_t file;
	char buffer[8192];
	size_t len;
	int error = 0;

	file_init(&file, opt.template_file, FileInitReusePath);
	fd = file_fopen(&file, FOpenRead | FOpenBin);
	if (!fd)
	{
		log_error_file_t(&file);
		file_cleanup(&file);
		return 0;
	}

	rhash_data.template_text = rsh_str_new();

	while (!feof(fd)) {
		len = fread(buffer, 1, 8192, fd);
		if (ferror(fd)) break;

		rsh_str_append_n(rhash_data.template_text, buffer, len);
		if (rhash_data.template_text->len >= MAX_TEMPLATE_SIZE) {
			log_msg_file_t(_("%s: template file is too big\n"), &file);
			error = 1;
		}
	}

	if (ferror(fd)) {
		log_error_file_t(&file);
		error = 1;
	}

	fclose(fd);
	file_cleanup(&file);
	rhash_data.printf_str = rhash_data.template_text->str;
	return !error;
}

/**
 * Free data allocated by an rhash_t object
 *
 * @param ptr pointer to rhash_t object
 */
void rhash_destroy(struct rhash_t* ptr)
{
	free_print_list(ptr->print_list);
	rsh_str_free(ptr->template_text);
	if (ptr->update_context)
		update_ctx_free(ptr->update_context);
	if (ptr->rctx)
		rhash_free(ptr->rctx);
	if (ptr->out && !FILE_ISSTDSTREAM(&ptr->out_file))
		fclose(ptr->out);
	if (ptr->log && !FILE_ISSTDSTREAM(&ptr->log_file))
		fclose(ptr->log);
	file_cleanup(&ptr->out_file);
	file_cleanup(&ptr->log_file);
	file_cleanup(&ptr->upd_file);
	file_cleanup(&ptr->config_file);
}

static void free_allocated_data(void)
{
	options_destroy(&opt);
	rhash_destroy(&rhash_data);
}

static void i18n_initialize(void)
{
	setlocale(LC_ALL, ""); /* set locale according to the environment */

#ifdef USE_GETTEXT
	bindtextdomain(TEXT_DOMAIN, LOCALEDIR); /* set the text message domain */
	IF_WINDOWS(setup_locale_dir());
	textdomain(TEXT_DOMAIN);
#endif /* USE_GETTEXT */
}

/**
 * RHash program entry point.
 *
 * @param argc number of program arguments including the program path
 * @param argv program arguments
 * @return the program exit code, zero on success and 1 on error
 */
int main(int argc, char* argv[])
{
	timedelta_t timer;
	int exit_code;
	int need_sfv_banner;

	memset(&rhash_data, 0, sizeof(rhash_data));
	memset(&opt, 0, sizeof(opt));
	rhash_data.out = stdout; /* set initial output streams */
	rhash_data.log = stderr; /* can be altered by options later */
	rsh_install_exit_handler(free_allocated_data);

	IF_WINDOWS(init_program_dir());
	init_hash_info_table();

	i18n_initialize(); /* initialize locale and translation */

	read_options(argc, argv); /* load config and parse command line options */
	prev_sigint_handler = signal(SIGINT, ctrl_c_handler); /* install SIGINT handler */
	rhash_library_init();
	setup_percents();

	/* in benchmark mode just run benchmark and exit */
	if (IS_MODE(MODE_BENCHMARK)) {
		unsigned flags = (HAS_OPTION(OPT_BENCH_RAW) ? BENCHMARK_CPB | BENCHMARK_RAW : BENCHMARK_CPB);
		if (!HAS_OPTION(OPT_BENCH_RAW)) {
			rsh_fprintf(rhash_data.out, _("%s v%s benchmarking...\n"), PROGRAM_NAME, get_version_string());
		}
		run_benchmark(opt.sum_flags, flags);
		exit_code = (rhash_data.stop_flags ? 3 : 0);
		rsh_exit(exit_code);
	}

	if (!opt.has_files) {
		if (argc > 1) {
			log_warning(_("no files/directories were specified at command line\n"));
		}

		/* print short usage help */
		log_msg(_("Usage: %s [OPTION...] <FILE>...\n\n"
			"Run `%s --help' for more help.\n"), CMD_FILENAME, CMD_FILENAME);
		rsh_exit(0);
	}
	assert(opt.search_data != 0);

	/* setup printf formatting string */
	rhash_data.printf_str = opt.printf_str;
	/* print SFV header if CRC32 or no hash function has been selected */
	rhash_data.is_sfv = (opt.fmt == FMT_SFV ||
		(!opt.fmt && (opt.sum_flags == RHASH_CRC32 || !opt.sum_flags)));
	if (!opt.sum_flags && opt.mode != MODE_CHECK)
		opt.sum_flags = RHASH_CRC32;

	if (opt.template_file) {
		if (!load_printf_template()) rsh_exit(2);
	} else if (!rhash_data.printf_str && IS_MODE(MODE_DEFAULT | MODE_UPDATE | MODE_TORRENT)) {
		/* initialize printf output format according to '--<hashname>' options */
		rhash_data.template_text = init_printf_format();
		rhash_data.printf_str = rhash_data.template_text->str;

		if (HAS_OPTION(OPT_VERBOSE)) {
			char* str = rsh_strdup(rhash_data.printf_str);
			log_msg(_("Format string is: %s\n"), str_trim(str));
			free(str);
		}
	}

	if (rhash_data.printf_str) {
		rhash_data.print_list = parse_print_string(rhash_data.printf_str, &opt.sum_flags);
	}

	opt.search_data->options = FIND_SKIP_DIRS;
	opt.search_data->options |= (HAS_OPTION(OPT_FOLLOW) ? FIND_FOLLOW_SYMLINKS : 0);
	opt.search_data->callback = scan_files_callback;

	need_sfv_banner = (rhash_data.is_sfv && IS_MODE(MODE_DEFAULT));
	if (need_sfv_banner && print_sfv_banner(rhash_data.out) < 0) {
		log_error_file_t(&rhash_data.out_file);
		rhash_data.stop_flags |= FatalErrorFlag;
	}

	/* preprocess files */
	if (need_sfv_banner || opt.bt_batch_file) {
		/* note: errors are not reported on preprocessing */
		opt.search_data->callback_data = 1;
		scan_files(opt.search_data);

		if (fflush(rhash_data.out) < 0) {
			log_error_file_t(&rhash_data.out_file);
			rhash_data.stop_flags |= FatalErrorFlag;
		}
	}
	if ((rhash_data.stop_flags & FatalErrorFlag) != 0)
		rsh_exit(2);

	/* measure total processing time */
	rsh_timer_start(&timer);
	rhash_data.processed = 0;

	if (opt.update_file)
	{
		file_init(&rhash_data.upd_file, opt.update_file, FileInitReusePath);
		rhash_data.update_context = update_ctx_new(&rhash_data.upd_file);
		if (!rhash_data.update_context)
			rsh_exit(0);
	}

	/* process files */
	opt.search_data->options |= FIND_LOG_ERRORS;
	opt.search_data->callback_data = 0;
	scan_files(opt.search_data);

	if (IS_MODE(MODE_CHECK_EMBEDDED) && rhash_data.processed > 1) {
		if (print_check_stats() < 0) {
			log_error_file_t(&rhash_data.out_file);
			rhash_data.stop_flags |= FatalErrorFlag;
		}
	} else if (IS_MODE(MODE_UPDATE) && rhash_data.update_context) {
		/* finalize hash file and check for errors */
		if (update_ctx_free(rhash_data.update_context) < 0)
				rhash_data.stop_flags |= FatalErrorFlag;
		rhash_data.update_context = 0;
	}

	if (!rhash_data.stop_flags) {
		if (opt.bt_batch_file && rhash_data.rctx) {
			file_t batch_torrent_file;
			file_init(&batch_torrent_file, opt.bt_batch_file, FileInitReusePath);

			rhash_final(rhash_data.rctx, 0);
			if (save_torrent_to(&batch_torrent_file, rhash_data.rctx) < 0)
				rhash_data.stop_flags |= FatalErrorFlag;
			file_cleanup(&batch_torrent_file);
		}

		if (HAS_OPTION(OPT_SPEED) && opt.mode != MODE_CHECK && rhash_data.total_size != 0) {
			uint64_t time = rsh_timer_stop(&timer);
			print_time_stats(time, rhash_data.total_size, 1);
		}
	} else
		report_interrupted();

	exit_code = ((rhash_data.stop_flags & FatalErrorFlag) ? 2 :
		(rhash_data.non_fatal_error || opt.search_data->errors_count) ? 1 :
		(rhash_data.stop_flags & InterruptedFlag) ? 3 : 0);
	rsh_call_exit_handlers();
	return exit_code;
}
