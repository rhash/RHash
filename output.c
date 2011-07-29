/* output.c */

#include "common_func.h" /* should be included before the C library files */
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* exit() */
#include <assert.h>
#include <errno.h>

#include "librhash/rhash.h"
#include "calc_sums.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "output.h"

/*#ifdef _WIN32
#define WIN32_USE_CURSOR
#endif*/

#ifdef _WIN32
#include <windows.h>
#include <share.h> /* for _SH_DENYNO */
#endif

#ifdef WIN32_USE_CURSOR
#include <io.h>
#endif

/* global pointer to the selected method of percents output */
struct percents_output_info_t *percents_output = NULL;


/**
 * Print a formated message to program log, and flush the log stream.
 *
 * @param format print a formated message to the program log
 * @param param a printf format parameter (NULL, if not needed)
 */
void log_msg(const char* format, ...)
{
	FILE* log = (rhash_data.log ? rhash_data.log : stderr);
	va_list ap;
	va_start(ap, format);
	vfprintf(log, format, ap);
	fflush(log);
}

/**
 * Print a file error to program log.
 *
 * @param filepath the path to file caused the error
 */
void log_file_error(const char* filepath)
{
	fprintf(rhash_data.log, PROGRAM_NAME " error: %s: %s\n", filepath, strerror(errno));
	fflush(rhash_data.log);
}

/* a structure to store how much percents processed */
struct percents_t {
	int points;
	int use_cursor;
	int same_output;
	unsigned ticks;

#ifdef WIN32_USE_CURSOR
	HANDLE hOut;
	unsigned short cur_x, cur_y; /* cursor position, where to print percents */
#endif
};
static struct percents_t percents;

/**
 * Print file path and result of its verification by hash.
 * Also if error occurred, print error message.
 *
 * @param info pointer to the file-info structure
 * @param print_name set to non-zero to print file path
 * @param print_result set to non-zero to print hash verification result
 */
static void print_check_result(struct file_info *info, int print_name, int print_result)
{
	if(print_name) {
		fprintf(rhash_data.out, "%-51s ", info->print_path);
	}
	if(print_result) {
		if(info->error == -1) {
			/* print error to stdout */
			fprintf(rhash_data.out, "%s\n", strerror(errno));
		} else if(info->wrong_sums == 0 || !(opt.flags & OPT_VERBOSE)) {
			/* using 4 characters to overwrite percent */
			fprintf(rhash_data.out, (info->wrong_sums == 0 ? "OK \n" : "ERR\n") );
		} else {
			int id;
			char actual[130], expected[130];

			/* print verbose info about wrong sums */
			fprintf(rhash_data.out, "ERROR");
			for(id = 1; id < RHASH_ALL_HASHES; id <<= 1) {
				if(id & info->wrong_sums) {
					int pflags = (rhash_is_base32(id) ? RHPR_BASE32 | RHPR_UPPERCASE : RHPR_HEX | RHPR_UPPERCASE);
					rhash_print_bytes(expected, rhash_get_digest_ptr(info->orig_sums, id), rhash_get_digest_size(id), pflags);

					rhash_print(actual, info->rctx, id, RHPR_UPPERCASE);
					fprintf(rhash_data.out, ", %s is %s should be %s", rhash_get_name(id), actual, expected);
				}
			}
			if(RHASH_EMBEDDED_CRC32 & info->wrong_sums) {
				rhash_print(expected, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
				fprintf(rhash_data.out, ", embedded CRC32 should be %s", expected);
			}
			fprintf(rhash_data.out, "\n");
		}
	}
	fflush(rhash_data.out);
}

/**
 * Prepare or print result of file processing.
 *
 * @param info pointer to the file-info structure
 * @param init non-zero on initialization before hash calculation,
 *             and zero after hash calculation finished.
 */
static void print_results_on_check(struct file_info *info, int init)
{
	if(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
		int print_name = (opt.flags & (OPT_PERCENTS | OPT_SKIP_OK) ? !init : init);

		if(!init && (opt.flags & OPT_SKIP_OK) && errno == 0 && info->wrong_sums == 0) {
			return; /* skip OK message */
		}

		print_check_result(info, print_name, !init);
	}
}

/* functions to output file info without percents */

/**
 * Print file name in hash checking mode.
 * No information is printed in other modes.
 *
 * @param info pointer to the file-info structure
 * @return non-zero, indicating that the output method succesfully initialized
 */
static int dummy_init_percents(struct file_info *info)
{
	print_results_on_check(info, 1);
	return 1;
}

/**
 * Print file check results without printing percents.
 * Information is printed only in hash verification mode.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occured while hashing/checking
 */
static void dummy_finish_percents(struct file_info *info, int process_res)
{
	info->error = process_res;
	print_results_on_check(info, 0);
}

/* functions to output file info with simple multy-line wget-like percents */

/**
 * Initialize dots percent mode.
 *
 * @param info pointer to the file-info structure
 * @return non-zero, indicating that the output method succesfully initialized
 */
static int dots_init_percents(struct file_info *info)
{
	(void)info;
	fflush(rhash_data.out);
	fflush(rhash_data.log);
	percents.points = 0;
	print_results_on_check(info, 1);
	return 1;
}

/**
 * Finish dots percent mode. If in hash verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occured while hashing/checking
 */
static void dots_finish_percents(struct file_info *info, int process_res)
{
	char buf[80];
	info->error = process_res;

	if((percents.points % 74) != 0) {
		log_msg("%s 100%%\n", str_set(buf, ' ', 74 - (percents.points%74) ));
	}
	print_results_on_check(info, 0);
}

/**
 * Output percents by printing one dot per each processed 1MiB.
 *
 * @param info pointer to the file-info structure
 * @param offset current file offset in bytes
 */
static void dots_update_percents(struct file_info *info, uint64_t offset)
{
	const int pt_size = 1024*1024; /* 1MiB */
	if( (offset % pt_size) != 0 ) return;

	if(percents.points == 0) {
		fprintf(rhash_data.log, "\n%s %s\n",
			(opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED) ? "Checking" : "Processing"),
			info->print_path);
		fflush(rhash_data.log);
	}
	putc('.', rhash_data.log);

	if(((++percents.points) % 74) == 0) {
		if(info->size > 0) {
			int perc = (int)( offset * 100.0 / (uint64_t)info->size + 0.5 );
			fprintf(rhash_data.log, "  %2u%%\n", perc);
			fflush(rhash_data.log);
		} else {
			putc('\n', rhash_data.log);
		}
	}
}

