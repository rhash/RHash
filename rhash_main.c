/*
 * rhash_main.c: compute CRC32, MD5, SHA1, Tiger, DC++ TTH and eDonkey 2000 hashes
 *
 * rhash is a small utility written in C that computes various message
 * digests of files. The message digests include CRC32, MD5, SHA1, TTH,
 * ED2K, GOST and many other.
 */

#include "common_func.h" /* should be included before the C library files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */
#include <unistd.h> /* S_ISDIR under VC6 */
#include <sys/stat.h> /* stat(), S_ISDIR */
#include <signal.h>
#include <locale.h>

#include "librhash/timing.h"
#include "win_utils.h"
#include "find_file.h"
#include "calc_sums.h"
#include "hash_update.h"
#include "file_mask.h"
#include "hash_print.h"
#include "parse_cmdline.h"
#include "output.h"
#include "version.h"

#include "rhash_main.h"

struct rhash_t rhash_data;

/**
 * Callback function to process a file found while recursively traversing
 * a directory.
 *
 * @param filepath path to the file
 * @param type file attributes
 * @param data the paramater specified, when calling find_file(). It shall be
 *             non-zero for printing SFV header, zero for actual processing.
 */
static int find_file_callback(const char* filepath, int type, void* data)
{
	int res = 0;
	if( !(type & FIND_IFDIR) ) {
		if(data) {
			if(!file_mask_match(opt.files_accept, filepath)) return 0;
			if(filepath[0] == '.' && IS_PATH_SEPARATOR(filepath[1])) filepath += 2;
			/* TODO: get size, fill file struct */
			print_sfv_header_line(rhash_data.out, filepath, filepath);
			/* rhash_data.total_size += file.size */
		} else {
			/* only check an update modes use crc_accept mask */
			file_mask_array* masks = (opt.mode & (MODE_CHECK | MODE_UPDATE) ?
				opt.crc_accept : opt.files_accept);
			if(!file_mask_match(masks, filepath)) return 0;

			if(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
				res = check_hash_file(filepath, 1);
			} else {
				if(opt.mode & MODE_UPDATE) {
					res = update_hash_file(filepath);
				} else {
					/* default mode: calculate hash */
					if(filepath[0] == '.' && IS_PATH_SEPARATOR(filepath[1])) filepath += 2;
					res = calculate_and_print_sums(rhash_data.out, filepath, filepath, NULL);
					rhash_data.processed++;
				}
			}
		}
	}
	if(res < 0) rhash_data.error_flag = 1;
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
	fflush(rhash_data.out);
	fprintf(rhash_data.log, _("Interrupted by user...\n"));
	fflush(rhash_data.log);

	/* print intermediate check results if process was interrupted */
	if((opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) && rhash_data.processed > 0) {
		print_check_stats();
		fflush(rhash_data.out);
	}

	/* restore precious signal handler and call it */
	if(prev_sigint_handler && prev_sigint_handler != SIG_ERR) {
		signal(signum, prev_sigint_handler);
		prev_sigint_handler(signum);
	}
	rsh_exit(1); /* exit the program */
}

#define MAX_TEMPLATE_SIZE 65536

/**
 * Load printf-template from file, specified by options or config.
 */
static int load_printf_template(void)
{
	FILE* fd = fopen(opt.template_file, "rb");
	char buffer[8192];
	size_t len;
	int error = 0;

	if(!fd) {
		log_file_error(opt.template_file);
		return 0;
	}

	rhash_data.template_text = rsh_str_new();

	while(!feof(fd)) {
		len = fread(buffer, 1, 8192, fd);
		/* read can return -1 on error */
		if(len == (size_t)-1) break;
		rsh_str_append_n(rhash_data.template_text, buffer, len);
		if(rhash_data.template_text->len >= MAX_TEMPLATE_SIZE) {
			log_msg(_("%s: template file is too big\n"), opt.template_file);
			error = 1;
		}
	}

	if(ferror(fd)) {
		log_file_error(opt.template_file);
		error = 1;
	}

	fclose(fd);
	rhash_data.printf_str = rhash_data.template_text->str;
	return !error;
}

/**
 * Output SFV-formated file header.
 */
static void preprocess_files(void)
{
	int i;

	/* print SFV file header */
	if(opt.fmt == FMT_SFV && opt.mode == 0) {
		print_sfv_banner(rhash_data.out);

		for(i = 0; i < opt.n_files; i++) {
			char *path = opt.files[i];
			if(opt.flags & OPT_RECURSIVE) {
				struct rsh_stat_struct stat_buf;
				if(rsh_stat(path, &stat_buf) < 0) {
					continue; /* don't report error here, it'll be reported later */
				}
				/* if file is a directory, then walk it recursively */
				if(S_ISDIR(stat_buf.st_mode)) {
					find_file(path, find_file_callback, 0, opt.find_max_depth, (void*)1);
					continue;
				}
			}
			print_sfv_header_line(rhash_data.out, path, path);
		}
		fflush(rhash_data.out);
	}
}

