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

#include "win_utils.h"
#include "find_file.h"

#if !defined(_WIN32) && (defined(_BSD_SOURCE) || _XOPEN_SOURCE >= 500)
#define USE_LSTAT_FOR_SYMLINKS
#endif

/* from perl manual File::Find
 * These are functions for searching through directory trees doing work on
 * each file found, similarly to the Unix find command.
 * File::Find exports two functions, "find" and "finddepth".
 * They work similarly but have subtle differences.
 * 1. find() does a breadth-first search over the given @directories in the order they are given.
 *    In essence, it works from the top down.
 * 2. finddepth() works just like find() except it does a depth-first search.
 *    It works from the bottom of the directory tree up. */

typedef struct dir_entry
{
	struct dir_entry *next;
	char* filename;
	/*unsigned short level; nesting level */
	unsigned type; /* dir,link, e.t.c. */
	/*bool operator < (struct dir_entry &right) { return (name < right.name); }*/
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
 * @param callback the function to call on each file/directory
 * @param options specifying how to walk the directory tree
 * @param call_back_data a pointer to pass to callback
 */
int find_file(const char* start_dir,
	int (*call_back)(const char* filepath, int type, void* data),
	int options, int max_depth, void* call_back_data)
{
	dir_entry *dirs_stack = NULL; /* root of the dir_list */
	dir_iterator* it;
	int level = 1;
	dir_entry* entry;
	struct rsh_stat_struct st;

	if(max_depth < 0 || max_depth >= MAX_DIRS_DEPTH) {
		max_depth = MAX_DIRS_DEPTH;
	}
	/* skip the directory if max_depth == 0 */
	if(max_depth == 0) {
		return 0;
	}

	/* check that start_dir is a directory */
	if(rsh_stat(start_dir, &st) < 0) {
		return -1; /* errno is already set by stat */
	}
	if( !S_ISDIR(st.st_mode) ) {
		errno = ENOTDIR;
		return -1;
	}

	/* check if we should descend into the root directory */
	if( !(options&FIND_WALK_DEPTH_FIRST)
		&& !call_back(start_dir, FIND_IFDIR | FIND_IFFIRST, call_back_data)) {
			return 0;
	}

	it = (dir_iterator*)malloc(MAX_DIRS_DEPTH*sizeof(dir_iterator));
	if(!it) return 0;

	/* push root directory into dirs_stack */
	it[0].left = 1;
	it[0].prev_dir = rsh_strdup(start_dir);
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

	for(;;) {
		dir_entry *dir, **insert_at;
		char* dir_path;
		DIR *dp;
		struct dirent *de;
		int type;
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

		if( options&FIND_WALK_DEPTH_FIRST ) {
			/* check if we should skip the directory */
			if( !call_back(dir_path, FIND_IFDIR, call_back_data) )
				continue;
		}

		/* read dir */
		dp = opendir(dir_path);
		if(dp == NULL) continue;
		type = FIND_IFFIRST;
		insert_at = &dirs_stack;

		while((de = readdir(dp)) != NULL) {
			int res;
			char* path;
			/* skip "." and ".." dirs */
			if(de->d_name[0] == '.' && (de->d_name[1] == 0 ||
				(de->d_name[1] == '.' && de->d_name[2] == 0 )))
				continue;

			if( !(path = make_path(dir_path, de->d_name)) ) continue;

#ifndef USE_LSTAT_FOR_SYMLINKS
			if(rsh_stat(path, &st) < 0) {
				free(path);
				continue;
			}
#else
			res = (options & FIND_FOLLOW_LINKS ? rsh_stat(path, &st) : lstat(path, &st));
			/*if((st.st_mode&S_IFMT) == S_IFLNK) type |= FIND_IFLNK;*/
			if(res < 0 || (!(options & FIND_FOLLOW_LINKS) && S_ISLNK(st.st_mode)) ) {
				free(path);
				continue;
			}
#endif

/* check bits (the check fails for gcc -ansi) */
/*#if( (S_IFMT >> 12) != 0x0f  || (S_IFDIR >> 12) != FIND_IFDIR )
#  error wrong bits for S_IFMT and S_IFDIR
#endif*/

			/*type |= (S_ISDIR(st.st_mode) ? FIND_IFDIR : 0);*/
			type |= ((st.st_mode >> 12) & 0x0f);

			if((type & FIND_IFDIR) && (options & FIND_WALK_DEPTH_FIRST)) res = 1;
			else {
				/* handle file by callback function */
				res = call_back(path, type, call_back_data);
			}
			free(path);

			/* if file is a directory and we need to walk it */
			if((type & FIND_IFDIR) && res && level < max_depth) {
				/* don't go deeper if max_depth reached */

				/* add directory to dirs_stack */
				if( dir_entry_insert(insert_at, de->d_name, type) ) {
					/* if really added */
					insert_at = &((*insert_at)->next);
					it[level].left++;
				}
			}
			type = 0; /* clear FIND_IFFIRST flag */
		}
		closedir(dp);

		if(it[level].left > 0) level++;
	}
	assert(dirs_stack == NULL);

	free(it);
	return 0;
}
