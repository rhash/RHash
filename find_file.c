/* find_file.c - functions for recursive scan of directories. */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "find_file.h"
#include "common_func.h"
#include "output.h"
#include "win_utils.h"

#ifdef _WIN32
# include <windows.h>
#else
# include <dirent.h>    /* opendir/readdir */
#endif

#define IS_DASH_STR(s) ((s)[0] == '-' && (s)[1] == '\0')
#define IS_CURRENT_OR_PARENT_DIR(s) ((s)[0]=='.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))
#define IS_CURRENT_OR_PARENT_DIRW(s) ((s)[0]==L'.' && (!(s)[1] || ((s)[1] == L'.' && !(s)[2])))

#define RF_BLOCK_SIZE 256
#define add_root_file(data, file) rsh_blocks_vector_add(&(data)->root_files, (file), RF_BLOCK_SIZE, sizeof(file_t))
#define get_root_file(data, index) rsh_blocks_vector_get_item(&(data)->root_files, (index), RF_BLOCK_SIZE, file_t)

static int dir_scan(file_t* start_dir, file_search_data* data);

/* allocate and fill the file_search_data */
file_search_data* file_search_data_new(void)
{
	file_search_data* data = (file_search_data*)rsh_malloc(sizeof(file_search_data));
	memset(data, 0, sizeof(file_search_data));
	rsh_blocks_vector_init(&data->root_files);
	data->max_depth = -1;
	return data;
}

