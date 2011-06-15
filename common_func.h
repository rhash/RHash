/* common_func.h */
#ifndef COMMON_FUNC_H
#define COMMON_FUNC_H

/* use 64-bit off_t */
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdint.h>
#include <stdio.h>
#include <time.h> /* for time_t */
#include "librhash/util.h"

#ifndef _WIN32
#include <sys/time.h> /* for timeval */
/*#else
#include <windows.h>*/
#elif _MSC_VER > 1300
#include "win32/platform-dependent.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* string function */
void sprintI64(char *dst, uint64_t number, int max_width);
int  int_len(uint64_t num);

int  urlencode(char *dst, const char *name);
int  is_binary_string(const char* str);
char* str_tolower(const char* str);
char* str_trim(char* str);
char* str_set(char* buf, int ch, int size);
size_t strlen_utf8_c(const char *str);

#define IS_DASH_STR(s) ((s)[0] == '-' && (s)[1] == '\0')
#define IS_COMMENT(c) ((c) == ';' || (c) == '#')

/* file function */
const char* get_basename(const char* path);
char* get_dirname(const char* path);
char* make_path(const char* dir, const char* filename);

#ifdef _WIN32
# define IF_WINDOWS(code) code
# define SYS_PATH_SEPARATOR '\\'
# define IS_PATH_SEPARATOR(c) ((c) == '\\' || (c) == '/')
# define IS_PATH_SEPARATOR_W(c) ((c) == L'\\' || (c) == L'/')
# define rsh_fopen_bin(path, mode) win_fopen_bin(path, mode)
# define is_utf8() win_is_utf8()
# define to_utf8(str) win_to_utf8(str)
#else /* non _WIN32 part */
# define IF_WINDOWS(code)
# define SYS_PATH_SEPARATOR '/'
# define IS_PATH_SEPARATOR(c) ((c) == '/')
# define rsh_fopen_bin(path, mode) fopen(path, mode)
  /* stub for utf8 */
# define is_utf8() 1
# define to_utf8(str) NULL
#endif /* _WIN32 */

/* rhash stat function */
#if (__MSVCRT_VERSION__ >= 0x0601) || (_MSC_VER >= 1400)
# define rsh_stat_struct __stat64
# define rsh_time_struct __time64_t
# define rsh_stat(path, st) win_stat(path, st)
# define clib_wstat(path, st) _wstat64(path, st)
#elif defined(_WIN32) && (defined(__MSVCRT__) || defined(_MSC_VER))
# define rsh_stat_struct _stati64
# define rsh_time_struct __time64_t
# define rsh_stat(path, st) win_stat(path, st)
# define clib_wstat(path, st) _wstati64(path, st)
#else
# define rsh_stat_struct stat
# define rsh_time_struct time_t
# define rsh_stat(path, st) stat(path, st)
/* # define clib_wstat(path, st) _wstat32(path, st) */
#endif

typedef struct rsh_stat_struct rsh_stat_buf;

void print_time(FILE *out, time_t time);
unsigned rhash_get_ticks(void);

void  rhash_exit(int code);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* COMMON_FUNC_H */
