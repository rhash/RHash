/* calc_sums.c - crc calculating and printing functions */

#include "common_func.h" /* should be included before the C library files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */
#include <unistd.h> /* read() */
#include <fcntl.h>  /* open() */
#include <time.h>   /* localtime(), time() */
#include <sys/stat.h> /* stat() */
#include <errno.h>
#include <assert.h>

#include "librhash/hex.h"
#include "librhash/byte_order.h"
#include "librhash/rhash.h"
#include "librhash/timing.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "file_set.h"
#include "crc_print.h"
#include "output.h"
#include "win_utils.h"
#include "version.h"

#include "calc_sums.h"

/**
 * Initialize BTIH hash function. Unlike other algorithms BTIH
 * requires more data for correct computation.
 *
 * @param info the file data
 */
static void init_btih_data(struct file_info *info)
{
	assert((info->rctx->hash_id & RHASH_BTIH) != 0);
	rhash_transmit(RMSG_BT_ADD_FILE, info->rctx, RHASH_STR2UPTR((char*)get_basename(file_info_get_utf8_print_path(info))), (rhash_uptr_t)&info->size);
	rhash_transmit(RMSG_BT_SET_PROGRAM_NAME, info->rctx, RHASH_STR2UPTR(PROGRAM_NAME "/" VERSION), 0);

	if(opt.flags & OPT_BT_PRIVATE) {
		rhash_transmit(RMSG_BT_SET_OPTIONS, info->rctx, RHASH_BT_OPT_PRIVATE, 0);
	}

	if(opt.bt_announce) {
		rhash_transmit(RMSG_BT_SET_ANNOUNCE, info->rctx, RHASH_STR2UPTR(opt.bt_announce), 0);
	}

	if(opt.bt_piece_length) {
		rhash_transmit(RMSG_BT_SET_PIECE_LENGTH, info->rctx, RHASH_STR2UPTR(opt.bt_piece_length), 0);
	}
}

/**
 * Calculate hash sums simultaneously, according to the info->sums_flags.
 * Calculated hashes are stored in info->rctx.
 *
 * @param info file data. The info->full_path can be "-" to denote stdin
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int calc_sums(struct file_info *info)
{
	FILE* fd = stdin; /* stdin */
	int res;

	if(IS_DASH_STR(info->full_path)) {
		info->print_path = "(stdin)";

#ifdef _WIN32
		/* using 0 instead of _fileno(stdin). _fileno() is undefined under 'gcc -ansi' */
		if(setmode(0, _O_BINARY) < 0) {
			return -1;
		}
#endif
	} else {
		struct rsh_stat_struct stat_buf;
		/* skip non-existing files */
		if(rsh_stat(info->full_path, &stat_buf) < 0) {
			return -1;
		}

		if((opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) && S_ISDIR(stat_buf.st_mode)) {
			errno = EISDIR;
			return -1;
		}

		info->size = stat_buf.st_size; /* total size, in bytes */
		IF_WINDOWS(win32_set_filesize64(info->full_path, &info->size)); /* set correct filesize for large files under win32 */

		if(!info->sums_flags) return 0;

		/* skip files opened with exclusive rights without reporting an error */
		fd = rsh_fopen_bin(info->full_path, "rb");
		if(!fd) {
			return -1;
		}
	}

	assert(info->rctx == 0);
	info->rctx = rhash_init(info->sums_flags);

	/* initialize BitTorrent data */
	if(info->sums_flags & RHASH_BTIH) {
		init_btih_data(info);
	}

	if(percents_output->update != 0) {
		rhash_set_callback(info->rctx, (rhash_callback_t)percents_output->update, info);
	}

	/* read and hash file content */
	if((res = rhash_file_update(info->rctx, fd)) != -1) {
		rhash_final(info->rctx, 0); /* finalize hashing */

		info->size = info->rctx->msg_size;
		rhash_data.total_size += info->size;
	}

	if(fd != stdin) fclose(fd);
	return res;
}

/**
 * Free memory allocated by given file_info structure.
 *
 * @param info pointer the structure to de-initialize
 */
void file_info_destroy(struct file_info* info)
{
	free(info->utf8_print_path);
	free(info->allocated_ptr);
	rhash_free(info->rctx);
}

/**
 * Store print_path in a file_info struct, replacing if needed
 * system path separators with specified by user command line option.
 *
 * @param info pointer to the the file_info structure to change
 * @param print_path the print path to store
 */
