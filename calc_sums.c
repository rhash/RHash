/* calc_sums.c - crc calculating and printing functions */

#include "platform.h" /* unlink() on unix */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* free() */
#include <errno.h>
#include <assert.h>
#ifdef _WIN32
# include <fcntl.h>  /* _O_BINARY */
# include <io.h>
#endif

#include "calc_sums.h"
#include "common_func.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "win_utils.h"
#include "librhash/rhash.h"
#include "librhash/rhash_torrent.h"

/**
 * Initialize BTIH hash function. Unlike other algorithms BTIH
 * requires more data for correct computation.
 *
 * @param info the file data
 */
static void init_btih_data(struct file_info *info)
{
	assert((info->rctx->hash_id & RHASH_BTIH) != 0);

	rhash_torrent_add_file(info->rctx, file_info_get_utf8_print_path(info), info->size);
	rhash_torrent_set_program_name(info->rctx, get_bt_program_name());

	if (opt.flags & OPT_BT_PRIVATE) {
		rhash_torrent_set_options(info->rctx, RHASH_TORRENT_OPT_PRIVATE);
	}

	if (opt.bt_announce) {
		size_t i;
		for (i = 0; i < opt.bt_announce->size; i++) {
			rhash_torrent_add_announce(info->rctx, (const char*)opt.bt_announce->array[i]);
		}
	}

	if (opt.bt_piece_length) {
		rhash_torrent_set_piece_length(info->rctx, opt.bt_piece_length);
	}
	else if (opt.bt_batch_file && rhash_data.batch_size) {
		rhash_torrent_set_batch_size(info->rctx, rhash_data.batch_size);
	}
}

/**
 * (Re)-initialize RHash context, to calculate hash sums.
 *
 * @param info the file data
 */
