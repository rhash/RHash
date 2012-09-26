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
# define USE_LSTAT_FOR_SYMLINKS
# define USE_LSTAT 1
#else
# define USE_LSTAT 0
#endif

void process_files(const char* paths[], size_t count,
	find_file_options* opt)
{
	struct file_t file;
	size_t i;
	memset(&file, 0, sizeof(file));

	for(i = 0; i < count && !(opt->options & FIND_CANCEL); i++) {
		file.path = (char*)paths[i];

		if(!IS_DASH_STR(file.path) && rsh_file_stat2(&file, USE_LSTAT) < 0) {
			if((opt->options & FIND_LOG_ERRORS) != 0) {
				log_file_error(file.path);
				opt->errors_count++;
			}
			continue;
		}
		if(!IS_DASH_STR(file.path) && (file.mode & FILE_IFDIR) != 0) {
			find_file(&file, opt);
		} else {
			file.mode |= FILE_ISROOT;
			opt->call_back(&file, opt->call_back_data);
		}
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
 * Free memory allocated by a dir_entry object.
 *
 * @param e pointer to object to deallocate
 */
static void dir_entry_free(dir_entry* e)
{
	free(e->filename);
	free(e);
}

/**
 * Directory iterator.
 */
typedef struct dir_iterator
{
	int left;
	char* prev_dir;
} dir_iterator;
#define MAX_DIRS_DEPTH 64

/**
 * Walk directory tree and call given callback function to process each file/directory.
 *
 * @param start_dir path to the directory to walk recursively
 * @param options the options specifying how to walk the directory tree
 */
int find_file(file_t* start_dir, find_file_options* options)
{
	dir_entry *dirs_stack = NULL; /* root of the dir_list */
	dir_iterator* it;
	int level = 1;
	int max_depth = options->max_depth;
	int flags = options->options;
	dir_entry* entry;
	file_t file;

	if(max_depth < 0 || max_depth >= MAX_DIRS_DEPTH) {
		max_depth = MAX_DIRS_DEPTH;
	}

	/* skip the directory if max_depth == 0 */
	if(max_depth == 0) {
		return 0;
	}

	memset(&file, 0, sizeof(file));

	if((start_dir->mode & FILE_IFDIR) == 0) {
		errno = ENOTDIR;
		return -1;
	}

	/* check if we should descend into the root directory */
	if((flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS)) == 0) {
		if(!options->call_back(start_dir, options->call_back_data))
			return 0;
	}

	it = (dir_iterator*)malloc(MAX_DIRS_DEPTH * sizeof(dir_iterator));
	if(!it) return 0;

	/* push root directory into dirs_stack */
	it[0].left = 1;
	it[0].prev_dir = rsh_strdup(start_dir->path);
	it[1].prev_dir = NULL;
	if(!it[0].prev_dir) {
		errno = ENOMEM;
		return -1;
	}
	entry = dir_entry_insert(&dirs_stack, NULL, 0);
	if(!entry) {
		free(it[0].prev_dir);
		free(it);
		errno = ENOMEM;
		return -1;
	}

	while(!(options->options & FIND_CANCEL)) {
		dir_entry *dir, **insert_at;
		char* dir_path;
		DIR *dp;
		struct dirent *de;

		/* walk back */
		while((--level) >= 0 && it[level].left <= 0) free(it[level+1].prev_dir);
		if(level < 0) break;
		assert(dirs_stack != NULL);
		/* on the first cycle: level == 0, stack[0] == 0; */

		dir = dirs_stack; /* take last dir from the list */
		dirs_stack = dirs_stack->next; /* remove last dir from the list */
		it[level].left--;

		dir_path = (!dir->filename ? rsh_strdup(it[level].prev_dir) :
			make_path(it[level].prev_dir, dir->filename) );
		dir_entry_free(dir);
		if(!dir_path) continue;

		level++;
		it[level].left = 0;
		it[level].prev_dir = dir_path;

		if((flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS))
			== FIND_WALK_DEPTH_FIRST) {
			rsh_file_cleanup(&file);
			file.path = dir_path;
			/* check if we should skip the directory */
			if(!options->call_back(&file, options->call_back_data))
				continue;
		}

		/* read dir */
		dp = opendir(dir_path);
		if(dp == NULL) continue;

		insert_at = &dirs_stack;

		while((de = readdir(dp)) != NULL) {
			int res;
			/* skip "." and ".." dirs */
			if(de->d_name[0] == '.' && (de->d_name[1] == 0 ||
				(de->d_name[1] == '.' && de->d_name[2] == 0 )))
				continue;

			if( !(file.path = make_path(dir_path, de->d_name)) ) continue;

			res  = rsh_file_stat2(&file, USE_LSTAT);
			/* process */
			if(res >= 0) {
				if((file.mode & FILE_IFDIR) &&
					(flags & (FIND_WALK_DEPTH_FIRST | FIND_SKIP_DIRS))) res = 1;
				else {
					/* handle file by callback function */
					res = options->call_back(&file, options->call_back_data);
				}

				/* if file is a directory and we need to walk it */
				if((file.mode & FILE_IFDIR) && res && level < max_depth) {
					/* don't go deeper if max_depth reached */

					/* add directory to dirs_stack */
					if(dir_entry_insert(insert_at, de->d_name, file.mode)) {
						/* if really added */
						insert_at = &((*insert_at)->next);
						it[level].left++;
					}
				}
			} else if (options->options & FIND_LOG_ERRORS) {
				log_file_error(file.path);
			}
			rsh_file_cleanup(&file);
			free(file.path);
		}
		closedir(dp);

		if(it[level].left > 0) level++;
	}
	assert(dirs_stack == NULL);

	free(it);
	return 0;
}
