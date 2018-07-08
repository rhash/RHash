/* hash_update.c - functions to update a crc file */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <dirent.h>
#endif

#include "calc_sums.h"
#include "common_func.h"
#include "file.h"
#include "file_set.h"
#include "file_mask.h"
#include "hash_print.h"
#include "hash_update.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "win_utils.h"

/* first define some internal functions, implemented later in this file */
static int add_new_crc_entries(file_t* file, file_set *crc_entries);
static int file_set_load_from_crc_file(file_set *set, file_t* file);
static int fix_sfv_header(file_t* file);

/**
 * Update given crc file, by adding to it hashes of files from the same
 * directory, but which the crc file doesn't contain yet.
 *
 * @param file the file containing hash sums
 * @return 0 on success, -1 on fail
 */
int update_hash_file(file_t* file)
{
	file_set* crc_entries;
	timedelta_t timer;
	int res;

	if (opt.flags & OPT_VERBOSE) {
		log_msg(_("Updating: %s\n"), file->path);
	}

	crc_entries = file_set_new();
	res = file_set_load_from_crc_file(crc_entries, file);

	if (opt.flags & OPT_SPEED) rsh_timer_start(&timer);
	rhash_data.total_size = 0;
	rhash_data.processed  = 0;

	if (res == 0) {
		/* add the crc file itself to the set of excluded from re-calculation files */
		file_set_add_name(crc_entries, get_basename(file->path));
		file_set_sort(crc_entries);

		/* update crc file with sums of files not present in the crc_entries */
		res = add_new_crc_entries(file, crc_entries);
	}
	file_set_free(crc_entries);

	if (opt.flags & OPT_SPEED && rhash_data.processed > 0) {
		double time = rsh_timer_stop(&timer);
		print_time_stats(time, rhash_data.total_size, 1);
	}

	return res;
}

/**
 * Load a set of files from given crc file.
 *
 * @param set the file set to store loaded files
 * @param file the file containing hash sums to load
 * @return 0 on success, -1 on fail with error code in errno
 */
static int file_set_load_from_crc_file(file_set *set, file_t* file)
{
	FILE *fd;
	int line_num;
	char buf[2048];
	hash_check hc;

	if ( !(fd = file_fopen(file, FOpenRead | FOpenBin) )) {
		/* if file not exist, it will be created */
		return (errno == ENOENT ? 0 : -1);
	}
	for (line_num = 0; fgets(buf, 2048, fd); line_num++) {
		char* line = buf;

		/* skip unicode BOM */
		if (line_num == 0 && buf[0] == (char)0xEF && buf[1] == (char)0xBB && buf[2] == (char)0xBF) line += 3;

		if (*line == 0) continue; /* skip empty lines */

		if (is_binary_string(line)) {
			log_error(_("skipping binary file %s\n"), file->path);
			fclose(fd);
			return -1;
		}

		if (IS_COMMENT(*line) || *line == '\r' || *line == '\n') continue;

		/* parse a hash file line */
		if (hash_check_parse_line(line, &hc, !feof(fd))) {
			/* store file info to the file set */
			if (hc.file_path) file_set_add_name(set, hc.file_path);
		}
	}
	fclose(fd);
	return 0;
}

/**
 * Add hash sums of files from given file-set to a specified hash-file.
 * A specified directory path will be prepended to the path of added files,
 * if it is not a current directory.
 *
 * @param file the hash file to add the hash sums to
 * @param dir_path the directory path to prepend
 * @param files_to_add the set of files to hash and add
 * @return 0 on success, -1 on error
 */
static int add_sums_to_file(file_t* file, char* dir_path, file_set *files_to_add)
{
	FILE* fd;
	unsigned i;
	int ch;

	/* SFV banner will be printed only in SFV mode and only for empty crc files */
	int print_banner = (opt.fmt == FMT_SFV);

	file->size = 0;
	if (file_stat(file, 0) == 0) {
		if (print_banner && file->size > 0) print_banner = 0;
	}

	/* open the hash file for writing */
	if ( !(fd = file_fopen(file, FOpenRead | FOpenWrite) )) {
		log_file_t_error(file);
		return -1;
	}
	rhash_data.upd_fd = fd;

	if (file->size > 0) {
		/* read the last character of the file to check if it is EOL */
		if (fseek(fd, -1, SEEK_END) != 0) {
			log_file_t_error(file);
			return -1;
		}
		ch = fgetc(fd);

		/* somehow writing doesn't work without seeking */
		if (fseek(fd, 0, SEEK_END) != 0) {
			log_file_t_error(file);
			return -1;
		}

		/* write EOL if it wasn't present */
		if (ch != '\n' && ch != '\r') {
			/* fputc('\n', fd); */
			rsh_fprintf(fd, "\n");
		}
	}

	/* append hash sums to the updated crc file */
	for (i = 0; i < files_to_add->size; i++, rhash_data.processed++) {
		file_t file;
		char *print_path = file_set_get(files_to_add, i)->filepath;
		memset(&file, 0, sizeof(file));

		if (dir_path[0] != '.' || dir_path[1] != 0) {
			/* prepend the file path by directory path */
			file_init(&file, make_path(dir_path, print_path), 0);
		} else {
			file_init(&file, print_path, FILE_OPT_DONT_FREE_PATH);
		}

		if (opt.fmt == FMT_SFV) {
			if (print_banner) {
				print_sfv_banner(fd);
				print_banner = 0;
			}
		}
		file_stat(&file, 0);

		/* print hash sums to the crc file */
		calculate_and_print_sums(fd, &file, print_path);

		file_cleanup(&file);

		if (rhash_data.interrupted) {
			fclose(fd);
			return 0;
		}
	}
	fclose(fd);
	log_msg(_("Updated: %s\n"), file->path);
	return 0;
}

