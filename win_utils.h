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

#define UNC_PREFIX_SIZE 4
#define IS_UNC_PREFIX(p) ((p)[0] == L'\\' &&  (p)[1] == L'\\' && (p)[2] == L'?' &&  (p)[3] == L'\\')

/* encoding conversion functions */
enum ConversionBitFlags {
	ConvertToPrimaryEncoding = 1,
	ConvertToSecondaryEncoding = 2,
	ConvertToUtf8 = 4,
	ConvertToNative = 8,
	ConvertPath = 16,
	ConvertExact = 32,
	ConvertUtf8ToWcs = ConvertToUtf8,
	ConvertNativeToWcs = ConvertToNative,
	ConvertEncodingMask = (ConvertToPrimaryEncoding | ConvertToSecondaryEncoding | ConvertToUtf8 | ConvertToNative)
};
wchar_t* convert_str_to_wcs(const char* str, unsigned flags);
char* convert_wcs_to_str(const wchar_t* wstr, unsigned flags);
char* convert_str_encoding(const char* str, unsigned flags);

/* file helper functions */
void set_errno_from_last_file_error(void);
wchar_t* get_long_path_if_needed(const wchar_t* wpath);
wchar_t* get_program_dir(void);

/* functions for program initialization */
void init_program_dir(void);
void setup_console(void);
void setup_locale_dir(void);
void hide_cursor(void);

/* output-related functions */
int win_is_console_stream(FILE* out);
int win_fprintf(FILE*, const char* format, ...);
int win_fprintf_warg(FILE*, const char* format, ...);
int win_vfprintf(FILE*, const char* format, va_list args);
size_t win_fwrite(const void* ptr, size_t size, size_t count, FILE* out);

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* defined(_WIN32) || defined(__CYGWIN__) */

#endif /* WIN_UTILS_H */
