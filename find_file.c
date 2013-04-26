/* find_file.c
 *
 * find_file function for searching through directory trees doing work
 * on each file found similar to the Unix find command.
 */

#include "common_func.h" /* should be included before the C library files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h> /* ino_t */
#include <dirent.h>    /* opendir/readdir */
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "output.h"
#include "win_utils.h"
#include "find_file.h"

#if !defined(_WIN32) && (defined(_BSD_SOURCE) || _XOPEN_SOURCE >= 500)
# define USE_LSTAT 1
#else
# define USE_LSTAT 0
#endif

#define IS_CURRENT_OR_PARENT_DIR(s) ((s)[0]=='.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

void process_files(const char* paths[], size_t count, find_file_options* opt)
{
	struct file_t file;
	size_t i;
	memset(&file, 0, sizeof(file));

	for(i = 0; i < count && !(opt->options & FIND_CANCEL); i++)
	{
		rsh_file_cleanup(&file);
		file.path = (char*)paths[i];

		if (IS_DASH_STR(file.path)) {
			file.mode = FILE_IFSTDIN;
		} else {
			/* read attributes of the file */
			if(rsh_file_stat2(&file, USE_LSTAT) < 0) {
				if((opt->options & FIND_LOG_ERRORS) != 0) {
					log_file_error(file.path);
					opt->errors_count++;
				}
				continue;
			}
			/* check if file is a directory */
			if(FILE_ISDIR(&file)) {
				find_file(&file, opt);
				continue;
			}
		}
		assert(!FILE_ISDIR(&file));

		/* process a regular file or a dash '-' path */
		file.mode |= FILE_IFROOT;
		opt->call_back(&file, opt->call_back_data);
	}

	rsh_file_cleanup(&file);
}

typedef struct dir_entry
{
	struct dir_entry *next;
	char* filename;
	unsigned type; /* dir,link, etc. */
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
	if(!e) return NULL;
	if(filename) {
		e->filename = rsh_strdup(filename);
		if(!e->filename) {
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
	if(e) *at = e;
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
	int left;
	char* dir_path;
} dir_iterator;
#define MAX_DIRS_DEPTH 64

/**
 * Walk directory tree and call given callback function to process each file/directory.
 *
 * @param start_dir path to the directory to walk recursively
 * @param options the options specifying how to walk the directory tree
 * @return 0 on success, -1 on error
 */
int find_file(file_t* start_dir, find_file_options* options)
{
	dir_entry *dirs_stack = NULL; /* root of the dir_list */
	dir_iterator* it;
	int level = 0;
	int max_depth = options->max_depth;
	int flags = options->options;
	file_t file;

	if(max_depth < 0 || max_depth >= MAX_DIRS_DEPTH) {
		max_depth = MAX_DIRS_DEPTH - 1;
	}

	/* skip the directory if max_depth == 0 */
	if(!max_depth) return 0;

	if(!FILE_ISDIR(start_dir)) {
		errno = ENOTDIR;
		return -1;
	}

	/* check if we should descend into the root directory */
	if((flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == 0) {
		if(!options->call_back(start_dir, options->call_back_data))
			return 0;
	}

	/* allocate array of counters of directory elements */
	it = (dir_iterator*)malloc((MAX_DIRS_DEPTH + 1) * sizeof(dir_iterator));
	if(!it) {
		errno = ENOMEM;
		return -1;
	}

	/* push dummy counter for the root element */
	it[0].left = 1;
	it[0].dir_path = 0;

	memset(&file, 0, sizeof(file));

	while(!(options->options & FIND_CANCEL))
	{
		dir_entry **insert_at;
		char* dir_path;
		DIR *dp;
		struct dirent *de;

		/* climb down from the tree */
		while(--it[level].left < 0) {
			/* do not need this dir_path anymore */
			free(it[level].dir_path);

			if(--level < 0) {
				/* walked the whole tree */
				assert(!dirs_stack);
				free(it);
				rsh_file_cleanup(&file);
				return 0;
			}
		}
		assert(level >= 0 && it[level].left >= 0);

		/* take a filename from dirs_stack and construct the next path */
		if(level) {
			assert(!dirs_stack);
			dir_path = make_path(it[level].dir_path, dirs_stack->filename);
			dir_entry_drop_head(&dirs_stack);
		} else {
			/* the first cycle: start from a root directory */
			dir_path = rsh_strdup(start_dir->path);
		}

		if(!dir_path) continue;

		/* fill the next level of directories */
		level++;
		assert(level < MAX_DIRS_DEPTH);
		it[level].left = 0;
		it[level].dir_path = dir_path;

		if((flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS))
			== FIND_WALK_DEPTH_FIRST)
		{
			file.path = dir_path;
			rsh_file_stat2(&file, USE_LSTAT);

			/* check if we should skip the directory */
			if(!options->call_back(&file, options->call_back_data))
				continue;
		}

		/* step into directory */
		dp = opendir(dir_path);
		if(!dp) continue;

		insert_at = &dirs_stack;

		while((de = readdir(dp)) != NULL) {
			int res;

			/* skip "." and ".." dirs */
			if(IS_CURRENT_OR_PARENT_DIR(de->d_name)) continue;

			file.path = make_path(dir_path, de->d_name);
			if(!file.path) continue;

			res  = rsh_file_stat2(&file, USE_LSTAT);
			if(res >= 0) {
				/* process the file or directory */
				if(FILE_ISDIR(&file) && (flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS))) {
					res = 1;
				} else {
					/* handle file by callback function */
					res = options->call_back(&file, options->call_back_data);
				}

				/* check if file is a directory and we need to walk it, */
				/* but don't go deeper than max_depth */
				if(FILE_ISDIR(&file) && res && level < max_depth) {
					/* add the directory name to the dirs_stack */
					if(dir_entry_insert(insert_at, de->d_name, file.mode)) {
						/* the directory name was successfuly inserted */
						insert_at = &((*insert_at)->next);
						it[level].left++;
					}
				}
			} else if (options->options & FIND_LOG_ERRORS) {
				/* report error only if FIND_LOG_ERRORS option is set */
				log_file_error(file.path);
			}
			free(file.path);
		}
		closedir(dp);
	}

	while(dirs_stack) dir_entry_drop_head(&dirs_stack);
	while(level) free(it[level--].dir_path);
	free(it);
	rsh_file_cleanup(&file);
	return 0;
}