/* console one-line percents */

/**
 * Initialize one-line percent mode.
 *
 * @param info pointer to the file-info structure
 * @return non-zero if the output method succesfully initialized
 */
static int p_init_percents(struct file_info *info)
{
#ifdef WIN32_USE_CURSOR
	CONSOLE_SCREEN_BUFFER_INFO csbInfo;
	percents.hOut = NULL;
#endif

	(void)info;
	percents.points      = 0;
	percents.same_output = 0;
	percents.use_cursor  = 0;

	fflush(rhash_data.out);
	fflush(rhash_data.log);
	assert(rhash_data.log == stderr);

	/* note: this output differs from print_check_result() by file handle */
	fprintf(rhash_data.log, "%-51s ", info->print_path);

#ifdef WIN32_USE_CURSOR
	if(percents.use_cursor) {
		/* store cursor coordinates */
		percents.hOut = GetStdHandle(STD_ERROR_HANDLE);
		if(percents.hOut == INVALID_HANDLE_VALUE ||
			!GetConsoleScreenBufferInfo(percents.hOut, &csbInfo)) {
				percents.hOut = NULL;
				return 0;
		} else {
			percents.cur_x = csbInfo.dwCursorPosition.X;
			percents.cur_y = csbInfo.dwCursorPosition.Y;
		}
	}
#endif

	percents.same_output = (rhash_data.out == stdout && isatty(0));
	percents.ticks = rhash_get_ticks();
	return 1;
}

/**
 * Output one-line percents by printing them after file path.
 * In the case the total file length is unknow (i.e. hashing stdin)
 * output a rotating stick.
 *
 * @param info pointer to the file-info structure
 * @param offset current file offset in bytes
 */
