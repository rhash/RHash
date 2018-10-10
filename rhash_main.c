/* rhash_main.c - main() and other top-level functions
 *
 * rhash is a small utility written in C that computes various message
 * digests of files. The message digests include CRC32, MD5, SHA1, TTH,
 * ED2K, GOST and many other.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */
#include <signal.h>
#include <locale.h>
#include <assert.h>

#include "rhash_main.h"
#include "calc_sums.h"
#include "common_func.h"
#include "file_mask.h"
#include "find_file.h"
#include "hash_print.h"
#include "hash_update.h"
#include "parse_cmdline.h"
#include "output.h"
#include "win_utils.h"
#include "librhash/rhash.h"


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
	const rsh_tchar* path = FILE_TPATH(file);

	/* check if the file path is the same as the output or the log file path */
	return (opt.output && are_paths_equal(path, opt.output)) ||
		(opt.log && are_paths_equal(path, opt.log));
}

/**
 * Callback function to process files while recursively traversing a directory.
 * It hashes, checks or updates a file according to the current work mode.
 *
 * @param file the file to process
 * @param preprocess non-zero when preprocessing files, zero for actual processing.
 */
static int find_file_callback(file_t* file, int preprocess)
{
	int res = 0;
	assert(!FILE_ISDIR(file));
	assert(opt.search_data);

	if (rhash_data.interrupted) {
		opt.search_data->options |= FIND_CANCEL;
		return 0;
	}

	if (preprocess) {
		if (!file_mask_match(opt.files_accept, file->path) ||
			(opt.files_exclude && file_mask_match(opt.files_exclude, file->path)) ||
			must_skip_file(file)) {
			return 0;
		}

		if (opt.fmt & FMT_SFV) {
			print_sfv_header_line(rhash_data.out, file, 0);
		}

		rhash_data.batch_size += file->size;
	} else {
		int not_root = !(file->mode & FILE_IFROOT);

		if (not_root) {
			if ((opt.mode & (MODE_CHECK | MODE_UPDATE)) != 0) {
				/* check and update modes use the crc_accept list */
				if (!file_mask_match(opt.crc_accept, file->path)) {
					return 0;
				}
			} else {
				if (!file_mask_match(opt.files_accept, file->path) ||
					(opt.files_exclude && file_mask_match(opt.files_exclude, file->path))) {
					return 0;
				}
			}
		}
		if (must_skip_file(file)) return 0;

		if (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			res = check_hash_file(file, not_root);
		} else {
			if (opt.mode & MODE_UPDATE) {
				res = update_hash_file(file);
			} else {
				/* default mode: calculate hash */
				const char* print_path = file->path;
				if (print_path[0] == '.' && IS_PATH_SEPARATOR(print_path[1])) print_path += 2;
				res = calculate_and_print_sums(rhash_data.out, file, print_path);
				if (rhash_data.interrupted) return 0;
				rhash_data.processed++;
			}
		}
	}
	if (res < 0) rhash_data.error_flag = 1;
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
	rhash_data.interrupted = 1;
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

	file_tinit(&file, opt.template_file, FILE_OPT_DONT_FREE_PATH);
	fd = file_fopen(&file, FOpenRead | FOpenBin);
	if (!fd)
	{
		log_file_t_error(&file);
		file_cleanup(&file);
		return 0;
	}

	rhash_data.template_text = rsh_str_new();

	while (!feof(fd)) {
		len = fread(buffer, 1, 8192, fd);
		if (ferror(fd)) break;

		rsh_str_append_n(rhash_data.template_text, buffer, len);
		if (rhash_data.template_text->len >= MAX_TEMPLATE_SIZE) {
			log_msg(_("%s: template file is too big\n"), file_cpath(&file));
			error = 1;
		}
	}

	if (ferror(fd)) {
		log_file_t_error(&file);
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
	if (ptr->rctx) rhash_free(ptr->rctx);
	if (ptr->out) fclose(ptr->out);
	if (ptr->log) fclose(ptr->log);
#ifdef _WIN32
	if (ptr->program_dir) free(ptr->program_dir);
#endif
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
int main(int argc, char *argv[])
{
	timedelta_t timer;
	int exit_code;
	int sfv;

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
	if (opt.mode & MODE_BENCHMARK) {
		unsigned flags = (opt.flags & OPT_BENCH_RAW ? BENCHMARK_CPB | BENCHMARK_RAW : BENCHMARK_CPB);
		if ((opt.flags & OPT_BENCH_RAW) == 0) {
			rsh_fprintf(rhash_data.out, _("%s v%s benchmarking...\n"), PROGRAM_NAME, get_version_string());
		}
		run_benchmark(opt.sum_flags, flags);
		exit_code = (rhash_data.interrupted ? 3 : 0);
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

	if (opt.template_file) {
		if (!load_printf_template()) rsh_exit(2);
	} else if (!rhash_data.printf_str && !(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED))) {
		/* initialize printf output format according to '--<hashname>' options */
		init_printf_format( (rhash_data.template_text = rsh_str_new()) );
		rhash_data.printf_str = rhash_data.template_text->str;

		if (opt.flags & OPT_VERBOSE) {
			char* str = rsh_strdup(rhash_data.printf_str);
			log_msg(_("Format string is: %s\n"), str_trim(str));
			free(str);
		}
	}

	if (rhash_data.printf_str) {
		rhash_data.print_list = parse_print_string(rhash_data.printf_str, &opt.sum_flags);
	}

	opt.search_data->options = FIND_SKIP_DIRS;
	opt.search_data->options |= (opt.flags & OPT_FOLLOW ? FIND_FOLLOW_SYMLINKS : 0);
	opt.search_data->call_back = find_file_callback;

	if ((sfv = (opt.fmt == FMT_SFV && !opt.mode))) {
		print_sfv_banner(rhash_data.out);
	}

	/* preprocess files */
	if (sfv || opt.bt_batch_file) {
		/* note: errors are not reported on preprocessing */
		opt.search_data->call_back_data = 1;
		scan_files(opt.search_data);

		fflush(rhash_data.out);
	}

	/* measure total processing time */
	rsh_timer_start(&timer);
	rhash_data.processed = 0;

	/* process files */
	opt.search_data->options |= FIND_LOG_ERRORS;
	opt.search_data->call_back_data = 0;
	scan_files(opt.search_data);

	if ((opt.mode & MODE_CHECK_EMBEDDED) && rhash_data.processed > 1) {
		print_check_stats();
	}

	if (!rhash_data.interrupted) {
		if (opt.bt_batch_file && rhash_data.rctx) {
			file_t batch_torrent_file;
			file_tinit(&batch_torrent_file, opt.bt_batch_file, FILE_OPT_DONT_FREE_PATH);

			rhash_final(rhash_data.rctx, 0);
			save_torrent_to(&batch_torrent_file, rhash_data.rctx);
		}

		if ((opt.flags & OPT_SPEED) &&
			!(opt.mode & (MODE_CHECK | MODE_UPDATE)) &&
			rhash_data.processed > 1)
		{
			double time = rsh_timer_stop(&timer);
			print_time_stats(time, rhash_data.total_size, 1);
		}
	} else {
		/* check if interruption was not reported yet */
		if (rhash_data.interrupted == 1) report_interrupted();
	}

	exit_code = (rhash_data.error_flag ? 1 :
		opt.search_data->errors_count ? 2 :
		rhash_data.interrupted ? 3 : 0);
	rsh_exit(exit_code);
	return 0; /* unreachable statement */
}
