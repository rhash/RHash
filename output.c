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
struct percents_output_info_t* percents_output = NULL;

#ifdef _WIN32
# define IS_UTF8() (opt.flags & OPT_UTF8)
#else
# define IS_UTF8() 1
#endif

/**
 * Calculate the number of symbols printed by fprintf_file_t(...) for a given formated message.
 * @patam format (nullable) message format string
 * @param path file path
 * @param output_flags bitmask specifying path output mode
 * @return the number of printed symbols
 */
static int count_printed_size(const char* format, const char* path, unsigned output_flags)
{
	size_t format_length = 0;
	if (format) {
		assert(strstr(format, "%s") != NULL);
		format_length = (IS_UTF8() ? count_utf8_symbols(format) : strlen(format)) - 2;
	}
	assert(path != NULL);
	return format_length + (IS_UTF8() || (output_flags & OutForceUtf8) ? count_utf8_symbols(path) : strlen(path));
}

/**
 * Print formatted file path to the specified stream.
 *
 * @param out the stream to write to
 * @param format (nullable) format string
 * @param file the file, which path will be formatted
 * @param output_flags bitmask containing mix of OutForceUtf8, OutBaseName, OutCountSymbols flags
 * @return the number of characters printed, -1 on fail with error code stored in errno
 */
int fprintf_file_t(FILE* out, const char* format, struct file_t* file, unsigned output_flags)
{
	unsigned basename_bit = output_flags & FPathBaseName;
#ifdef _WIN32
	const char* print_path;
	if (!file->real_path) {
		print_path = file_get_print_path(file, FPathPrimaryEncoding | FPathNotNull | basename_bit);
	} else {
		unsigned ppf = ((output_flags & OutForceUtf8) || (opt.flags & OPT_UTF8) ? FPathUtf8 | FPathNotNull : FPathPrimaryEncoding);
		assert(file->real_path != NULL);
		assert((int)OutBaseName == (int)FPathBaseName);
		print_path = file_get_print_path(file, ppf | basename_bit);
		if (!print_path) {
			print_path = file_get_print_path(file, FPathUtf8 | FPathNotNull | basename_bit);
			assert(print_path);
			assert(!(opt.flags & OPT_UTF8));
		}
	}
#else
	const char* print_path = file_get_print_path(file, FPathPrimaryEncoding | FPathNotNull | basename_bit);
	assert((int)OutBaseName == (int)FPathBaseName);
	assert(print_path);
#endif
	if (rsh_fprintf(out, (format ? format : "%s"), print_path) < 0)
		return -1;
	if ((output_flags & OutCountSymbols) != 0)
		return count_printed_size(format, print_path, output_flags);
	return 0;
}

/* RFC 3986: safe url characters are ascii alpha-numeric and "-._~", other characters should be percent-encoded */
static unsigned url_safe_char_mask[4] = { 0, 0x03ff6000, 0x87fffffe, 0x47fffffe };
#define IS_URL_GOOD_CHAR(c) ((unsigned)(c) < 128 && (url_safe_char_mask[c >> 5] & (1 << (c & 31))))

/**
 * Print to a stram an url-encoded representation of the given string.
 *
 * @param out the stream to print the result to
 * @param str string to encode
 * @param upper_case flag to print hex-codes in uppercase
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int fprint_urlencoded(FILE* out, const char* str, int upper_case)
{
	char buffer[1024];
	char* buffer_limit = buffer + (sizeof(buffer) - 3);
	char *p;
	const char hex_add = (upper_case ? 'A' - 10 : 'a' - 10);
	while (*str) {
		for (p = buffer; p < buffer_limit && *str; str++) {
			if (IS_URL_GOOD_CHAR(*str)) {
				*(p++) = *str;
			} else {
				unsigned char hi = ((unsigned char)(*str) >> 4) & 0x0f;
				unsigned char lo = (unsigned char)(*str) & 0x0f;
				*(p++) = '%';
				*(p++) = (hi > 9 ? hi + hex_add : hi + '0');
				*(p++) = (lo > 9 ? lo + hex_add : lo + '0');
			}
		}
		*p = 0;
		if (rsh_fprintf(out, "%s", buffer) < 0)
			return -1;
	}
	return 0;
}

/*=========================================================================
 * Logging functions
 *=========================================================================*/

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
	va_end(ap);
}

/**
 * Print a formatted message, where a single %s substring is replaced with a filepath, and flush the log stream.
 * This function aims to correctly process utf8 conversion on windows.
 * Note: on windows the format string must be in utf8 encoding.
 *
 * @param format the format string of a formatted message
 * @param file the file, which path will be formatted
 */