static void p_update_percents(struct file_info *info, uint64_t offset)
{
	static const char rot[4] = {'-', '\\', '|', '/'};
	int perc = 0;
	unsigned ticks;

#ifdef WIN32_USE_CURSOR
	COORD dwCursorPosition;
	if(percents.use_cursor && percents.hOut == NULL) return;
#endif

	if(info->size > 0) {
		/* use only two digits to display percents: 0%-99% */
		perc = (int)( offset * 99.9 / (uint64_t)info->size );
		if(percents.points == perc) return;
	}

	/* update percents no more than 20 times per second */
	ticks = rhash_get_ticks(); /* clock ticks count in milliseconds */
	if((unsigned)(ticks - percents.ticks) < 50) return;

	/* output percents or rotated bar */
	if(info->size > 0) {
		fprintf(rhash_data.log, "%u%%", perc);
		percents.points = perc;
	} else {
		fprintf(rhash_data.log, "%c", rot[(percents.points++) & 3]);
	}

#ifdef WIN32_USE_CURSOR
	if(percents.use_cursor) {
		fflush(rhash_data.log);

		/* rewind the cursor position */
		dwCursorPosition.X = percents.cur_x;
		dwCursorPosition.Y = percents.cur_y;
		SetConsoleCursorPosition(percents.hOut, dwCursorPosition);
	} else
#endif
	{
		fprintf(rhash_data.log, "\r%-51s ", info->print_path);
		fflush(rhash_data.log);
	}
	percents.ticks  = ticks;
}

/**
 * Finish one-line percent mode. If in hash verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occured while hashing/checking
 */
static void p_finish_percents(struct file_info *info, int process_res)
{
	int need_check_result;

#ifdef WIN32_USE_CURSOR
	if(percents.use_cursor && percents.hOut == NULL) return;
#endif

	need_check_result = (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) &&
		!((opt.flags & OPT_SKIP_OK) && errno == 0 && info->wrong_sums == 0);
	info->error = process_res;

	if(percents.same_output && need_check_result) {
		print_check_result(info, 0, 1);
	} else {
		fprintf(rhash_data.log, "100%%\n");
		fflush(rhash_data.log);
		if(need_check_result) print_check_result(info, 1, 1);
	}
}

/* three methods of percents output */
struct percents_output_info_t dummy_perc = {
	dummy_init_percents, 0, dummy_finish_percents, "dummy"
};
struct percents_output_info_t dots_perc = {
	dots_init_percents, dots_update_percents, dots_finish_percents, "dots"
};
struct percents_output_info_t p_perc = {
	p_init_percents, p_update_percents, p_finish_percents, "digits"
};

/**
 * Initialize pointers to output functions.
 */
void setup_output(void)
{
	rhash_data.out = stdout;
	rhash_data.log = stderr;

	if(opt.flags & OPT_PERCENTS) {
		/* we don't use _fileno() cause it is not in ISO C90, and so undefined
		when compiling with the GCC -ansi option */
		if(rhash_data.log == stderr && isatty(2)) {
			percents_output  = &p_perc;
		} else
		{
			percents_output  = &dots_perc;
		}
	} else {
		percents_output  = &dummy_perc;
	}

	if(opt.output) {
#ifdef _WIN32
		if( !(rhash_data.out = _wfsopen((wchar_t*)opt.output, L"w", _SH_DENYNO)) ) {
#else
		if( !(rhash_data.out = fopen(opt.output, "w")) ) {
#endif
			fprintf(stderr, PROGRAM_NAME ": %s: %s\n", opt.output, strerror(errno));
			rsh_exit(-1);
		}
	}

	if(opt.log) {
#ifdef _WIN32
		if( !(rhash_data.log = _wfsopen((wchar_t*)opt.log, L"w", _SH_DENYNO)) ) {
#else
		if( !(rhash_data.log = fopen(opt.log, "w")) ) {
#endif
			fprintf(stderr, PROGRAM_NAME ": %s: %s\n", opt.log, strerror(errno));
			rsh_exit(-1);
		}
	}
}

/* misc output functions */

/**
 * Print total statistics of crc file checking.
 */
void print_check_stats(void)
{
	if(rhash_data.processed == rhash_data.ok) {
		/* NOTE: don't use puts() here cause it mess with printf stdout buffering */
		fprintf(rhash_data.out, "Everything OK\n");
	} else {
		fprintf(rhash_data.out, "Errors Occurred: Errors:%-3u Miss:%-3u Success:%-3u Total:%-3u\n", rhash_data.processed-rhash_data.ok-rhash_data.miss, rhash_data.miss, rhash_data.ok, rhash_data.processed);
	}
	fflush(rhash_data.out);
}

/**
 * Print file processing times.
 */
void print_file_time_stats(struct file_info* info)
{
	print_time_stats(info->time, info->size, 0);
}

/**
 * Print processing time statistics.
 */
void print_time_stats(double time, uint64_t size, int total)
{
	double speed = (time == 0 ? 0 : (double)(int64_t)size / 1048576.0 / time);
	fprintf(rhash_data.log, "%s %.3f sec, %4.2f MBps\n",
		(total ? "Total" : "Calculated in"), time, speed);
	fflush(rhash_data.log);
}
