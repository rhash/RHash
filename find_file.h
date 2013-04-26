/* find_file.h - declaration of find_file function.
 *
 * find_file function searches through a directory tree calling a call_back on
 * each file.
 */
#ifndef FIND_FILE_H
#define FIND_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/* find_file options */
#define FIND_WALK_DEPTH_FIRST 1
#define FIND_FOLLOW_LINKS 2
#define FIND_SKIP_DIRS 4
#define FIND_LOG_ERRORS 8
#define FIND_CANCEL 16

/* file flags  */
#define FILE_IFROOT  0x10
#define FILE_IFSTDIN 0x20

typedef struct find_file_options {
	int options;
	int max_depth;
	int (*call_back)(file_t* file, void* data);
	void* call_back_data;
	int errors_count;
} find_file_options;

void process_files(const char** paths, size_t count,
	find_file_options* options);

int find_file(file_t* start_dir, find_file_options* options);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FIND_FILE_H */
