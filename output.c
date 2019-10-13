/* output.c - output of results, errors and percents */

#include "output.h"
#include "calc_sums.h"
#include "parse_cmdline.h"
#include "platform.h"
#include "rhash_main.h"
#include "win_utils.h"
#include "librhash/rhash.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* exit() */
#include <assert.h>
#include <errno.h>

#ifdef _WIN32
# include <windows.h>
# include <io.h> /* for isatty */
#endif

/* global pointer to the selected method of percents output */
struct percents_output_info_t *percents_output = NULL;

/**
 * Print message prefix before printing an error/warning message.
 *
 * @return the number of characters printed, -1 on error
 */
static int print_log_prefix(void)
{
	return rsh_fprintf(rhash_data.log, "%s: ", PROGRAM_NAME);
}

/**
 * Print a formatted message to the program log, and flush the log stream.
 *
 * @param format print a formatted message to the program log
 * @param args
 */
static void log_va_msg(const char* format, va_list args)
{
	rsh_vfprintf(rhash_data.log, format, args);
	fflush(rhash_data.log);
}

/**
 * Print a formatted message to the program log, and flush the log stream.
 *
 * @param format print a formatted message to the program log
 */
void log_msg(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	log_va_msg(format, ap);
}

/**
 * Print formatted file path to the specified stream.
 *
 * @param out the stream to write to
 * @param format the format string
 * @param file the file, which path will be formatted
 * @return the number of characters printed, -1 on error
 */
int fprintf_file_t(FILE* out, const char* format, struct file_t* file)
{
#ifdef _WIN32
	if (!FILE_TPATH(file) || !win_is_console_stream(out))
		return rsh_fprintf(out, format, file_cpath(file));
#endif
	return rsh_fprintf_targ(out, format, FILE_TPATH(file));
}

/**
 * Print a formatted message, where a single %s substring is replaced with a filepath, and flush the log stream.
 * This function aims to correctly process utf8 conversion on windows.
 * Note: on windows the format string must be in utf8 encoding.
 *
 * @param format the format string of a formatted message
 * @param file the file, which path will be formatted
 */
void log_file_t_msg(const char* format, struct file_t* file)
{
	fprintf_file_t(rhash_data.log, format, file);
	fflush(rhash_data.log);
}

/**
 * Print file error to the program log.
 *
 * @param file the file, caused the error
 */
void log_file_t_error(struct file_t* file)
{
	int file_errno = errno;
	print_log_prefix();
	fprintf_file_t(rhash_data.log, "%s", file);
	rsh_fprintf(rhash_data.log, ": %s\n", strerror(file_errno));
	fflush(rhash_data.log);
}

/**
 * Print a formatted error message to the program log.
 *
 * @param format the format string
 */
void log_error(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	print_log_prefix();
	log_va_msg(format, ap);
}

/**
 * Print a formatted warning message to the program log.
 *
 * @param format the format string
 */
void log_warning(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	print_log_prefix();
	log_va_msg(format, ap);
}

/* global flag */
int is_interrupted_reported = 0;

/**
 * Log the message, that the program was interrupted.
 * The function should be called only once.
 */
void report_interrupted(void)
{
	assert(rhash_data.stop_flags != 0);
	if (rhash_data.stop_flags != InterruptedFlag || is_interrupted_reported)
		return;
	is_interrupted_reported = 1;
	log_msg(_("Interrupted by user...\n"));
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
};
static struct percents_t percents;

/* the hash functions, which needs to be reported first on mismatch */
#define REPORT_FIRST_MASK (RHASH_MD5 | RHASH_SHA256 | RHASH_SHA512)

/**
 * Print verbose error on a hash sum mismatch.
 *
 * @param info file information with path and its hash sums.
 * @return 0 on success, -1 on error
 */
