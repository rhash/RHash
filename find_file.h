/* find_file.h - functions for recursive scan of directories. */
#ifndef FIND_FILE_H
#define FIND_FILE_H

#include "common_func.h"
#include "file.h"

#ifdef __cplusplus
extern "C" {
#endif

/* find_file options */
#define FIND_WALK_DEPTH_FIRST 1
#define FIND_FOLLOW_SYMLINKS 2
#define FIND_SKIP_DIRS 4
#define FIND_LOG_ERRORS 8
#define FIND_CANCEL 16

/**
 * Options for file search.
 */
typedef struct file_search_data
{
	int options;
	int max_depth;
	blocks_vector_t root_files;
	int (*call_back)(file_t* file, int data);
	int call_back_data;
	int errors_count;
} file_search_data;

file_search_data* file_search_data_new(void);
void file_search_add_file(file_search_data* data, tstr_t path, int is_file_list);
void file_search_data_free(file_search_data* data);

void scan_files(file_search_data* data);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FIND_FILE_H */