static void file_info_set_print_path(struct file_info* info, const char* print_path)
{
	char *p;
	char wrong_sep;

	/* check if path separator was specified by command line options */
	if(opt.path_separator) {
		wrong_sep = (opt.path_separator == '/' ? '\\' : '/');
		if((p = (char*)strchr(print_path, wrong_sep)) != NULL) {
			info->allocated_ptr = rsh_strdup(print_path);
			info->print_path = info->allocated_ptr;
			p = info->allocated_ptr + (p - print_path);

			/* replace wrong_sep in the print_path with separator defined by options */
			for(; *p; p++) {
				if(*p == wrong_sep) *p = opt.path_separator;
			}
			return;
		}
	}

	/* if path was not replaces, than just store the value */
	info->print_path = print_path;
}

/**
 * Return utf8 version of print_path.
 *
 * @param info file information
 * @return utf8 string on success, NULL if couldn't convert.
 */
const char* file_info_get_utf8_print_path(struct file_info* info)
{
	if(info->utf8_print_path == NULL) {
		if(is_utf8()) return info->print_path;
		info->utf8_print_path = to_utf8(info->print_path);
	}
	return info->utf8_print_path;
}

/* functions to calculate and print file sums */

/**
 * Search for a crc32 hash sum in the given file name.
 *
 * @param filepath the path to the file.
 * @param crc32 pointer to integer to receive parsed hash sum.
 * @return non zero if crc32 was found, zero otherwise.
 */
static int find_embedded_crc32(const char* filepath, unsigned* crc32_be)
{
	const char* e = filepath + strlen(filepath) - 10;

	/* search for the sum enclosed in brackets */
	for(; e >= filepath && !IS_PATH_SEPARATOR(*e); e--) {
		if((*e == '[' && e[9] == ']') || (*e == '(' && e[9] == ')')) {
			const char *p = e + 8;
			for(; p > e && IS_HEX(*p); p--);
			if(p == e) {
				rhash_hex_to_byte(e + 1, (char unsigned*)crc32_be, 8);
				return 1;
			}
			e -= 9;
		}
	}
	return 0;
}

/**
 * Rename given file inserting its crc32 sum enclosed in braces just before
 * the file extension.
 *
 * @param info pointer to the data of the file to rename.
 * @return 0 on success, -1 on fail with error code in errno
 */