static int print_verbose_hash_check_error(struct file_info *info)
{
	char actual[130], expected[130];
	assert(HC_FAILED(info->hc.flags));

	/* TRANSLATORS: printed in verbose mode on a hash sum mismatch */
	if (rsh_fprintf(rhash_data.out, _("ERROR")) < 0)
		return -1;

	if ((HC_WRONG_FILESIZE & info->hc.flags)) {
		sprintI64(actual, info->rctx->msg_size, 0);
		sprintI64(expected, info->hc.file_size, 0);
		if (rsh_fprintf(rhash_data.out, _(", size is %s should be %s"), actual, expected) < 0)
			return -1;
	}

	if (HC_WRONG_EMBCRC32 & info->hc.flags) {
		rhash_print(expected, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
		if (rsh_fprintf(rhash_data.out, _(", embedded CRC32 should be %s"), expected) < 0)
			return -1;
	}

	if (HC_WRONG_HASHES & info->hc.flags) {
		unsigned reported = 0;
		int i;
		for (i = 0; i < info->hc.hashes_num; i++) {
			hash_value *hv = &info->hc.hashes[i];
			char *expected_hash = info->hc.data + hv->offset;
			unsigned hid = hv->hash_id;
			int pflags;
			if ((info->hc.wrong_hashes & (1 << i)) == 0)
				continue;

			assert(hid != 0);

			/* if can't detect precise hash */
			if ((hid & (hid - 1)) != 0) {
				/* guess the hash id */
				if (hid & opt.sum_flags) hid &= opt.sum_flags;
				if (hid & ~info->hc.found_hash_ids) hid &= ~info->hc.found_hash_ids;
				if (hid & ~reported) hid &= ~reported; /* avoid repeating */
				if (hid & REPORT_FIRST_MASK) hid &= REPORT_FIRST_MASK;
				hid &= -(int)hid; /* take the lowest bit */
			}
			assert(hid != 0 && (hid & (hid - 1)) == 0); /* single bit only */
			reported |= hid;

			pflags = (hv->length == (rhash_get_digest_size(hid) * 2) ?
				(RHPR_HEX | RHPR_UPPERCASE) : (RHPR_BASE32 | RHPR_UPPERCASE));
			rhash_print(actual, info->rctx, hid, pflags);
			/* TRANSLATORS: messages like "CRC32 is ABC12345 should be BCA54321" */
			if (rsh_fprintf(rhash_data.out, _(", %s is %s should be %s"),
					rhash_get_name(hid), actual, expected_hash) < 0)
				return -1;
		}
	}
	return PRINTF_RES(rsh_fprintf(rhash_data.out, "\n"));
}

/**
 * Print file path and result of its verification by hash.
 * Also if error occurred, print error message.
 *
 * @param info pointer to the file-info structure
 * @param print_name set to non-zero to print file path
 * @param print_result set to non-zero to print hash verification result
 * @return 0 on success, -1 on i/o error
 */
static int print_check_result(struct file_info *info, int print_name, int print_result)
{
	int saved_errno = errno;
	int res = 0;
	if (print_name) {
		res = PRINTF_RES(fprintf_file_t(rhash_data.out, "%-51s ", info->file));
	}
	if (print_result && res == 0) {
		if (info->processing_result < 0) {
			/* print error to stdout */
			res = PRINTF_RES(rsh_fprintf(rhash_data.out, "%s\n", strerror(saved_errno)));
		} else if (!HC_FAILED(info->hc.flags) || !(opt.flags & OPT_VERBOSE)) {
			res = PRINTF_RES(rsh_fprintf(rhash_data.out, (!HC_FAILED(info->hc.flags) ?
				/* TRANSLATORS: printed when hash sums match, use at least 3 characters to overwrite "99%" */
				("OK \n") :
				/* TRANSLATORS: ERR (short for 'error') is printed on hash mismatch */
				_("ERR\n"))));
		} else {
			res = print_verbose_hash_check_error(info);
		}
	}
	if (fflush(rhash_data.out) < 0)
		res = -1;
	return res;
}

/**
 * Prepare or print result of file processing.
 *
 * @param info pointer to the file-info structure
 * @param init non-zero on initialization before hash calculation,
 *             and zero after hash calculation finished.
 * @return 0 on success, -1 on i/o error
 */
static int print_results_on_check(struct file_info *info, int init)
{
	if (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
		int print_name = (opt.flags & (OPT_PERCENTS | OPT_SKIP_OK) ? !init : init);

		/* print result, but skip OK messages if required */
		if (init || info->processing_result != 0 || !(opt.flags & OPT_SKIP_OK) || HC_FAILED(info->hc.flags))
			return print_check_result(info, print_name, !init);
	}
	return 0;
}

/* functions to output file info without percents */

/**
 * Print file name in hash checking mode.
 * No information is printed in other modes.
 *
 * @param info pointer to the file-info structure
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dummy_init_percents(struct file_info *info)
{
	return print_results_on_check(info, 1);
}

/**
 * Print file check results without printing percents.
 * Information is printed only in hash verification mode.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dummy_finish_percents(struct file_info *info, int process_res)
{
	info->processing_result = process_res;
	return print_results_on_check(info, 0);
}

/* functions to output file info with simple multi-line wget-like percents */

/**
 * Initialize dots percent mode.
 *
 * @param info pointer to the file-info structure
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dots_init_percents(struct file_info *info)
{
	int res = fflush(rhash_data.out);
	fflush(rhash_data.log);
	(void)info;
	percents.points = 0;
	if (print_results_on_check(info, 1) < 0)
		res = -1;
	return res;
}

/**
 * Finish dots percent mode. If in hash verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dots_finish_percents(struct file_info *info, int process_res)
{
	char buf[80];
	info->processing_result = process_res;

	if ((percents.points % 74) != 0) {
		log_msg("%s 100%%\n", str_set(buf, ' ', 74 - (percents.points%74) ));
	}
	return print_results_on_check(info, 0);
}

/**
 * Output percents by printing one dot per each processed 1MiB.
 *
 * @param info pointer to the file-info structure
 * @param offset the number of hashed bytes
 */
static void dots_update_percents(struct file_info *info, uint64_t offset)
{
	const int pt_size = 1024*1024; /* 1MiB */
	offset -= info->msg_offset; /* get real file offset */
	if ( (offset % pt_size) != 0 ) return;

	if (percents.points == 0) {
		if (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			rsh_fprintf(rhash_data.log, _("\nChecking %s\n"), info->print_path);
		} else {
			rsh_fprintf(rhash_data.log, _("\nProcessing %s\n"), info->print_path);
		}
		fflush(rhash_data.log);
	}
	putc('.', rhash_data.log);

	if (((++percents.points) % 74) == 0) {
		if (info->size > 0) {
			int perc = (int)( offset * 100.0 / (uint64_t)info->size + 0.5 );
			rsh_fprintf(rhash_data.log, "  %2u%%\n", perc);
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
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int p_init_percents(struct file_info *info)
{
	int res = fflush(rhash_data.out);
	fflush(rhash_data.log);

	(void)info;
	percents.points      = 0;
	percents.same_output = 0;
	percents.use_cursor  = 0;

	/* note: this output differs from print_check_result() by the file handle, so ingnoring errors */
	rsh_fprintf(rhash_data.log, "%-51s ", info->print_path);

	percents.same_output = (rhash_data.out == stdout && isatty(0));
	percents.ticks = rhash_get_ticks();
	return res;
}

/**
 * Output one-line percents by printing them after file path.
 * If the total file length is unknow (i.e. hashing stdin),
 * then output a rotating stick.
 *
 * @param info pointer to the file-info structure
 * @param offset the number of hashed bytes
 */
static void p_update_percents(struct file_info *info, uint64_t offset)
{
	static const char rot[4] = {'-', '\\', '|', '/'};
	int perc = 0;
	unsigned ticks;

	if (info->size > 0) {
		offset -= info->msg_offset;
		/* use only two digits to display percents: 0%-99% */
		perc = (int)( offset * 99.9 / (uint64_t)info->size );
		if (percents.points == perc) return;
	}

	/* update percents no more than 20 times per second */
	ticks = rhash_get_ticks(); /* clock ticks count in milliseconds */
	if ((unsigned)(ticks - percents.ticks) < 50) return;

	/* output percents or rotated bar */
	if (info->size > 0) {
		rsh_fprintf(rhash_data.log, "%u%%", perc);
		percents.points = perc;
	} else {
		rsh_fprintf(rhash_data.log, "%c", rot[(percents.points++) & 3]);
	}

	rsh_fprintf(rhash_data.log, "\r%-51s ", info->print_path);
	fflush(rhash_data.log);
	percents.ticks  = ticks;
}

/**
 * Finish one-line percent mode. If in hash verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int p_finish_percents(struct file_info *info, int process_res)
{
	int need_check_result;

	need_check_result = (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) &&
		!((opt.flags & OPT_SKIP_OK) && process_res == 0 && !HC_FAILED(info->hc.flags));
	info->processing_result = process_res;

	if (percents.same_output && need_check_result) {
		return print_check_result(info, 0, 1);
	} else {
		rsh_fprintf(rhash_data.log, "100%%\n");
		fflush(rhash_data.log);
		if (need_check_result)
			return print_check_result(info, 1, 1);
	}
	return 0;
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
 * Initialize given output stream.
 *
 * @param p_stream the stream to initialize.
 * @param stream_path the path to the log file, or NULL, to use the default stream
 * @param default_stream the default stream value, for the case of invalid stream_path
 */
static void setup_log_stream(FILE **p_stream, file_t* file, const opt_tchar* stream_path, FILE* default_stream)
{
	FILE* result;
	/* set to the default stream, to enable error reporting via log_file_t_error() */
	*p_stream = default_stream;
	if (!stream_path) {
		file_init(file, (default_stream == stdout ? "(stdout)" : "(stderr)"), FILE_IFSTDIN);
		return;
	}
	file_tinit(file, stream_path, FILE_OPT_DONT_FREE_PATH);
	result = file_fopen(file, FOpenWrite);
	if (!result) {
		log_file_t_error(file);
		rsh_exit(2);
	}
	*p_stream = result;
}

/**
 * Initialize pointers to output functions.
 */
void setup_output(void)
{
	setup_log_stream(&rhash_data.log, &rhash_data.log_file, opt.log, stderr);
	setup_log_stream(&rhash_data.out, &rhash_data.out_file, opt.output, stdout);
}

void setup_percents(void)
{
	if (opt.flags & OPT_PERCENTS) {
		/* NB: we don't use _fileno() cause it is not in ISO C90, and so
		 * is incompatible with the GCC -ansi option */
		if (rhash_data.log == stderr && isatty(2)) {
			/* use one-line percents by default on console */
			percents_output  = &p_perc;
			IF_WINDOWS(hide_cursor());
		} else {
			/* print percents as dots */
			percents_output  = &dots_perc;
		}
	} else {
		percents_output  = &dummy_perc; /* no percents */
	}
}

/* misc output functions */

/**
 * Print total statistics of hash file checking.
 */
void print_check_stats(void)
{
	if (rhash_data.processed == rhash_data.ok) {
		/* NOTE: don't use puts() here cause it mess with printf stdout buffering */
		rsh_fprintf(rhash_data.out, _("Everything OK\n"));
	} else {
		rsh_fprintf(rhash_data.out, _("Errors Occurred: Errors:%-3u Miss:%-3u Success:%-3u Total:%-3u\n"),
			rhash_data.processed-rhash_data.ok-rhash_data.miss, rhash_data.miss, rhash_data.ok, rhash_data.processed);
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
		rsh_fprintf(rhash_data.log, _("Total %.3f sec, %4.2f MBps\n"), time, speed);
	} else {
		rsh_fprintf(rhash_data.log, _("Calculated in %.3f sec, %4.2f MBps\n"), time, speed);
	}
	fflush(rhash_data.log);
}
