/* find_file.h - declaration of find_file function.
 *
 * find_file function searces through a directory tree calling a call_back on
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

/* masks for file flags passed to the call_back function */
#define FIND_IFDIR   0x04
#define FIND_IFLNK   0x0a
#define FIND_IFFIRST 0x10

/*struct find_file_options {
  unsigned flags;
  int max_depth;
};*/

int find_file(const char* start_dir,
  int (*call_back)(const char* filepath, int type, void* data),
  int options, int max_depth, void* call_back_data);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FIND_FILE_H */