void file_search_add_file(file_search_data* data, tstr_t path, int is_file_list)
{
#ifdef _WIN32
	/* expand wildcards and fill the root_files */
	int added = 0;
	size_t length, index;
	wchar_t* p = wcschr(path, L'\0') - 1;

	/* strip trailing '\','/' symbols (if not preceded by ':') */
	for (; p > path && IS_PATH_SEPARATOR_W(*p) && p[-1] != L':'; p--) *p = 0;

	/* Expand a wildcard in the current file path and store results into data->root_files.
	 * If a wildcard is not found then just the file path is stored.
	 * NB, only wildcards in the last filename of the path are expanded. */

	length = p - path + 1;
	index = wcscspn(path, L"*?");

	if (index < length && wcscspn(path + index, L"/\\") >= (length - index))
	{
		/* a wildcard is found without a directory separator after it */
		wchar_t* parent;
		WIN32_FIND_DATAW d;
		HANDLE handle;

		/* find a directory separator before the file name */
		for (; index > 0 && !IS_PATH_SEPARATOR(path[index]); index--);
		parent = (IS_PATH_SEPARATOR(path[index]) ? path : 0);

		handle = FindFirstFileW(path, &d);
		if (INVALID_HANDLE_VALUE != handle)
		{
			do {
				file_t file;
				int failed;
				if (IS_CURRENT_OR_PARENT_DIRW(d.cFileName)) continue;

				memset(&file, 0, sizeof(file));
				file.wpath = make_pathw(parent, index + 1, d.cFileName);
				if (!file.wpath) continue;

				/* skip directories if not in recursive mode */
				if (data->max_depth == 0 && (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

				/* convert file name */
				file.path = wchar_to_cstr(file.wpath, WIN_DEFAULT_ENCODING, &failed);
				if (!failed) {
					failed = (file_stat(&file, 0) < 0);
				}

				/* quietly skip unconvertible file names */
				if (!file.path || failed) {
					if (failed) {
						data->errors_count++;
					}
					free(file.path);
					free(file.wpath);
					continue;
				}

				/* fill the file information */
				file.mode |= FILE_IFROOT;
				add_root_file(data, &file);
				added++;
			} while (FindNextFileW(handle, &d));
			FindClose(handle);
		} else {
			/* report error on the specified wildcard */
			char * cpath = wchar_to_cstr(path, WIN_DEFAULT_ENCODING, NULL);
			set_errno_from_last_file_error();
			log_file_error(cpath);
			free(cpath);
			data->errors_count++;
		}
	}
	else
	{
		int failed;
		file_t file;
		memset(&file, 0, sizeof(file));

		/* if filepath is a dash string "-" */
		if ((path[0] == L'-' && path[1] == L'\0'))
		{
			file.mode = FILE_IFSTDIN;
			file.path = rsh_strdup("(stdin)");
		} else {
			file.path = wchar_to_cstr(path, WIN_DEFAULT_ENCODING, &failed);
			if (failed) {
				log_error(_("can't convert the file path to local encoding: %s\n"), file.path);
				free(file.path);
				data->errors_count++;
				return;
			}
			file.wpath = path;
			if (file_stat(&file, 0) < 0) {
				log_file_t_error(&file);
				free(file.path);
				data->errors_count++;
				return;
			}
		}

		/* mark the file as obtained from the command line */
		file.mode |= FILE_IFROOT;
		if (is_file_list) file.mode |= FILE_IFLIST;
		file.wpath = rsh_wcsdup(path);
		add_root_file(data, &file);
	}
#else
	/* copy file path */
	file_t file;
	file_init(&file, path, 0);

	if (IS_DASH_STR(file.path))
	{
		file.mode = FILE_IFSTDIN;
	}
	else if (file_stat(&file, FUseLstat) < 0) {
		log_file_t_error(&file);
		file_cleanup(&file);
		data->errors_count++;
		return;
	}

	file.mode |= FILE_IFROOT;
	if (is_file_list) file.mode |= FILE_IFLIST;
	add_root_file(data, &file);
#endif /* _WIN32 */
}

/**
 * Free memory allocated by the file_search_data structure
 */
void file_search_data_free(file_search_data* data)
{
	if (data)
	{
		size_t i;
		/* clean the memory allocated by file_t elements */
		for (i = 0; i < data->root_files.size; i++)
		{
			file_t* file = get_root_file(data, i);
			file_cleanup(file);
		}
		rsh_blocks_vector_destroy(&data->root_files);
		free(data);
	}
}

void scan_files(file_search_data* data)
{
	size_t i;
	size_t count = data->root_files.size;
	int skip_symlinked_dirs = (data->options & FIND_FOLLOW_SYMLINKS ? 0 : FUseLstat);

	for (i = 0; i < count && !(data->options & FIND_CANCEL); i++)
	{
		file_t* file = get_root_file(data, i);
		assert(!!(file->mode & FILE_IFROOT));

		/* check if file is a directory */
		if (!!(file->mode & FILE_IFLIST)) {
			file_list_t list;
			if (file_list_open(&list, file) < 0) {
				log_file_t_error(file);
				continue;
			}
			while (file_list_read(&list)) {
				data->call_back(&list.current_file, data->call_back_data);
			}
			file_list_close(&list);
		} else if (FILE_ISDIR(file)) {
			/* silently skip symlinks to directories if required */
			if (skip_symlinked_dirs && FILE_ISLNK(file)) {
				continue;
			}

			if (data->max_depth != 0) {
				dir_scan(file, data);
			} else if ((data->options & FIND_LOG_ERRORS) != 0) {
				errno = EISDIR;
				log_file_t_error(file);
			}
		} else {
			/* process a regular file or a dash '-' path */
			data->call_back(file, data->call_back_data);
		}
	}
}

/**
 * An entry of a list containing content of a directory.
 */
typedef struct dir_entry
{
	struct dir_entry *next;
	char* filename;
	unsigned type; /* a directory, symlink, etc. */
} dir_entry;

/**
 * Allocate and initialize a dir_entry.
 *
 * @param next next dir_entry in list
 * @param filename a filename to store in the dir_entry
 * @param type type of dir_entry
 * @return allocated dir_entry
 */
static dir_entry* dir_entry_new(dir_entry *next, char* filename, unsigned type)
{
	dir_entry* e = (dir_entry*)malloc(sizeof(dir_entry));
	if (!e) return NULL;
	if (filename) {
		e->filename = rsh_strdup(filename);
		if (!e->filename) {
			free(e);
			return NULL;
		}
	} else {
		e->filename = NULL;
	}
	e->next = next;
	e->type = type;
	return e;
}

/**
 * Insert a dir_entry with given filename and type in list.
 *
 * @param at the position before which the entry will be inserted
 * @param filename file name
 * @param type file type
 * @return pointer to the inserted dir_entry
 */
static dir_entry* dir_entry_insert(dir_entry **at, char* filename, unsigned type)
{
	dir_entry* e = dir_entry_new(*at, filename, type);
	if (e) *at = e;
	return e;
}

/**
 * Free the first entry of the list of dir_entry elements.
 *
 * @param p pointer to the list.
 */
static void dir_entry_drop_head(dir_entry** p)
{
	dir_entry* e = *p;
	*p = e->next;
	free(e->filename);
	free(e);
}

/**
 * Directory iterator.
 */
typedef struct dir_iterator
{
	int count;
	char* dir_path;
} dir_iterator;
#define MAX_DIRS_DEPTH 64

/**
 * Walk directory tree and call given callback function to process each file/directory.
 *
 * @param start_dir path to the directory to walk recursively
 * @param data the options specifying how to walk the directory tree
 * @return 0 on success, -1 on error
 */
static int dir_scan(file_t* start_dir, file_search_data* data)
{
	dir_entry *dirs_stack = NULL; /* root of the dir_list */
	dir_iterator* it;
	int level = 0;
	int max_depth = data->max_depth;
	int options = data->options;
	int fstat_flags = (data->options & FIND_FOLLOW_SYMLINKS ? 0 : FUseLstat);
	file_t file;

	if (max_depth < 0 || max_depth >= MAX_DIRS_DEPTH) {
		max_depth = MAX_DIRS_DEPTH - 1;
	}

	/* skip the directory if max_depth == 0 */
	if (!max_depth) return 0;

	if (!FILE_ISDIR(start_dir)) {
		errno = ENOTDIR;
		return -1;
	}

	/* check if we should descend into the root directory */
	if ((options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == 0) {
		if (!data->call_back(start_dir, data->call_back_data))
			return 0;
	}

	/* allocate array of counters of directory elements */
	it = (dir_iterator*)malloc((MAX_DIRS_DEPTH + 1) * sizeof(dir_iterator));
	if (!it) {
		return -1;
	}

	/* push dummy counter for the root element */
	it[0].count = 1;
	it[0].dir_path = 0;

	memset(&file, 0, sizeof(file));

	while (!(data->options & FIND_CANCEL))
	{
		dir_entry **insert_at;
		char* dir_path;
		DIR *dp;
		struct dirent *de;

		/* climb down from the tree */
		while (--it[level].count < 0) {
			/* do not need this dir_path anymore */
			free(it[level].dir_path);

			if (--level < 0) {
				/* walked the whole tree */
				assert(!dirs_stack);
				free(it);
				return 0;
			}
		}
		assert(level >= 0 && it[level].count >= 0);

		/* take a filename from dirs_stack and construct the next path */
		if (level) {
			assert(dirs_stack != NULL);
			dir_path = make_path(it[level].dir_path, dirs_stack->filename);
			dir_entry_drop_head(&dirs_stack);
		} else {
			/* the first cycle: start from a root directory */
			dir_path = rsh_strdup(start_dir->path);
		}

		if (!dir_path) continue;

		/* fill the next level of directories */
		level++;
		assert(level < MAX_DIRS_DEPTH);
		it[level].count = 0;
		it[level].dir_path = dir_path;

		if ((options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == FIND_WALK_DEPTH_FIRST)
		{
			int res;
			file_init(&file, dir_path, 1);
			res = file_stat(&file, fstat_flags);

			/* check if we should skip the directory */
			if (res < 0 || !data->call_back(&file, data->call_back_data)) {
				if (res < 0 && (options & FIND_LOG_ERRORS)) {
					data->errors_count++;
				}
				file_cleanup(&file);
				continue;
			}
		}
		file_cleanup(&file);

		/* step into directory */
		dp = opendir(dir_path);
		if (!dp) continue;

		insert_at = &dirs_stack;

		while ((de = readdir(dp)) != NULL)
		{
			int res;

			/* skip the "." and ".." directories */
			if (IS_CURRENT_OR_PARENT_DIR(de->d_name)) continue;

			file.mode = 0;
			file.path = make_path(dir_path, de->d_name);
			if (!file.path) continue;

			res  = file_stat(&file, fstat_flags);
			if (res >= 0) {
				/* process the file or directory */
				if (FILE_ISDIR(&file) && (options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS))) {
					res = ((options & FIND_FOLLOW_SYMLINKS) || !FILE_ISLNK(&file));
				} else if (FILE_ISREG(&file)) {
					/* handle file by callback function */
					res = data->call_back(&file, data->call_back_data);
				}

				/* check if file is a directory and we need to walk it, */
				/* but don't go deeper than max_depth */
				if (FILE_ISDIR(&file) && res && level < max_depth &&
					((options & FIND_FOLLOW_SYMLINKS) || !FILE_ISLNK(&file)))
				{
					/* add the directory name to the dirs_stack */
					if (dir_entry_insert(insert_at, de->d_name, file.mode)) {
						/* the directory name was successfully inserted */
						insert_at = &((*insert_at)->next);
						it[level].count++;
					}
				}
			} else if (options & FIND_LOG_ERRORS) {
				/* report error only if FIND_LOG_ERRORS option is set */
				log_file_t_error(&file);
				data->errors_count++;
			}
			file_cleanup(&file);
		}
		closedir(dp);
	}

	while (dirs_stack) {
		dir_entry_drop_head(&dirs_stack);
	}
	while (level) {
		free(it[level--].dir_path);
	}
	free(it);
	assert(file.path == 0);
	return 0;
}