/**
 * Hash, check or update files according to work mode.
 */
static void process_files(void)
{
	timedelta_t timer;
	struct rsh_stat_struct stat_buf;
	int i;
	rhash_timer_start(&timer);
	rhash_data.processed = 0;

	/* process filenames */
	for(i = 0; i < opt.n_files; i++) {
		int res = 0;
		char* filepath = opt.files[i];
		stat_buf.st_mode = 0;

		if(!IS_DASH_STR(filepath) && rsh_stat(filepath, &stat_buf) < 0) {
			log_file_error(filepath);
			continue;
		}
		if(opt.flags & OPT_RECURSIVE) {
			if(S_ISDIR(stat_buf.st_mode)) {
				find_file(filepath, find_file_callback, 0, opt.find_max_depth, NULL);
				continue;
			}
		} else {
			if(S_ISDIR(stat_buf.st_mode)){
				if(opt.flags & OPT_VERBOSE){
					log_warning(_("%s: is a directory\n"), filepath);
				}
				continue;
			}
		}

		if(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			res = check_hash_file(filepath, 0);
		} else if(opt.mode & MODE_UPDATE) {
			res = update_hash_file(filepath);
		} else {
			res = calculate_and_print_sums(rhash_data.out, filepath, filepath, &stat_buf);
			rhash_data.processed++;
		}
		if(res < 0) rhash_data.error_flag = 1;
	}

	if((opt.mode & MODE_CHECK_EMBEDDED) && rhash_data.processed > 1) {
		print_check_stats();
	}

	if((opt.flags & OPT_SPEED) && !(opt.mode&(MODE_CHECK | MODE_UPDATE)) &&
		rhash_data.processed > 1) {
			double time = rhash_timer_stop(&timer);
			print_time_stats(time, rhash_data.total_size, 1);
	}
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
	IF_WINDOWS(restore_console());
}

static void i18n_initialize(void)
{
	setlocale(LC_ALL, ""); /* set locale according to the environment */

#ifdef USE_GETTEXT
	bindtextdomain("rhash", LOCALEDIR); /* set the text message domain */
	textdomain("rhash");
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
	i18n_initialize(); /* initialize locale and translation */

	memset(&rhash_data, 0, sizeof(rhash_data));
	rhash_data.out = stdout; /* set initial output streams */
	rhash_data.log = stderr; /* can be altered by options later */

	init_hash_info_table();

	read_options(argc, argv); /* load config and parse command line options */
	prev_sigint_handler = signal(SIGINT, ctrl_c_handler); /* install SIGINT handler */
	rhash_library_init();

	/* in benchmark mode just run benchmark and exit */
	if(opt.mode & MODE_BENCHMARK) {
		unsigned flags = (opt.flags & OPT_BENCH_RAW ? RHASH_BENCHMARK_CPB | RHASH_BENCHMARK_RAW : RHASH_BENCHMARK_CPB);
		if((opt.flags & OPT_BENCH_RAW) == 0) {
			fprintf(rhash_data.out, _("%s v%s benchmarking...\n"), PROGRAM_NAME, VERSION);
		}
		rhash_run_benchmark(opt.sum_flags, flags, rhash_data.out);
		rsh_exit(0);
	}

	if(opt.n_files == 0) {
		if(argc > 1) {
			log_warning(_("no files/directories were specified at command line\n"));
		}

		/* print short usage help */
		log_msg(_("Usage: %s [OPTION...] <FILE>...\n\n"
			"Run `%s --help' for more help.\n"), CMD_FILENAME, CMD_FILENAME);
		rsh_exit(0);
	}

	/* setup printf formating string */
	rhash_data.printf_str = opt.printf_str;

	if(opt.template_file) {
		if(!load_printf_template()) rsh_exit(2);
	} else if(!rhash_data.printf_str && !(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED))) {
		/* initialize printf output format according to '--<hashname>' options */
		init_printf_format( (rhash_data.template_text = rsh_str_new()) );
		rhash_data.printf_str = rhash_data.template_text->str;

		if(opt.flags & OPT_VERBOSE) {
			char* str = rsh_strdup(rhash_data.printf_str);
			log_msg(_("Format string is: %s\n"), str_trim(str));
			free(str);
		}
	}

	if(rhash_data.printf_str) {
		rhash_data.print_list = parse_print_string(rhash_data.printf_str, &opt.sum_flags);
	}

	preprocess_files();
	process_files();

	options_destroy(&opt);
	rhash_destroy(&rhash_data);
	return (rhash_data.error_flag ? 1 : 0);
}
