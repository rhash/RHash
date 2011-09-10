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
#include "win_utils.h"
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
 * @param args
 */
static void log_va_msg(const char* format, va_list args)
{
	FILE* log = rhash_data.log;
	vfprintf(log, format, args);
	fflush(log);
}

/**
 * Print a formated message to program log, and flush the log stream.
 *
 * @param format print a formated message to the program log
 */
void log_msg(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	log_va_msg(format, ap);
}

/**
 * Print an error to program log.
 *
 * @param format print a formated message to the program log
 */
void log_error(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(rhash_data.log, _("%s: "), PROGRAM_NAME);
	log_va_msg(format, ap);
}

/**
 * Print an error to program log.
 *
 * @param filepath the path to file caused the error
 */
void log_warning(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(rhash_data.log, _("%s: "), PROGRAM_NAME);
	log_va_msg(format, ap);
}

/**
 * Print a file error to program log.
 *
 * @param filepath path to the file, which caused the error
 */
void log_file_error(const char* filepath)
{
	log_error(_("%s: %s\n"), filepath, strerror(errno));
}

/**
 * Information about printed percents.
 */
struct percents_t
{
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

/* the hash functions, which needs to be reported first on mismatch */
#define REPORT_FIRST_MASK (RHASH_MD5 | RHASH_SHA256 | RHASH_SHA512)

/**
 * Print verbose error on hash sums mismatch.
 * 
 * @param info file information with path and its hash sums.
 */
static void print_verbose_error(struct file_info *info)
{
	char actual[130], expected[130];
	assert(HC_FAILED(info->hc.flags));

	fprintf(rhash_data.out, _("ERROR"));

	if(HC_WRONG_FILESIZE & info->hc.flags) {
		sprintI64(actual, info->rctx->msg_size, 0);
		sprintI64(expected, info->hc.file_size, 0);
		fprintf(rhash_data.out, _(", size is %s should be %s"), actual, expected);
	}

	if(HC_WRONG_EMBCRC32 & info->hc.flags) {
		rhash_print(expected, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
		fprintf(rhash_data.out, _(", embedded CRC32 should be %s"), expected);
	}

	if(HC_WRONG_HASHES & info->hc.flags) {
		int i;
		unsigned reported = 0;
		for(i = 0; i < info->hc.hashes_num; i++) {
			hash_value *hv = &info->hc.hashes[i];
			char *expected_hash = info->hc.data + hv->offset;
			unsigned hid = hv->hash_id;
			int pflags;
			if((info->hc.wrong_hashes & (1 << i)) == 0) continue;

			assert(hid != 0);

			/* if can't detect precise hash */
			if((hid & (hid - 1)) != 0) {
				/* guess the hash id */
				if(hid & opt.sum_flags) hid &= opt.sum_flags;
				if(hid & ~info->hc.found_hash_ids) hid &= ~info->hc.found_hash_ids;
				if(hid & ~reported) hid &= ~reported; /* avoiding repeating */
				if(hid & REPORT_FIRST_MASK) hid &= REPORT_FIRST_MASK;
				hid &= -(int)hid; /* take the lowest bit */
			}
			assert(hid != 0 && (hid & (hid - 1)) == 0); /* single bit only */
			reported |= hid;

			pflags = (hv->length == (rhash_get_digest_size(hid) * 2) ?
				(RHPR_HEX | RHPR_UPPERCASE) : (RHPR_BASE32 | RHPR_UPPERCASE));
			rhash_print(actual, info->rctx, hid, pflags);
			fprintf(rhash_data.out, _(", %s is %s should be %s"),
				rhash_get_name(hid), actual, expected_hash);
		}
	}

	fprintf(rhash_data.out, "\n");
}


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
		} else if(!HC_FAILED(info->hc.flags) || !(opt.flags & OPT_VERBOSE)) {
			/* TRANSLATORS: use at least 3 characters to overwrite "99%" */
			fprintf(rhash_data.out, (!HC_FAILED(info->hc.flags) ? _("OK \n") : _("ERR\n")) );
		} else {
			print_verbose_error(info);
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

		if(!init && (opt.flags & OPT_SKIP_OK) && errno == 0 && !HC_FAILED(info->hc.flags)) {
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
 * @return non-zero, indicating that the output method successfully initialized
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
 * @param process_res non-zero if error occurred while hashing/checking
 */
static void dummy_finish_percents(struct file_info *info, int process_res)
{
	info->error = process_res;
	print_results_on_check(info, 0);
}

/* functions to output file info with simple multi-line wget-like percents */

/**
 * Initialize dots percent mode.
 *
 * @param info pointer to the file-info structure
 * @return non-zero, indicating that the output method successfully initialized
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
 * @param process_res non-zero if error occurred while hashing/checking
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
		if (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			fprintf(rhash_data.log, _("\nChecking %s\n"), info->print_path);
		} else {
			fprintf(rhash_data.log, _("\nProcessing %s\n"), info->print_path);
		}
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
 * @return non-zero if the output method successfully initialized
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
 * @param process_res non-zero if error occurred while hashing/checking
 */
static void p_finish_percents(struct file_info *info, int process_res)
{
	int need_check_result;

#ifdef WIN32_USE_CURSOR
	if(percents.use_cursor && percents.hOut == NULL) return;
#endif

	need_check_result = (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) &&
		!((opt.flags & OPT_SKIP_OK) && errno == 0 && !HC_FAILED(info->hc.flags));
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

static void setup_log_stream(FILE **p_stream, const char *stream_path)
{
	if(stream_path) {
#ifdef _WIN32
		if( !(*p_stream = _wfsopen((wchar_t*)stream_path, L"w", _SH_DENYNO)) ) {
			stream_path = w2c((wchar_t*)stream_path);
#else
		if( !(*p_stream = fopen(stream_path, "w")) ) {
#endif
			log_error(_("%s: %s\n"), stream_path, strerror(errno));
			rsh_exit(2);
		}
	}
}

/**
 * Initialize pointers to output functions.
 */
void setup_output(void)
{
	rhash_data.out = stdout;
	rhash_data.log = stderr;

	if(opt.flags & OPT_PERCENTS) {
		/* we don't use _fileno() cause it is not in ISO C90, and so
		 * is incompatible with the GCC -ansi option */
		if(rhash_data.log == stderr && isatty(2)) {
			percents_output  = &p_perc; /* one-line percents */
		} else {
			percents_output  = &dots_perc; /* print percents as dots */
		}
	} else {
		percents_output  = &dummy_perc; /* no percents */
	}

	setup_log_stream(&rhash_data.out, opt.output);
	setup_log_stream(&rhash_data.log, opt.log);
}

/* misc output functions */

/**
 * Print total statistics of crc file checking.
 */
void print_check_stats(void)
{
	if(rhash_data.processed == rhash_data.ok) {
		/* NOTE: don't use puts() here cause it mess with printf stdout buffering */
		fprintf(rhash_data.out, _("Everything OK\n"));
	} else {
		fprintf(rhash_data.out, _("Errors Occurred: Errors:%-3u Miss:%-3u Success:%-3u Total:%-3u\n"), rhash_data.processed-rhash_data.ok-rhash_data.miss, rhash_data.miss, rhash_data.ok, rhash_data.processed);
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
	if (total) {
		fprintf(rhash_data.log, _("Total %.3f sec, %4.2f MBps\n"), time, speed);
	} else {
		fprintf(rhash_data.log, _("Calculated in %.3f sec, %4.2f MBps\n"), time, speed);
	}
	fflush(rhash_data.log);
}
