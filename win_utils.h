/* win_utils.h - utility functions for Windows and CygWin */
#ifndef WIN_UTILS_H
#define WIN_UTILS_H

#if defined(_WIN32) || defined(__CYGWIN__)

#ifdef __cplusplus
extern "C" {
#endif

void set_benchmark_cpu_affinity(void);

/* windows only declarations */
#ifdef _WIN32
#include "common_func.h"
#include <stdarg.h>

/* encoding conversion functions */
wchar_t* c2w(const char* str, int try_no);
wchar_t* c2w_long_path(const char* str, int try_no);
char* w2c(const wchar_t* wstr);
char* win_to_utf8(const char* str);
#define win_is_utf8() (opt.flags & OPT_UTF8)
#define WIN_DEFAULT_ENCODING -1
char* wchar_to_cstr(const wchar_t* wstr, int codepage, int* failed);

/* file functions */
void set_errno_from_last_file_error(void);
wchar_t* make_pathw(const wchar_t* dir_path, size_t dir_len, wchar_t* filename);
wchar_t* get_long_path_if_needed(const wchar_t* wpath);

void setup_console(void);
void hide_cursor(void);
void init_program_dir(void);
void setup_locale_dir(void);
int win_fprintf(FILE*, const char* format, ...);
int win_vfprintf(FILE*, const char* format, va_list args);
size_t win_fwrite(const void *ptr, size_t size, size_t count, FILE *out);

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* defined(_WIN32) || defined(__CYGWIN__) */

#endif /* WIN_UTILS_H */