void log_msg_file_t(const char* format, struct file_t* file)
{
	fprintf_file_t(rhash_data.log, format, file, OutDefaultFlags);
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
	va_end(ap);
}

/**
 * Print file error to the program log.
 *
 * @param file the file, caused the error
 */
void log_error_file_t(struct file_t* file)
{
	int file_errno = errno;
	print_log_prefix();
	fprintf_file_t(rhash_data.log, "%s", file, OutDefaultFlags);
	rsh_fprintf(rhash_data.log, ": %s\n", strerror(file_errno));
	fflush(rhash_data.log);
}

/**
 * Print a formated error message with file path.
 *
 * @param file the file, caused the error
 */
void log_error_msg_file_t(const char* format, struct file_t* file)
{
	print_log_prefix();
	fprintf_file_t(rhash_data.log, format, file, OutDefaultFlags);
	fflush(rhash_data.log);
}

/**
 * Log the message, that the program was interrupted.
 * The function should be called only once.
 */
void report_interrupted(void)
{
	static int is_interrupted_reported = 0;
	assert(rhash_data.stop_flags != 0);
	if (rhash_data.stop_flags == InterruptedFlag && !is_interrupted_reported) {
		is_interrupted_reported = 1;
		log_msg(_("Interrupted by user...\n"));
	}
}

/**
 * Information about printed percents.
 */
struct percents_t
{
	int points;
	int same_output;
	unsigned ticks;
};
static struct percents_t percents;

/* the hash functions, which needs to be reported first on mismatch */
#define REPORT_FIRST_MASK (RHASH_MD5 | RHASH_SHA256 | RHASH_SHA512)

/**
 * Print verbose error on a message digest mismatch.
 *
 * @param info file information with path and its message digests
 * @return 0 on success, -1 on error
 */
