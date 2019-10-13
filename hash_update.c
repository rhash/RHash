/* hash_update.c - functions to update a hash file */

#include "hash_update.h"
#include "calc_sums.h"
#include "file_mask.h"
#include "file_set.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "win_utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <dirent.h>
#endif

enum UpdateFlagsBits
{
	IsEmptyFile = 1,
	HasBom = 2,
	ErrorOcurred = 4
};

/* define some internal functions, implemented later in this file */
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
 * Load a set of files from the specified hash file.
 * In a case of fail, the error will be logged.
 *
 * @param set the file set to store loaded files
 * @param file the file containing hash sums to load
 * @return bit-mask containg UpdateFlagsBits on success, -1 on fail
 */
static int file_set_load_from_crc_file(file_set *set, file_t* file)
{
	int result = IsEmptyFile;
	char buf[2048];
	hash_check hc;

	FILE* fd = file_fopen(file, FOpenRead | FOpenBin);
	if (!fd) {
		/* if file does not exist, it will be created */
		if (errno == ENOENT)
			return IsEmptyFile;
		log_file_t_error(file);
		return -1;
	}
	while (!feof(fd) && fgets(buf, 2048, fd)) {
		char* line = buf;
		if ((result && IsEmptyFile) != 0) {
			/* skip unicode BOM */
			if (STARTS_WITH_UTF8_BOM(line)) {
				line += 3;
				result |= HasBom;
			}
			if (*line == 0 && feof(fd))
				break;
			result &= ~IsEmptyFile;
		}
		if (*line == 0)
			continue; /* skip empty lines */
		if (is_binary_string(line)) {
			log_file_t_msg(_("is a binary file: %s\n"), file);
			fclose(fd);
			return -1;
		}
		if (IS_COMMENT(*line) || *line == '\r' || *line == '\n')
			continue;
		/* parse a hash file line */
		if (hash_check_parse_line(line, &hc, !feof(fd))) {
			/* put file path into the file set */
			if (hc.file_path) file_set_add_name(set, hc.file_path);
		}
	}
	if (ferror(fd)) {
		log_file_t_error(file);
		result = -1;
	}
	fclose(fd);
	return result;
}

/**
 * Add hash sums of files from given file-set to a specified hash-file.
 * A specified directory path will be prepended to the path of added files,
 * if it is not a current directory.
 *
 * @param hfile the hash file to add the hash sums to
 * @param dir_path the directory path to prepend
 * @param files_to_add the set of files to hash and add
 * @return 0 on success, -1 on error
 */
static int add_sums_to_file(file_t* hfile, char* dir_path, file_set *files_to_add)
{
	FILE* fd;
	unsigned i;
	int ch;

	/* SFV banner will be printed only in SFV mode and only for empty crc files */
	int print_banner = (opt.fmt == FMT_SFV);

	hfile->size = 0;
	if (file_stat(hfile, 0) == 0) {
		if (print_banner && hfile->size > 0) print_banner = 0;
	}

	/* open the hash file for writing */
	if ( !(fd = file_fopen(hfile, FOpenRead | FOpenWrite) )) {
		log_file_t_error(hfile);
		return -1;
	}
	rhash_data.upd_fd = fd;

	if (hfile->size > 0) {
		/* read the last character of the file to check if it is EOL */
		if (fseek(fd, -1, SEEK_END) != 0) {
			log_file_t_error(hfile);
			return -1;
		}
		ch = fgetc(fd);

		/* somehow writing doesn't work without seeking */
		if (fseek(fd, 0, SEEK_END) != 0) {
			log_file_t_error(hfile);
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
		calculate_and_print_sums(fd, hfile, &file, print_path);

		file_cleanup(&file);

		if (rhash_data.stop_flags) {
			fclose(fd);
			return 0;
		}
	}
	fclose(fd);
	log_msg(_("Updated: %s\n"), hfile->path);
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

		if (res == 0 && opt.fmt == FMT_SFV && !rhash_data.stop_flags) {
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
 * In a case of fail, the error will be logged.
 *
 * @param file the hash file requiring fixing of its SFV header
 * @return 0 on success, -1 on error
 */
static int fix_sfv_header(file_t* file)
{
	FILE* in;
	FILE* out;
	char buf[2048];
	file_t new_file;
	int result = 0;
	int is_comment;
	int print_comments;
	/* open the hash file for reading */
	in = file_fopen(file, FOpenRead);
	if (!in) {
		log_file_t_error(file);
		return -1;
	}
	/* open a temporary file for writing */
	file_path_append(&new_file, file, ".new");
	out = file_fopen(&new_file, FOpenWrite);
	if (!out) {
		log_file_t_error(&new_file);
		file_cleanup(&new_file);
		fclose(in);
		return -1;
	}
	/* The first pass, prints commented lines to the destination file,
	 * and the second pass, prints all other lines */
	for (print_comments = 1;; print_comments = 0) {
		while (fgets(buf, 2048, in)) {
			char* line = buf;
			/* skip BOM, unless it is on the first line */
			if (STARTS_WITH_UTF8_BOM(line)) {
				is_comment = (line[3] == ';');
				if (ftell(out) != 0)
					line += 3;
			} else
				is_comment = (line[0] == ';');
			if (is_comment == print_comments) {
				if (fputs(line, out) < 0)
					break;
			}
		}
		if (!print_comments || ferror(out) || ferror(in) || fseek(in, 0, SEEK_SET) != 0)
			break;
	}
	if (ferror(in)) {
		log_file_t_error(file);
		result = -1;
	}
	else if (ferror(out)) {
		log_file_t_error(&new_file);
		result = -1;
	}
	fclose(in);
	if (fclose(out) < 0 && result == 0) {
		log_file_t_error(&new_file);
		result = -1;
	}
	/* overwrite the hash file with the new one */
	if (result == 0 && file_rename(&new_file, file) < 0) {
		/* TRANSLATORS: a file failed to be renamed */
		log_error(_("can't move %s to %s: %s\n"),
			new_file.path, file->path, strerror(errno));
		result = -1;
	}
	file_cleanup(&new_file);
	return result;
}
