/* file_set.h - functions to manipulate a set of files with their hash sums */
#ifndef FILE_SET_H
#define FILE_SET_H

#include "calc_sums.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Filepath with its string-hash (for fast search).
 */
typedef struct file_set_item
{
	unsigned hash;
	char* filepath;
	char* search_filepath; /* for case-insensitive comparison */
} file_set_item;

/* array to store filenames from a parsed hash file */
struct vector_t;
typedef struct vector_t file_set;

#define file_set_new() rsh_vector_new((void(*)(void*))file_set_item_free) /* allocate new file set */
#define file_set_free(set) rsh_vector_free(set) /* free memory */
#define file_set_get(set, index) ((file_set_item*)((set)->array[index])) /* get i-th element */
#define file_set_add(set, item) rsh_vector_add_ptr(set, item) /* add a file_set_item to file_set */

void file_set_item_free(file_set_item *item);
void file_set_add_name(file_set *set, const char* filename);
void file_set_sort(file_set *set);
void file_set_sort_by_path(file_set *set);
int file_set_exist(file_set *set, const char* filename);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FILE_SET_H */