static void re_init_rhash_context(struct file_info *info)
{
	if (rhash_data.rctx != 0) {
		if (opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) {
			/* a set of hash sums can change from file to file */
			rhash_free(rhash_data.rctx);
			rhash_data.rctx = 0;
		} else {
			info->rctx = rhash_data.rctx;

			if (opt.bt_batch_file) {
				/* add another file to the torrent batch */
				rhash_torrent_add_file(info->rctx, file_info_get_utf8_print_path(info), info->size);
				return;
			} else {
				rhash_reset(rhash_data.rctx);
			}
		}
	}

	if (rhash_data.rctx == 0) {
		rhash_data.rctx = rhash_init(info->sums_flags);
		info->rctx = rhash_data.rctx;
	}

	if (info->sums_flags & RHASH_BTIH) {
		/* re-initialize BitTorrent data */
		init_btih_data(info);
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

	assert(info->file);
	if (info->file->mode & FILE_IFSTDIN) {
		info->print_path = "(stdin)";

#ifdef _WIN32
		/* using 0 instead of _fileno(stdin). _fileno() is undefined under 'gcc -ansi' */
		if (setmode(0, _O_BINARY) < 0) {
			return -1;
		}
#endif
	} else {
		if ((opt.mode & (MODE_CHECK | MODE_CHECK_EMBEDDED)) && FILE_ISDIR(info->file)) {
			errno = EISDIR;
			return -1;
		}

		info->size = info->file->size; /* total size, in bytes */

		if (!info->sums_flags) return 0;

		/* skip without reporting an error the files
		 * opened exclusively by another process */
		fd = file_fopen(info->file, FOpenRead | FOpenBin);
		if (!fd) return -1;
	}

	re_init_rhash_context(info);
	/* save initial msg_size, for correct calculation of percents */
	info->msg_offset = info->rctx->msg_size;

	if (percents_output->update != 0) {
		rhash_set_callback(info->rctx, (rhash_callback_t)percents_output->update, info);
	}

	/* read and hash file content */
	if ((res = rhash_file_update(info->rctx, fd)) != -1) {
		if (!opt.bt_batch_file) {
			rhash_final(info->rctx, 0); /* finalize hashing */
		}
	}
	/* calculate real file size */
	info->size = info->rctx->msg_size - info->msg_offset;
	rhash_data.total_size += info->size;

	if (fd != stdin) fclose(fd);
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
	if (opt.path_separator) {
		wrong_sep = (opt.path_separator == '/' ? '\\' : '/');
		if ((p = (char*)strchr(print_path, wrong_sep)) != NULL) {
			info->allocated_ptr = rsh_strdup(print_path);
			info->print_path = info->allocated_ptr;
			p = info->allocated_ptr + (p - print_path);

			/* replace wrong_sep in the print_path with separator defined by options */
			for (; *p; p++) {
				if (*p == wrong_sep) *p = opt.path_separator;
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
	if (info->utf8_print_path == NULL) {
		if (is_utf8()) return info->print_path;
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
static int find_embedded_crc32(const char* filepath, unsigned* crc32)
{
	const char* e = filepath + strlen(filepath) - 10;
	unsigned char raw[4];

	/* search for the sum enclosed in brackets */
	for (; e >= filepath && !IS_PATH_SEPARATOR(*e); e--) {
		if ((*e == '[' && e[9] == ']') || (*e == '(' && e[9] == ')')) {
			const char *p = e + 8;
			for (; p > e && IS_HEX(*p); p--);
			if (p == e) {
				rhash_hex_to_byte(e + 1, raw, 8);
				*crc32 = ((unsigned)raw[0] << 24) | ((unsigned)raw[1] << 16) |
					((unsigned)raw[2] << 8) | (unsigned)raw[3];
				return 1;
			}
			e -= 9;
		}
	}
	return 0;
}

/**
 * Rename given file inserting its crc32 sum enclosed into square braces
 * and placing it right before the file extension.
 *
 * @param info pointer to the data of the file to rename.
 * @return 0 on success, -1 on fail with error code in errno
 */
int rename_file_by_embeding_crc32(struct file_info *info)
{
	size_t len = strlen(info->full_path);
	const char* p = info->full_path + len;
	const char* c = p - 1;
	char* new_path;
	char* insertion_point;
	unsigned crc32;
	assert((info->rctx->hash_id & RHASH_CRC32) != 0);

	/* check if the filename contains a CRC32 sum */
	if (find_embedded_crc32(info->print_path, &crc32)) {
		/* compare with calculated CRC32 */
		if (crc32 != get_crc32(info->rctx)) {
			char crc32_str[9];
			rhash_print(crc32_str, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
			/* TRANSLATORS: sample filename with embedded CRC32: file_[A1B2C3D4].mkv */
			log_warning(_("wrong embedded CRC32, should be %s\n"), crc32_str);
		} else return 0;
	}

	/* find file extension (as the place to insert the hash sum) */
	for (; c >= info->full_path && !IS_PATH_SEPARATOR(*c); c--) {
		if (*c == '.') {
			p = c;
			break;
		}
	}

	/* now p is the point to insert delimiter + hash string in brackets */
	new_path = (char*)rsh_malloc(len + 12);
	insertion_point = new_path + (p - info->full_path);
	memcpy(new_path, info->full_path, p - info->full_path);
	if (opt.embed_crc_delimiter && *opt.embed_crc_delimiter) *(insertion_point++) = *opt.embed_crc_delimiter;
	rhash_print(insertion_point+1, info->rctx, RHASH_CRC32, RHPR_UPPERCASE);
	insertion_point[0] = '[';
	insertion_point[9] = ']'; /* ']' overrides '\0' inserted by rhash_print_sum() */
	strcpy(insertion_point + 10, p); /* append file extension */

	/* rename the file */
	if (rename(info->full_path, new_path) < 0) {
		log_error(_("can't move %s to %s: %s\n"), info->full_path, new_path,
			strerror(errno));
		free(new_path);
		return -1;
	}

	/* change file name in the file info structure */
	if (info->print_path >= info->full_path && info->print_path < p) {
		file_info_set_print_path(info, new_path + len - strlen(info->print_path));
	} else {
		file_info_set_print_path(info, new_path);
	}

	free(info->full_path);
	info->full_path = new_path;
	return 0;
}

/**
 * Save torrent file to the given path.
 *
 * @param torrent_file the path to save torrent file to
 * @param rctx the context containing torrent data
 * @return 0 on success, -1 on fail with error code in errno
 */
int save_torrent_to(file_t* torrent_file, rhash_context* rctx)
{
	FILE* fd;
	int res = 0;

	const rhash_str* text = rhash_torrent_generate_content(rctx);
	if (!text) {
		errno = ENOMEM;
		log_file_t_error(torrent_file);
		return -1;
	}
	
	/* make backup copy of the existing torrent file */
	file_move_to_bak(torrent_file);

	/* write the torrent file */
	fd = file_fopen(torrent_file, FOpenWrite | FOpenBin);
	if (fd && text->length == fwrite(text->str, 1, text->length, fd) &&
		!ferror(fd) && !fflush(fd))
	{
		log_msg(_("%s saved\n"), file_cpath(torrent_file));
	} else {
		log_file_t_error(torrent_file);
		res = -1;
	}
	if (fd) fclose(fd);
	return res;
}

/**
 * Save torrent file.
 *
 * @param info information about the hashed file
 */
static void save_torrent(struct file_info* info)
{
	/* append .torrent extension to the file path */
	file_t torrent_file;
	file_path_append(&torrent_file, info->file, ".torrent");
	save_torrent_to(&torrent_file, info->rctx);	
	file_cleanup(&torrent_file);
}

/**
 * Calculate and print file hash sums using printf format.
 *
 * @param out a stream to print to
 * @param file the file to calculate sums for
 * @param print_path the path to print
 * @return 0 on success, -1 on fail
 */
int calculate_and_print_sums(FILE* out, file_t* file, const char *print_path)
{
	struct file_info info;
	timedelta_t timer;
	int res = 0;

	memset(&info, 0, sizeof(info));
	info.file = file;
	info.full_path = rsh_strdup(file->path);
	file_info_set_print_path(&info, print_path);
	info.size = 0;

	info.sums_flags = opt.sum_flags;

	if (file->mode & FILE_IFSTDIN) {
		print_path = "(stdin)";
	} else {
		if (file->mode & FILE_IFDIR) return 0; /* don't handle directories */
		info.size = file->size; /* total size, in bytes */
	}

	/* initialize percents output */
	init_percents(&info);
	rsh_timer_start(&timer);

	if (info.sums_flags) {
		/* calculate sums */
		if (calc_sums(&info) < 0) {
			/* print i/o error */
			log_file_t_error(file);
			res = -1;
		}
		if (rhash_data.interrupted) {
			report_interrupted();
			return 0;
		}
	}

	info.time = rsh_timer_stop(&timer);
	finish_percents(&info, res);

	if (opt.flags & OPT_EMBED_CRC) {
		/* rename the file */
		rename_file_by_embeding_crc32(&info);
	}

	if ((opt.mode & MODE_TORRENT) && !opt.bt_batch_file) {
		save_torrent(&info);
	}

	if ((opt.mode & MODE_UPDATE) && opt.fmt == FMT_SFV) {
		/* updating SFV file: print SFV header line */
		print_sfv_header_line(rhash_data.upd_fd, file, 0);
		if (opt.flags & OPT_VERBOSE) {
			print_sfv_header_line(rhash_data.log, file, 0);
			fflush(rhash_data.log);
		}
		file_cleanup(file);
	}

	if (rhash_data.print_list && res >= 0) {
		if (!opt.bt_batch_file) {
			print_line(out, rhash_data.print_list, &info);

			/* print calculated line to stderr or log-file if verbose */
			if ((opt.mode & MODE_UPDATE) && (opt.flags & OPT_VERBOSE)) {
				print_line(rhash_data.log, rhash_data.print_list, &info);
			}
		}

		if ((opt.flags & OPT_SPEED) && info.sums_flags) {
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
	rsh_timer_start(&timer);

	if (calc_sums(info) < 0) {
		finish_percents(info, -1);
		return -1;
	}
	info->time = rsh_timer_stop(&timer);

	if (rhash_data.interrupted) {
		report_interrupted();
		return 0;
	}

	if ((opt.flags & OPT_EMBED_CRC) && find_embedded_crc32(
		info->print_path, &info->hc.embedded_crc32)) {
			info->hc.flags |= HC_HAS_EMBCRC32;
			assert(info->hc.hash_mask & RHASH_CRC32);
	}

	if (!hash_check_verify(&info->hc, info->rctx)) {
		res = -2;
	}

	finish_percents(info, res);

	if ((opt.flags & OPT_SPEED) && info->sums_flags) {
		print_file_time_stats(info);
	}
	return res;
}

/**
 * Check hash sums in a hash file.
 * Lines beginning with ';' and '#' are ignored.
 *
 * @param hash_file_path - the path of the file with hash sums to verify.
 * @param chdir - true if function should emulate chdir to directory of filepath before checking it.
 * @return zero on success, -1 on fail
 */
int check_hash_file(file_t* file, int chdir)
{
	FILE *fd;
	char buf[4096];
	size_t pos;
	const char *ralign;
	timedelta_t timer;
	struct file_info info;
	const char* hash_file_path = file->path;
	int res = 0, line_num = 0;
	double time;

	/* process --check-embedded option */
	if (opt.mode & MODE_CHECK_EMBEDDED) {
		unsigned crc32;
		if (find_embedded_crc32(hash_file_path, &crc32)) {
			/* initialize file_info structure */
			memset(&info, 0, sizeof(info));
			info.full_path = rsh_strdup(hash_file_path);
			info.file = file;
			file_info_set_print_path(&info, info.full_path);
			info.sums_flags = info.hc.hash_mask = RHASH_CRC32;
			info.hc.flags = HC_HAS_EMBCRC32;
			info.hc.embedded_crc32 = crc32;

			res = verify_sums(&info);
			fflush(rhash_data.out);
			if (!rhash_data.interrupted) {
				if (res == 0) rhash_data.ok++;
				else if (res == -1 && errno == ENOENT) rhash_data.miss++;
				rhash_data.processed++;
			}

			free(info.full_path);
			file_info_destroy(&info);
		} else {
			log_warning(_("file name doesn't contain a CRC32: %s\n"), hash_file_path);
			return -1;
		}
		return 0;
	}

	/* initialize statistics */
	rhash_data.processed = rhash_data.ok = rhash_data.miss = 0;
	rhash_data.total_size = 0;

	if (file->mode & FILE_IFSTDIN) {
		fd = stdin;
		hash_file_path = "<stdin>";
	} else if ( !(fd = file_fopen(file, FOpenRead | FOpenBin) )) {
		log_file_error(hash_file_path);
		return -1;
	}

	pos = strlen(hash_file_path)+16;
	ralign = str_set(buf, '-', (pos < 80 ? 80 - (int)pos : 2));
	rsh_fprintf(rhash_data.out, _("\n--( Verifying %s )%s\n"), hash_file_path, ralign);
	fflush(rhash_data.out);
	rsh_timer_start(&timer);

	/* mark the directory part of the path, by setting the pos index */
	if (chdir) {
		pos = strlen(hash_file_path);
		for (; pos > 0 && !IS_PATH_SEPARATOR(hash_file_path[pos]); pos--);
		if (IS_PATH_SEPARATOR(hash_file_path[pos])) pos++;
	} else pos = 0;

	/* read crc file line by line */
	for (line_num = 0; fgets(buf, sizeof(buf), fd); line_num++) {
		char* line = buf;
		char* path_without_ext = NULL;

		/* skip unicode BOM */
		if (line_num == 0 && buf[0] == (char)0xEF && buf[1] == (char)0xBB && buf[2] == (char)0xBF) line += 3;

		if (*line == 0) continue; /* skip empty lines */

		if (is_binary_string(line)) {
			log_error(_("file is binary: %s\n"), hash_file_path);
			if (fd != stdin) fclose(fd);
			return -1;
		}

		/* skip comments and empty lines */
		if (IS_COMMENT(*line) || *line == '\r' || *line == '\n') continue;

		memset(&info, 0, sizeof(info));

		if (!hash_check_parse_line(line, &info.hc, !feof(fd))) continue;
		if (info.hc.hash_mask == 0) continue;

		info.print_path = info.hc.file_path;
		info.sums_flags = info.hc.hash_mask;

		/* see if crc file contains a hash sum without a filename */
		if (info.print_path == NULL) {
			char* point;
			path_without_ext = rsh_strdup(hash_file_path);
			point = strrchr(path_without_ext, '.');

			if (point) {
				*point = '\0';
				file_info_set_print_path(&info, path_without_ext);
			}
		}

		if (info.print_path != NULL) {
			file_t file_to_check;
			int is_absolute = IS_PATH_SEPARATOR(info.print_path[0]);
			IF_WINDOWS(is_absolute = is_absolute || (info.print_path[0] && info.print_path[1] == ':'));

			/* if filename shall be prepended by a directory path */
			if (pos && !is_absolute) {
				size_t len = strlen(info.print_path);
				info.full_path = (char*)rsh_malloc(pos + len + 1);
				memcpy(info.full_path, hash_file_path, pos);
				strcpy(info.full_path + pos, info.print_path);
			} else {
				info.full_path = rsh_strdup(info.print_path);
			}
			memset(&file_to_check, 0, sizeof(file_t));
			file_to_check.path = info.full_path;
			file_stat(&file_to_check, 0);
			info.file = &file_to_check;

			/* verify hash sums of the file */
			res = verify_sums(&info);

			fflush(rhash_data.out);
			file_cleanup(&file_to_check);
			file_info_destroy(&info);

			if (rhash_data.interrupted) {
				free(path_without_ext);
				break;
			}

			/* update statistics */
			if (res == 0) rhash_data.ok++;
			else if (res == -1 && errno == ENOENT) rhash_data.miss++;
			rhash_data.processed++;
		}
		free(path_without_ext);
	}
	time = rsh_timer_stop(&timer);

	rsh_fprintf(rhash_data.out, "%s\n", str_set(buf, '-', 80));
	print_check_stats();

	if (rhash_data.processed != rhash_data.ok) rhash_data.error_flag = 1;

	if (opt.flags & OPT_SPEED && rhash_data.processed > 1) {
		print_time_stats(time, rhash_data.total_size, 1);
	}

	rhash_data.processed = 0;
	res = ferror(fd); /* check that crc file has been read without errors */
	if (fd != stdin) fclose(fd);
	return (res == 0 ? 0 : -1);
}

/*=========================================================================
 * Benchmark functions
 *=========================================================================*/

/**
 * Hash a repeated message chunk by specified hash function.
 *
 * @param hash_id hash function identifier
 * @param message a message chunk to hash
 * @param msg_size message chunk size
 * @param count number of chunks
 * @param out computed hash
 * @return 1 on success, 0 on error
 */
static int benchmark_loop(unsigned hash_id, const unsigned char* message, size_t msg_size, int count, unsigned char* out)
{
	int i;
	struct rhash_context *context = rhash_init(hash_id);
	if (!context) return 0;

	/* process the repeated message buffer */
	for (i = 0; i < count && !rhash_data.interrupted; i++) {
		rhash_update(context, message, msg_size);
	}
	rhash_final(context, out);
	rhash_free(context);
	return 1;
}

#if defined(_MSC_VER)
#define ALIGN_DATA(n) __declspec(align(n))
#elif defined(__GNUC__)
#define ALIGN_DATA(n) __attribute__((aligned (n)))
#else
#define ALIGN_DATA(n) /* do nothing */
#endif

/* define read_tsc() if possible */
#if defined(__i386__) || defined(_M_IX86) || \
	defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)

#if defined( _MSC_VER ) /* if MS VC */
# include <intrin.h>
# pragma intrinsic( __rdtsc )
# define read_tsc() __rdtsc()
# define HAVE_TSC
#elif defined( __GNUC__ ) /* if GCC */
static uint64_t read_tsc(void) {
	unsigned long lo, hi;
	__asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return (((uint64_t)hi) << 32) + lo;
}
# define HAVE_TSC
#endif /* _MSC_VER, __GNUC__ */
#endif /* x86/amd64 arch */

void run_benchmark(unsigned hash_id, unsigned flags)
{
	unsigned char ALIGN_DATA(64) message[8192]; /* 8 KiB */
	timedelta_t timer;
	int i, j;
	size_t sz_mb, msg_size;
	double time, total_time = 0;
	const int rounds = 4;
	const char* hash_name;
	unsigned char out[130];
#ifdef HAVE_TSC
	double cpb = 0;
#endif /* HAVE_TSC */

#ifdef _WIN32
	set_benchmark_cpu_affinity(); /* set CPU affinity to improve test results */
#endif

	/* set message size for fast and slow hash functions */
	msg_size = 1073741824 / 2;
	if (hash_id & (RHASH_WHIRLPOOL | RHASH_SNEFRU128 | RHASH_SNEFRU256 | RHASH_SHA3_224 | RHASH_SHA3_256 | RHASH_SHA3_384 | RHASH_SHA3_512)) {
		msg_size /= 8;
	} else if (hash_id & (RHASH_GOST | RHASH_GOST_CRYPTOPRO | RHASH_SHA384 | RHASH_SHA512)) {
		msg_size /= 2;
	}
	sz_mb = msg_size / (1 << 20); /* size in MiB */
	hash_name = rhash_get_name(hash_id);
	if (!hash_name) hash_name = ""; /* benchmarking several hashes*/

	for (i = 0; i < (int)sizeof(message); i++) message[i] = i & 0xff;

	for (j = 0; j < rounds && !rhash_data.interrupted; j++) {
		rsh_timer_start(&timer);
		benchmark_loop(hash_id, message, sizeof(message), (int)(msg_size / sizeof(message)), out);

		time = rsh_timer_stop(&timer);
		total_time += time;

		if ((flags & BENCHMARK_RAW) == 0 && !rhash_data.interrupted) {
			rsh_fprintf(rhash_data.out, "%s %u MiB calculated in %.3f sec, %.3f MBps\n", hash_name, (unsigned)sz_mb, time, (double)sz_mb / time);
			fflush(rhash_data.out);
		}
	}

#if defined(HAVE_TSC)
	/* measure the CPU "clocks per byte" speed */
	if ((flags & BENCHMARK_CPB) != 0 && !rhash_data.interrupted) {
		unsigned int c1 = -1, c2 = -1;
		unsigned volatile long long cy0, cy1, cy2;
		int msg_size = 128 * 1024;

		/* make 200 tries */
		for (i = 0; i < 200; i++) {
			cy0 = read_tsc();
			benchmark_loop(hash_id, message, sizeof(message), msg_size / sizeof(message), out);
			cy1 = read_tsc();
			benchmark_loop(hash_id, message, sizeof(message), msg_size / sizeof(message), out);
			benchmark_loop(hash_id, message, sizeof(message), msg_size / sizeof(message), out);
			cy2 = read_tsc();

			cy2 -= cy1;
			cy1 -= cy0;
			c1 = (unsigned int)(c1 > cy1 ? cy1 : c1);
			c2 = (unsigned int)(c2 > cy2 ? cy2 : c2);
		}
		cpb = ((c2 - c1) + 1) / (double)msg_size;
	}
#endif /* HAVE_TSC */

	if (rhash_data.interrupted) {
		report_interrupted();
		return;
	}

	if (flags & BENCHMARK_RAW) {
		/* output result in a "raw" machine-readable format */
		rsh_fprintf(rhash_data.out, "%s\t%u\t%.3f\t%.3f", hash_name, ((unsigned)sz_mb * rounds), total_time, (double)(sz_mb * rounds) / total_time);
#if defined(HAVE_TSC)
		if (flags & BENCHMARK_CPB) {
			rsh_fprintf(rhash_data.out, "\t%.2f", cpb);
		}
#endif /* HAVE_TSC */
		rsh_fprintf(rhash_data.out, "\n");
	} else {
		rsh_fprintf(rhash_data.out, "%s %u MiB total in %.3f sec, %.3f MBps", hash_name, ((unsigned)sz_mb * rounds), total_time, (double)(sz_mb * rounds) / total_time);
#if defined(HAVE_TSC)
		if (flags & BENCHMARK_CPB) {
			rsh_fprintf(rhash_data.out, ", CPB=%.2f", cpb);
		}
#endif /* HAVE_TSC */
		rsh_fprintf(rhash_data.out, "\n");
	}
}
