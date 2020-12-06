/* find_file.c - functions for recursive scan of directories. */

#include "find_file.h"
#include "output.h"
#include "parse_cmdline.h"
#include "platform.h"
#include "win_utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <dirent.h>    /* opendir/readdir */
#endif

#define IS_DASH_TSTR(s) ((s)[0] == RSH_T('-') && (s)[1] == RSH_T('\0'))
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

static void file_search_add_special_file(file_search_data* search_data, unsigned file_mode, tstr_t str)
{
	file_t file;
	char* filename = (file_mode & FileIsStdin ? "(stdin)" : "(message)");
	char* ext_data = 0;
	if (file_mode & FileIsData)
	{
#ifdef _WIN32
		ext_data = convert_wcs_to_str(str, ConvertToUtf8 | ConvertExact);
		/* we assume that this conversion always succeeds and the following condition is never met */
		if (!ext_data)
			return;
#else
		ext_data = rsh_strdup(str);
#endif
	}
	file_init_by_print_path(&file, NULL, filename, file_mode);
	if (file_mode & FileIsData)
	{
		file.data = ext_data;
		file.size = strlen(ext_data);
	}
	add_root_file(search_data, &file);
}

void file_search_add_file(file_search_data* data, tstr_t path, unsigned file_mode)
{
#ifdef _WIN32
	size_t length, index;
	wchar_t* p;
#endif
	file_t file;
	file_mode &= (FileIsList | FileIsData);
	assert((file_mode & (file_mode - 1)) == 0);

	file_mode |= FileIsRoot; /* mark the file as obtained from the command line */
	if ((file_mode & FileIsData) != 0)
	{
		file_search_add_special_file(data, file_mode, path);
		return;
	}
	if (IS_DASH_TSTR(path))
	{
		file_search_add_special_file(data, (file_mode | FileIsStdin), NULL);
		return;
	}

#ifdef _WIN32
	/* remove trailing path separators, except a separator preceded by ':' */
	p = wcschr(path, L'\0') - 1;
	for (; p > path && IS_PATH_SEPARATOR_W(*p) && p[-1] != L':'; p--)
		*p = 0;

	length = p - path + 1;
	index = wcscspn(path, L"*?");

	if (index < length && wcscspn(path + index, L"/\\") >= (length - index))
	{
		/* a wildcard is found without a directory separator after it */
		wchar_t* parent;
		WIN32_FIND_DATAW d;
		HANDLE handle;

		/* Expand the wildcard, found in the provided file path, and store the results into
		 * data->root_files. If the wildcard is not found then error is reported.
		 * NB, only wildcards in the basename (the last filename) of the path are expanded. */

		/* find a directory separator before the file name */
		for (; index > 0 && !IS_PATH_SEPARATOR(path[index]); index--);
		parent = (IS_PATH_SEPARATOR(path[index]) ? path : 0);

		handle = FindFirstFileW(path, &d);
		if (INVALID_HANDLE_VALUE != handle)
		{
			do
			{
				int res;
				tpath_t filepath;
				if (IS_CURRENT_OR_PARENT_DIRW(d.cFileName))
					continue;

				/* skip directories unless we are in recursive mode */
				if (data->max_depth == 0 && (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					continue;

				filepath = make_wpath(parent, index + 1, d.cFileName);
				if (!filepath)
					continue;
				res = file_init(&file, filepath, file_mode | FileInitUpdatePrintPathSlashes | FileInitRunFstat);
				free(filepath);

				/* convert file name */
				if (res == 0 && !(opt.flags & OPT_UTF8) && !file_get_print_path(&file, FPathPrimaryEncoding))
					res = -1;

				/* quietly skip unconvertible file names and nonexistent files */
				if (res < 0) {
					data->errors_count++;
					file_cleanup(&file);
					continue;
				}
				add_root_file(data, &file);
			}
			while (FindNextFileW(handle, &d));
			FindClose(handle);
		}
		else
		{
			/* report error on the specified wildcard */
			file_init(&file, path, FileInitReusePath);
			set_errno_from_last_file_error();
			log_error_file_t(&file);
			file_cleanup(&file);
			data->errors_count++;
		}
	}
	else
	{
		if (file_init(&file, path, file_mode | FileInitRunFstat) < 0) {
			log_error_file_t(&file);
			file_cleanup(&file);
			data->errors_count++;
			return;
		}
		if (!(opt.flags & OPT_UTF8) && !file_get_print_path(&file, FPathPrimaryEncoding)) {
			log_error_msg_file_t(_("can't convert the file path to local encoding: %s\n"), &file);
			file_cleanup(&file);
			data->errors_count++;
			return;
		}

		/* mark the file as obtained from the command line */
		add_root_file(data, &file);
	}
#else
	/* init the file and test for its existence */
	if (file_init(&file, path, file_mode | FileInitRunLstat) < 0)
	{
		log_error_file_t(&file);
		file_cleanup(&file);
		data->errors_count++;
		return;
	}
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
		assert(!!(file->mode & FileIsRoot));

		/* check if file is a directory */
		if (FILE_ISLIST(file))
		{
			file_list_t list;
			if (file_list_open(&list, file) < 0)
			{
				log_error_file_t(file);
				continue;
			}
			while (file_list_read(&list))
			{
				data->callback(&list.current_file, data->callback_data);
			}
			file_list_close(&list);
		}
		else if (FILE_ISDIR(file))
		{
			/* silently skip symlinks to directories if required */
			if (skip_symlinked_dirs && FILE_ISLNK(file))
				continue;

			if (data->max_depth != 0)
			{
				dir_scan(file, data);
			}
			else if ((data->options & FIND_LOG_ERRORS) != 0)
			{
				errno = EISDIR;
				log_error_file_t(file);
			}
		}
		else
		{
			/* process a regular file or a dash '-' path */
			data->callback(file, data->callback_data);
		}
	}
}

/**
 * An entry of a list containing content of a directory.
 */
typedef struct dir_entry
{
	struct dir_entry* next;
	tstr_t filename;
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
static dir_entry* dir_entry_new(dir_entry* next, tstr_t filename, unsigned type)
{
	dir_entry* e = (dir_entry*)malloc(sizeof(dir_entry));
	if (!e)
		return NULL;
	if (filename)
	{
		e->filename = rsh_tstrdup(filename);
		if (!e->filename)
		{
			free(e);
			return NULL;
		}
	}
	else
	{
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
static dir_entry* dir_entry_insert(dir_entry** at, tstr_t filename, unsigned type)
{
	dir_entry* e = dir_entry_new(*at, filename, type);
	if (e)
		*at = e;
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
	tpath_t dir_path;
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
	dir_entry* dirs_stack = NULL; /* root of the dir_list */
	dir_iterator* it;
	int level = 0;
	int max_depth = data->max_depth;
	int options = data->options;
	int fstat_bit = (data->options & FIND_FOLLOW_SYMLINKS ? FileInitRunFstat : FileInitRunLstat);
	file_t file;

	if (max_depth < 0 || max_depth >= MAX_DIRS_DEPTH)
		max_depth = MAX_DIRS_DEPTH - 1;

	/* skip the directory if max_depth == 0 */
	if (!max_depth)
		return 0;

	if (!FILE_ISDIR(start_dir))
	{
		errno = ENOTDIR;
		return -1;
	}

	/* check if we should descend into the root directory */
	if ((options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == 0)
	{
		if (!data->callback(start_dir, data->callback_data))
			return 0;
	}

	/* allocate array of counters of directory elements */
	it = (dir_iterator*)malloc((MAX_DIRS_DEPTH + 1) * sizeof(dir_iterator));
	if (!it)
		return -1;

	/* push dummy counter for the root element */
	it[0].count = 1;
	it[0].dir_path = 0;

	memset(&file, 0, sizeof(file));

	while (!(data->options & FIND_CANCEL))
	{
		dir_entry** insert_at;
		tpath_t dir_path;
		DIR* dp;
		struct dirent* de;

		/* go down from the tree */
		while (--it[level].count < 0)
		{
			/* do not need this dir_path anymore */
			free(it[level].dir_path);
			if (--level < 0)
			{
				/* walked the whole tree */
				assert(dirs_stack == NULL);
				free(it);
				return 0;
			}
		}
		assert(level >= 0 && it[level].count >= 0);

		/* take a filename from dirs_stack and construct the next path */
		if (level)
		{
			assert(dirs_stack != NULL);
			dir_path = make_tpath(it[level].dir_path, dirs_stack->filename);
			dir_entry_drop_head(&dirs_stack);
		}
		else
		{
			/* the first cycle: start from a root directory */
			dir_path = rsh_tstrdup(start_dir->real_path);
		}
		if (!dir_path)
			continue;

		/* fill the next level of directories */
		level++;
		assert(level < MAX_DIRS_DEPTH);
		it[level].count = 0;
		it[level].dir_path = dir_path;

		if ((options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == FIND_WALK_DEPTH_FIRST)
		{
			int res = file_init(&file, dir_path, FileIsDir | fstat_bit);

			/* check if we should skip the directory */
			if (res < 0 || !data->callback(&file, data->callback_data))
			{
				if (res < 0 && (options & FIND_LOG_ERRORS)) {
					log_error_file_t(&file);
					data->errors_count++;
				}
				file_cleanup(&file);
				continue;
			}
		}
		file_cleanup(&file);

		/* step into directory */
		dp = rsh_topendir(dir_path);
		if (!dp)
			continue;
		insert_at = &dirs_stack;

		while ((de = readdir(dp)) != NULL)
		{
			int res;
			tpath_t filepath;

			/* skip the "." and ".." directories */
			if (IS_CURRENT_OR_PARENT_DIR(de->d_name))
				continue;

			filepath = make_tpath(dir_path, dirent_get_tname(de));
			if (!filepath)
				continue;
			res  = file_init(&file, filepath, fstat_bit | FileInitUpdatePrintPathSlashes);
			free(filepath);
			if (res >= 0)
			{
				/* process the file or directory */
				if (FILE_ISDIR(&file) && (options & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)))
				{
					res = ((options & FIND_FOLLOW_SYMLINKS) || !FILE_ISLNK(&file));
				}
				else if (FILE_ISREG(&file))
				{
					/* handle file by callback function */
					res = data->callback(&file, data->callback_data);
				}

				/* check if file is a directory and we need to walk it, */
				/* but don't go deeper than max_depth */
				if (FILE_ISDIR(&file) && res && level < max_depth &&
					((options & FIND_FOLLOW_SYMLINKS) || !FILE_ISLNK(&file)))
				{
					/* add the directory name to the dirs_stack */
					if (dir_entry_insert(insert_at, dirent_get_tname(de), file.mode))
					{
						/* the directory name was successfully inserted */
						insert_at = &((*insert_at)->next);
						it[level].count++;
					}
				}
			}
			else if (options & FIND_LOG_ERRORS)
			{
				/* report error only if FIND_LOG_ERRORS option is set */
				log_error_file_t(&file);
				data->errors_count++;
			}
			file_cleanup(&file);
		}
		closedir(dp);
	}

	while (dirs_stack)
	{
		dir_entry_drop_head(&dirs_stack);
	}
	while (level)
	{
		free(it[level--].dir_path);
	}
	free(it);
	return 0;
}