/**
 * Read a directory and load files not present in the crc_entries file-set
 * into the files_to_add file-set.
 *
 * @param dir_path the path of the directory to load files from
 * @param crc_entries file-set of files which should be skipped
 * @param files_to_add file-set to load the list of files into
 * @return 0 on success, -1 on error (and errno is set)
 */
static int load_filtered_dir(const char* dir_path, file_set *crc_entries, file_set *files_to_add)
{
	DIR *dp;
	struct dirent *de;

	/* read directory */
	dp = opendir(dir_path);
	if (!dp) return -1;

	while ((de = readdir(dp)) != NULL) {
		char *path;
		unsigned is_regular;

		/* skip "." and ".." directories */
		if (de->d_name[0] == '.' && (de->d_name[1] == 0 ||
				(de->d_name[1] == '.' && de->d_name[2] == 0))) {
					continue;
		}

		/* check if the file is a regular one */
		path = make_path(dir_path, de->d_name);
		is_regular = is_regular_file(path);
		free(path);

		/* skip non-regular files (directories, device files, e.t.c.),
		 * as well as files not accepted by current file filter
		 * and files already present in the crc_entries file set */
		if (!is_regular || !file_mask_match(opt.files_accept, de->d_name) ||
			(opt.files_exclude && file_mask_match(opt.files_exclude, de->d_name)) ||
			file_set_exist(crc_entries, de->d_name))
		{
			continue;
		}

		file_set_add_name(files_to_add, de->d_name);
	}
	return 0;
}

/**
 * Calculate and add to the given hash-file the hash-sums for all files
 * from the same directory as the hash-file, but absent from given
 * crc_entries file-set.
 *
 * <p/>If SFV format was specified by a command line switch, the after adding
 * hash sums SFV header of the file is fixed by moving all lines starting
 * with a semicolon before other lines. So an SFV-formatted hash-file
 * will remain correct.
 *
 * @param file the hash-file to add sums into
 * @param crc_entries file-set of files to omit from adding
 * @return 0 on success, -1 on error
 */
static int add_new_crc_entries(file_t* file, file_set *crc_entries)
{
	file_set* files_to_add;
	char* dir_path;
	int res = 0;

	dir_path = get_dirname(file->path);
	files_to_add = file_set_new();

	/* load into files_to_add files from directory not present in the crc_entries */
	load_filtered_dir(dir_path, crc_entries, files_to_add);

	if (files_to_add->size > 0) {
		/* sort files by path */
		file_set_sort_by_path(files_to_add);

		/* calculate and write crc sums to the file */
		res = add_sums_to_file(file, dir_path, files_to_add);

		if (res == 0 && opt.fmt == FMT_SFV && !rhash_data.interrupted) {
			/* move SFV header from the end of updated file to its head */
			res = fix_sfv_header(file);
		}
	}

	file_set_free(files_to_add);
	free(dir_path);
	return res;
}

/**
 * Move all SFV header lines (i.e. all lines starting with a semicolon)
 * from the end of updated file to its head.
 *
 * @param file the hash file requiring fixing of its SFV header
 */
static int fix_sfv_header(file_t* file)
{
	FILE* in;
	FILE* out;
	char line[2048];
	file_t new_file;
	int err = 0;

	if ( !(in = file_fopen(file, FOpenRead))) {
		log_file_t_error(file);
		return -1;
	}

	/* open a temporary file for writing */
	file_path_append(&new_file, file, ".new");
	if ( !(out = file_fopen(&new_file, FOpenWrite) )) {
		log_file_t_error(&new_file);
		file_cleanup(&new_file);
		fclose(in);
		return -1;
	}

	/* The first, output all commented lines to the file header */
	while (fgets(line, 2048, in)) {
		if (*line == ';') {
			if (fputs(line, out) < 0) break;
		}
	}
	if (!ferror(out) && !ferror(in)) {
		fseek(in, 0, SEEK_SET);
		/* The second, output non-commented lines */
		while (fgets(line, 2048, in)) {
			if (*line != ';') {
				if (fputs(line, out) < 0) break;
			}
		}
	}
	if (ferror(in)) {
		log_file_t_error(file);
		err = 1;
	}
	if (ferror(out)) {
		log_file_t_error(&new_file);
		err = 1;
	}

	fclose(in);
	fclose(out);

	/* overwrite the hash file with a new one */
	if (!err && file_rename(&new_file, file) < 0) {
		log_error(_("can't move %s to %s: %s\n"),
			new_file.path, file->path, strerror(errno));
	}
	file_cleanup(&new_file);
	return (err ? -1 : 0);
}