static int print_verbose_hash_check_error(struct file_info* info)
{
	char actual[130], expected[130];
	assert(HP_FAILED(info->hp->bit_flags));

	/* TRANSLATORS: printed in the verbose mode on a message digest mismatch */
	if (rsh_fprintf(rhash_data.out, _("ERROR")) < 0)
		return -1;

	if ((HpWrongFileSize & info->hp->bit_flags)) {
		sprintI64(actual, info->rctx->msg_size, 0);
		sprintI64(expected, info->hp->file_size, 0);
		if (rsh_fprintf(rhash_data.out, _(", size is %s should be %s"), actual, expected) < 0)
			return -1;
	}

	if (HpWrongEmbeddedCrc32 & info->hp->bit_flags) {
		rhash_print(expected, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
		if (rsh_fprintf(rhash_data.out, _(", embedded CRC32 should be %s"), expected) < 0)
			return -1;
	}

	if (HpWrongHashes & info->hp->bit_flags) {
		unsigned reported = 0;
		int i;
		for (i = 0; i < info->hp->hashes_num; i++) {
			struct hash_value* hv = &info->hp->hashes[i];
			char* expected_hash = info->hp->line_begin + hv->offset;
			unsigned hid = hv->hash_id;
			int pflags;
			if ((info->hp->wrong_hashes & (1 << i)) == 0)
				continue;

			assert(hid != 0);

			/* if can't detect precise hash */
			if ((hid & (hid - 1)) != 0) {
				/* guess the hash id */
				if (hid & opt.sum_flags) hid &= opt.sum_flags;
				if (hid & ~info->hp->found_hash_ids) hid &= ~info->hp->found_hash_ids;
				if (hid & ~reported) hid &= ~reported; /* avoid repeating */
				if (hid & REPORT_FIRST_MASK) hid &= REPORT_FIRST_MASK;
				hid &= -(int)hid; /* take the lowest bit */
			}
			assert(hid != 0 && (hid & (hid - 1)) == 0); /* single bit only */
			reported |= hid;

			pflags = (hv->length == (rhash_get_digest_size(hid) * 2) ?
				(RHPR_HEX | RHPR_UPPERCASE) : (RHPR_BASE32 | RHPR_UPPERCASE));
			rhash_print(actual, info->rctx, hid, pflags);
			/* TRANSLATORS: print a message like "CRC32 is ABC12345 should be BCA54321" */
			if (rsh_fprintf(rhash_data.out, _(", %s is %s should be %s"),
					rhash_get_name(hid), actual, expected_hash) < 0)
				return -1;
		}
	}
	return PRINTF_RES(rsh_fprintf(rhash_data.out, "\n"));
}

/**
 * Print file path and no more than 52 spaces.
 *
 * @param out stream to print the filepath
 * @param info pointer to the file-info structure
 * @return 0 on success, -1 on i/o error
 */
static int print_aligned_filepath(FILE* out, struct file_info* info)
{
	int symbols_count = fprintf_file_t(out, NULL, info->file, OutCountSymbols);
	if (symbols_count >= 0) {
		char buf[56];
		int spaces_count = (symbols_count <= 51 ? 52 - symbols_count : 1);
		return PRINTF_RES(rsh_fprintf(out, "%s", str_set(buf, ' ', spaces_count)));
	}
	return -1;
}

/**
 * Print file path and result of its verification against message digests.
 * Also if error occurred, print error message.
 *
 * @param info pointer to the file-info structure
 * @param print_name set to non-zero to print file path
 * @param print_result set to non-zero to print verification result
 * @return 0 on success, -1 on i/o error
 */
static int print_check_result(struct file_info* info, int print_name, int print_result)
{
	int saved_errno = errno;
	int res = 0;
	if (print_name)
		res = print_aligned_filepath(rhash_data.out, info);
	if (print_result && res == 0) {
		if (info->processing_result < 0) {
			/* print error to stdout */
			res = PRINTF_RES(rsh_fprintf(rhash_data.out, "%s\n", strerror(saved_errno)));
		} else if (!HP_FAILED(info->hp->bit_flags) || !(opt.flags & OPT_VERBOSE)) {
			res = PRINTF_RES(rsh_fprintf(rhash_data.out, (!HP_FAILED(info->hp->bit_flags) ?
				/* TRANSLATORS: printed when all message digests match, use at least 3 characters to overwrite "99%" */
				_("OK \n") :
				/* TRANSLATORS: ERR (short for 'error') is printed on a message digest mismatch */
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
 * @param init non-zero on initialization before message digests calculation,
 *             and zero after message digests calculation finished.
 * @return 0 on success, -1 on i/o error
 */
static int print_results_on_check(struct file_info* info, int init)
{
	if (IS_MODE(MODE_CHECK | MODE_CHECK_EMBEDDED)) {
		int print_name = (opt.flags & (OPT_PERCENTS | OPT_SKIP_OK) ? !init : init);

		/* print result, but skip OK messages if required */
		if (init || info->processing_result != 0 || !(opt.flags & OPT_SKIP_OK) || HP_FAILED(info->hp->bit_flags))
			return print_check_result(info, print_name, !init);
	}
	return 0;
}

/* functions to output file info without percents */

/**
 * Print file name in the verification mode.
 * No information is printed in other modes.
 *
 * @param info pointer to the file-info structure
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dummy_init_percents(struct file_info* info)
{
	return print_results_on_check(info, 1);
}

/**
 * Print file check results without printing percents.
 * Information is printed only in the verification mode.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dummy_finish_percents(struct file_info* info, int process_res)
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
static int dots_init_percents(struct file_info* info)
{
	int res = fflush(rhash_data.out);
	fflush(rhash_data.log);
	percents.points = 0;
	if (print_results_on_check(info, 1) < 0)
		res = -1;
	return res;
}

/**
 * Finish dots percent mode. If in the verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int dots_finish_percents(struct file_info* info, int process_res)
{
	char buf[80];
	info->processing_result = process_res;
	if ((percents.points % 74) != 0) {
		log_msg("%s 100%%\n", str_set(buf, ' ', 74 - (percents.points % 74)));
	}
	return print_results_on_check(info, 0);
}

/**
 * Output percents by printing one dot per each processed 1MiB.
 *
 * @param info pointer to the file-info structure
 * @param offset the number of hashed bytes
 */
static void dots_update_percents(struct file_info* info, uint64_t offset)
{
	const int pt_size = 1024 * 1024; /* 1MiB */
	offset -= info->msg_offset; /* get real file offset */
	if ( (offset % pt_size) != 0 ) return;

	if (percents.points == 0) {
		if (IS_MODE(MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			rsh_fprintf(rhash_data.log, _("\nChecking %s\n"), file_get_print_path(info->file, FPathPrimaryEncoding | FPathNotNull));
		} else {
			rsh_fprintf(rhash_data.log, _("\nProcessing %s\n"), file_get_print_path(info->file, FPathPrimaryEncoding | FPathNotNull));
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
static int p_init_percents(struct file_info* info)
{
	int res = fflush(rhash_data.out);
	fflush(rhash_data.log);
	/* ingnore output errors, while logging to rhash_data.log */
	print_aligned_filepath(rhash_data.log, info);
	percents.points = 0;
	percents.same_output = (rhash_data.out == stdout && isatty(0) &&
			rhash_data.log == stderr && isatty(1));
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
static void p_update_percents(struct file_info* info, uint64_t offset)
{
	static const char rotated_bar[4] = {'-', '\\', '|', '/'};
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
	if ((unsigned)(ticks - percents.ticks) < 50)
		return;

	/* output percents or a rotated bar */
	if (info->size > 0) {
		rsh_fprintf(rhash_data.log, "%u%%\r", perc);
		percents.points = perc;
	} else {
		rsh_fprintf(rhash_data.log, "%c\r", rotated_bar[(percents.points++) & 3]);
	}
	print_aligned_filepath(rhash_data.log, info);
	fflush(rhash_data.log);
	percents.ticks  = ticks;
}

/**
 * Finish one-line percent mode. If in the verification mode,
 * then print the results of file check.
 *
 * @param info pointer to the file-info structure
 * @param process_res non-zero if error occurred while hashing/checking
 * @return 0 on success, -1 if output to rhash_data.out failed
 */
static int p_finish_percents(struct file_info* info, int process_res)
{
	int need_check_result = IS_MODE(MODE_CHECK | MODE_CHECK_EMBEDDED) &&
		!((opt.flags & OPT_SKIP_OK) && process_res == 0 && !HP_FAILED(info->hp->bit_flags));
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
 * @param p_stream the stream to initialize
 * @param stream_path the path to the log file, or NULL, to use the default stream
 * @param default_stream the default stream value, for the case of invalid stream_path
 */
static void setup_log_stream(FILE** p_stream, file_t* file, const opt_tchar* stream_path, FILE* default_stream)
{
	FILE* result;
	/* set to the default stream, to enable error reporting via log_error_file_t() */
	*p_stream = default_stream;
	if (!stream_path) {
		file_init_by_print_path(file, NULL, (default_stream == stdout ? "(stdout)" : "(stderr)"), FileIsStdStream);
		return;
	}
	file_init(file, stream_path, FileInitReusePath);
	result = file_fopen(file, FOpenWrite);
	if (!result) {
		log_error_file_t(file);
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
 * Print the "Verifying <FILE>" heading line.
 *
 * @param file the file containing message digests to verify
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_verifying_header(file_t* file)
{
	if ((opt.flags & OPT_BRIEF) == 0)
	{
		char dash_line[84];
		/* TRANSLATORS: the line printed before a hash file is verified */
		int count = fprintf_file_t(rhash_data.out, _("\n--( Verifying %s )"), file, OutCountSymbols);
		int tail_dash_len = (0 < count && count < 81 ? 81 - count : 2);
		int res = rsh_fprintf(rhash_data.out, "%s\n", str_set(dash_line, '-', tail_dash_len));
		if (res >= 0)
			res = fflush(rhash_data.out);
		return (count < 0 ? count : res);
	}
	return 0;
}

/**
 * Print a line consisting of 80 dashes.
 *
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_verifying_footer(void)
{
	char dash_line[84];
	return (opt.flags & OPT_BRIEF ? 0 :
		rsh_fprintf(rhash_data.out, "%s\n", str_set(dash_line, '-', 80)));
}

/**
 * Print total statistics of hash file checking.
 *
 * @return 0 on success, -1 on i/o error with error code stored in errno
 */
int print_check_stats(void)
{
	int res;
	if (rhash_data.processed == rhash_data.ok) {
		/* NOTE: don't use puts() here cause it messes up with fprintf stdout buffering */
		const char* message = (rhash_data.processed > 0 ?
			_("Everything OK\n") :
			_("Nothing to verify\n"));
		res = PRINTF_RES(rsh_fprintf(rhash_data.out, "%s", message));
	} else {
		res = PRINTF_RES(rsh_fprintf(rhash_data.out, _("Errors Occurred: Errors:%-3u Miss:%-3u Success:%-3u Total:%-3u\n"),
			rhash_data.processed - rhash_data.ok - rhash_data.miss, rhash_data.miss, rhash_data.ok, rhash_data.processed));
	}
	return (fflush(rhash_data.out) < 0 ? -1 : res);
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
 *
 * @param time time in milliseconds
 * @param size total size of the processed data
 * @param total boolean flag, to print total or per-file result
 */
void print_time_stats(uint64_t time, uint64_t size, int total)
{
	double seconds = (double)(int64_t)time / 1000.0;
	double speed = (time == 0 ? 0 : (double)(int64_t)size / 1048576.0 / seconds);
	if (total) {
		rsh_fprintf(rhash_data.log, _("Total %.3f sec, %4.2f MBps\n"), seconds, speed);
	} else {
		rsh_fprintf(rhash_data.log, _("Calculated in %.3f sec, %4.2f MBps\n"), seconds, speed);
	}
	fflush(rhash_data.log);
}