int rename_file_to_embed_crc32(struct file_info *info)
{
	size_t len = strlen(info->full_path);
	const char* p = info->full_path + len;
	const char* c = p - 1;
	char* new_path;
	char* insertion_point;
	unsigned crc32_be;
	assert((info->rctx->hash_id & RHASH_CRC32) != 0);

	/* check if the filename contains a CRC32 hash sum */
	if(find_embedded_crc32(info->print_path, &crc32_be)) {
		/* compare with calculated CRC32 */
		if(be2me_32(crc32_be) != 
			*(unsigned*)rhash_get_context_ptr(info->rctx, RHASH_CRC32)) {
			char crc32_str[9];
			rhash_print(crc32_str, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
			log_msg("warning: wrong embedded sum, should be %s\n", crc32_str);
		} else return 0;
	}

	/* find file extension (as the place to insert the hash sum) */
	for(; c >= info->full_path && !IS_PATH_SEPARATOR(*c); c--) {
		if(*c == '.') {
			p = c;
			break;
		}
	}

	/* now p is the point to insert the 10-bytes hash string */
	new_path = (char*)rsh_malloc(len + 11);
	insertion_point = new_path + (p - info->full_path);
	memcpy(new_path, info->full_path, p - info->full_path);
	if(opt.embed_crc_delimiter && *opt.embed_crc_delimiter) *(insertion_point++) = *opt.embed_crc_delimiter;
	rhash_print(insertion_point+1, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
	insertion_point[0] = '[';
	insertion_point[9] = ']'; /* ']' overrides '\0' inserted by rhash_print_sum() */
	strcpy(insertion_point + 10, p); /* append file extension */

	/* rename the file */
	if(rename(info->full_path, new_path) < 0) {
		fprintf(rhash_data.log, PROGRAM_NAME ": can't move %s to %s: %s\n", info->full_path, new_path, strerror(errno));
		free(new_path);
		return -1;
	}

	/* change file name in the file info structure */
	if(info->print_path >= info->full_path && info->print_path < p) {
		file_info_set_print_path(info, new_path + len - strlen(info->print_path));
	} else {
		file_info_set_print_path(info, new_path);
	}

	free(info->full_path);
	info->full_path = new_path;
	return 0;
}

/**
 * Save torrent file.
 *
 * @param info information about the hashed file
 */
static void save_torrent(struct file_info* info)
{
	char *str;
	FILE* fd;
	struct rsh_stat_struct stat_buf;
	size_t path_len = strlen(info->full_path);
	size_t text_len;
	char* path = (char*)rsh_malloc(path_len + 9);

	/* append .torrent extension to the file path */
	memcpy(path, info->full_path, path_len);
	memcpy(path + path_len, ".torrent", 9);

	/* get torrent file content */
	text_len = rhash_transmit(RMSG_BT_GET_TEXT, info->rctx, (unsigned long)&str, 0);
	assert(text_len != RHASH_ERROR);

	if(rsh_stat(path, &stat_buf) >= 0) {
		errno = EEXIST;
		log_file_error(path);
	} else {
		fd = rsh_fopen_bin(path, "wb");
		if(fd && text_len == fwrite(str, 1, text_len, fd) && !ferror(fd)) {
			log_msg("%s saved\n", path);
		} else {
			log_file_error(path);
		}
		if(fd) fclose(fd);
	}
	free(path);
}

/**
 * Calculate and print file hash sums using printf format.
 *
 * @param out a stream to print to
 * @param filepath path to the file to calculate sums for
 * @param fullpath fullpath to the file relative to the current directory
 * @return 0 on success, -1 on fail
 */
int calculate_and_print_sums(FILE* out, const char *print_path, const char *full_path, struct rsh_stat_struct* stat_buf)
{
	struct file_info info;
	timedelta_t timer;
	int res = 0;

	memset(&info, 0, sizeof(info));
	info.full_path = rsh_strdup(full_path);
	file_info_set_print_path(&info, print_path);
	info.size = 0;

	info.sums_flags = opt.sum_flags;

	if(IS_DASH_STR(full_path)) {
		print_path = "(stdin)";
		memset(&info.stat_buf, 0, sizeof(info.stat_buf));
	} else {
		if(stat_buf != NULL) {
			memcpy(&info.stat_buf, stat_buf, sizeof(info.stat_buf));
		} else {
			if(rsh_stat(full_path, (stat_buf = &info.stat_buf)) < 0) {
				log_file_error(full_path);
				free(info.full_path);
				file_info_destroy(&info);
				return -1;
			}
		}
		if(S_ISDIR(stat_buf->st_mode)) return 0; /* don't handle directories */

		info.size = stat_buf->st_size; /* total size, in bytes */
		IF_WINDOWS(win32_set_filesize64(info.full_path, &info.size)); /* set correct filesize for large files under win32 */
	}

	/* initialize percents output */
	init_percents(&info);
	rhash_timer_start(&timer);

	if(info.sums_flags) {
		/* calculate sums */
		if(calc_sums(&info) < 0) {
			/* print error unless sharing access error occurred */
			if(errno == EACCES) return 0;
			log_file_error(full_path);
			res = -1;
		}
	}

	info.time = rhash_timer_stop(&timer);
	finish_percents(&info, res);

	if(opt.mode & MODE_TORRENT) {
		save_torrent(&info);
	}

	if(opt.flags & OPT_EMBED_CRC) {
		/* rename the file */
		rename_file_to_embed_crc32(&info);
	}

	if((opt.mode & MODE_UPDATE) && opt.fmt == FMT_SFV) {
		print_sfv_header_line(rhash_data.upd_fd, info.print_path, info.full_path);
		if(opt.flags & OPT_VERBOSE) {
			print_sfv_header_line(rhash_data.log, info.print_path, info.full_path);
			fflush(rhash_data.log);
		}
	}

	if(rhash_data.print_list && res >= 0) {
		print_line(out, rhash_data.print_list, &info);
		fflush(out);

		/* duplicate calculated line to stderr or log file if verbose */
		if( (opt.mode & MODE_UPDATE) && (opt.flags & OPT_VERBOSE) ) {
			print_line(rhash_data.log, rhash_data.print_list, &info);
			fflush(rhash_data.log);
		}

		if((opt.flags & OPT_SPEED) && info.sums_flags) {
			print_file_time_stats(&info);
		}
	}
	free(info.full_path);
	file_info_destroy(&info);
	return res;
}

/**
 * Verify hash sums of the file.
 *
 * @param info structure file path to process
 * @return zero on success, -1 on file error, -2 if hash sums are different
 */
static int verify_sums(struct file_info *info)
{
	timedelta_t timer;
	int res = 0;
	errno = 0;

	/* initialize percents output */
	init_percents(info);
	rhash_timer_start(&timer);

	if(calc_sums(info) < 0) {
		finish_percents(info, -1);
		return -1;
	}
	info->time = rhash_timer_stop(&timer);

	if((opt.flags & OPT_EMBED_CRC) && find_embedded_crc32(
		info->print_path, &info->hc.embedded_crc32_be)) {
			info->hc.flags |= HC_HAS_EMBCRC32;
			assert(info->hc.hash_mask & RHASH_CRC32);
	}

	if(!hash_check_verify(&info->hc, info->rctx)) {
		res = -2;
	}

	finish_percents(info, res);

	if((opt.flags & OPT_SPEED) && info->sums_flags) {
		print_file_time_stats(info);
	}
	return res;
}

/**
 * Check hash sums in a hash file.
 * Lines beginning with ';' and '#' are ignored.
 *
 * @param crc_file_path - the path of the file with hash sums to verify.
 * @param chdir - true if function should emulate chdir to directory of filepath before checking it.
 * @return zero on success, -1 on fail
 */
int check_hash_file(const char* crc_file_path, int chdir)
{
	FILE *fd;
	char buf[2048];
	size_t pos;
	const char *ralign;
	timedelta_t timer;
	struct file_info info;
	int res = 0, line_num = 0;
	double time;

	/* process --check-embedded option */
	if(opt.mode & MODE_CHECK_EMBEDDED) {
		unsigned crc32_be;
		if(find_embedded_crc32(crc_file_path, &crc32_be)) {
			/* initialize file_info structure */
			memset(&info, 0, sizeof(info));
			info.full_path = rsh_strdup(crc_file_path);
			file_info_set_print_path(&info, info.full_path);
			info.sums_flags = info.hc.hash_mask = RHASH_CRC32;
			info.hc.flags = HC_HAS_EMBCRC32;
			info.hc.embedded_crc32_be = crc32_be;

			res = verify_sums(&info);
			fflush(rhash_data.out);

			if(res == 0) rhash_data.ok++;
			else if(res == -1 && errno == ENOENT) rhash_data.miss++;
			rhash_data.processed++;

			free(info.full_path);
			file_info_destroy(&info);
		} else {
			log_msg("warning: file name doesn't contain a crc: %s\n", crc_file_path);
			return -1;
		}
		return 0;
	}

	/* initialize statistics */
	rhash_data.processed = rhash_data.ok = rhash_data.miss = 0;
	rhash_data.total_size = 0;

	if( IS_DASH_STR(crc_file_path) ) {
		fd = stdin;
		crc_file_path = "<stdin>";
	} else if( !(fd = rsh_fopen_bin(crc_file_path, "rb") )) {
		log_file_error(crc_file_path);
		return -1;
	}

	pos = strlen(crc_file_path)+16;
	ralign = str_set(buf, '-', (pos < 80 ? 80 - (int)pos : 2));
	fprintf(rhash_data.out, "\n--( Verifying %s )%s\n", crc_file_path, ralign);
	fflush(rhash_data.out);
	rhash_timer_start(&timer);

	/* mark the directory part of the path, by setting the pos index */
	if(chdir) {
		pos = strlen(crc_file_path);
		for(; pos > 0 && !IS_PATH_SEPARATOR(crc_file_path[pos]); pos--);
		if(IS_PATH_SEPARATOR(crc_file_path[pos])) pos++;
	} else pos = 0;

	/* read crc file line by line */
	for(line_num = 0; fgets(buf, 2048, fd); line_num++)
	{
		char* line = buf;
		char* path_without_ext = NULL;

		/* skip unicode BOM */
		if(line_num == 0 && buf[0] == (char)0xEF && buf[1] == (char)0xBB && buf[2] == (char)0xBF) line += 3;

		if(*line == 0) continue; /* skip empty lines */

		if(is_binary_string(line)) {
			fprintf(rhash_data.log, PROGRAM_NAME ": error: file is binary: %s\n", crc_file_path);
			if(fd != stdin) fclose(fd);
			return -1;
		}

		/* skip comments and empty lines */
		if(IS_COMMENT(*line) || *line == '\r' || *line == '\n') continue;

		memset(&info, 0, sizeof(info));

		if(hash_check_parse_line(line, &info.hc, !feof(fd))) {
			info.print_path = info.hc.file_path;
			info.sums_flags = info.hc.hash_mask;
		} else info.print_path = NULL;

		/* see if crc file contains a hash sum without a filename */
		if(!info.print_path && info.sums_flags) {
			char* point;
			path_without_ext = rsh_strdup(crc_file_path);
			point = strrchr(path_without_ext, '.');

			if(point) {
				*point = '\0';
				file_info_set_print_path(&info, path_without_ext);
			}
		}

		if(!info.print_path || !info.sums_flags) {
			log_msg("warning: can't parse line: %s\n", buf);
		} else {
			int is_absolute = IS_PATH_SEPARATOR(info.print_path[0]);
			IF_WINDOWS(is_absolute = is_absolute || (info.print_path[0] && info.print_path[1] == ':'));

			/* if filename shall be prepent by a directory path */
			if(pos && !is_absolute) {
				size_t len = strlen(info.print_path);
				info.full_path = (char*)rsh_malloc(pos+len+1);
				memcpy(info.full_path, crc_file_path, pos);
				strcpy(info.full_path+pos, info.print_path);
			} else {
				info.full_path = rsh_strdup(info.print_path);
			}

			/* verify hash sums of the file */
			res = verify_sums(&info);
			fflush(rhash_data.out);
			free(info.full_path);
			file_info_destroy(&info);

			/* update statistics */
			if(res == 0) rhash_data.ok++;
			else if(res == -1 && errno == ENOENT) rhash_data.miss++;
		}
		rhash_data.processed++;
		free(path_without_ext);
	}
	time = rhash_timer_stop(&timer);

	fprintf(rhash_data.out, "%s\n", str_set(buf, '-', 80));
	print_check_stats();

	if(rhash_data.processed != rhash_data.ok) rhash_data.error_flag = 1;

	if(opt.flags & OPT_SPEED && rhash_data.processed > 1) {
		print_time_stats(time, rhash_data.total_size, 1);
	}

	rhash_data.processed = 0;
	res = ferror(fd); /* check that crc file has been read without errors */
	if(fd != stdin) fclose(fd);
	return (res == 0 ? 0 : -1);
}

/**
 * Print a file info line in SFV header format.
 *
 * @param out a stream to print info to
 * @param printpath relative file path to print
 * @param fullpath a path to the file relative to the current directory.
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_sfv_header_line(FILE* out, const char* printpath, const char* fullpath)
{
	struct rsh_stat_struct stat_buf;
	uint64_t filesize;
	char buf[24];

	if( (rsh_stat(fullpath, &stat_buf)) < 0 ) {
		return -1; /* not reporting an error here */
	}
	if(S_ISDIR(stat_buf.st_mode)) return 0; /* don't handle directories */

	filesize = stat_buf.st_size; /* total size, in bytes */
	IF_WINDOWS(win32_set_filesize64(fullpath, &filesize)); /* set correct filesize for large files under win32 */

#ifdef _WIN32
	/* skip file if it can't be opened with exclusive sharing rights */
	if(!can_open_exclusive(fullpath)) {
		return 0;
	}
#endif

	sprintI64(buf, filesize, 12);
	fprintf(out, "; %s  ", buf);
	print_time(out, stat_buf.st_mtime);
	fprintf(out, " %s\n", printpath);
	return 0;
}

/**
 * Print an SFV header banner. The banner consist of 3 comment lines,
 * with the program description and current time.
 *
 * @param out a stream to print to
 */
void print_sfv_banner(FILE* out)
{
	time_t cur_time = time(NULL);
	struct tm *t = localtime(&cur_time);
	if(t) {
		fprintf(out, "; Generated by " PROGRAM_NAME " v" VERSION " on %4u-%02u-%02u at %02u:%02u.%02u\n"
			"; Written by Aleksey (Akademgorodok) - http://rhash.sourceforge.net/\n;\n",
			(1900+t->tm_year), t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	}
}
