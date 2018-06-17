/* win_utils.c - utility functions for Windows and CygWin */
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include "win_utils.h"

/**
 * Set process priority and affinity to use any CPU but the first one,
 * this improves benchmark results on a multi-core systems.
 */
void set_benchmark_cpu_affinity(void)
{
	DWORD_PTR dwProcessMask, dwSysMask, dwDesired;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if ( GetProcessAffinityMask(GetCurrentProcess(), &dwProcessMask, &dwSysMask) ) {
		dwDesired = dwSysMask & (dwProcessMask & ~1); /* remove the first processor */
		dwDesired = (dwDesired ? dwDesired : dwSysMask & ~1);
		if (dwDesired != 0) {
			SetProcessAffinityMask(GetCurrentProcess(), dwDesired);
		}
	}
}

#ifdef _WIN32
/* Windows-only (non-CygWin) functions */

#include <share.h> /* for _SH_DENYWR */
#include <fcntl.h> /* for _O_RDONLY, _O_BINARY */
#include <io.h> /* for isatty */
#include <assert.h>
#include <errno.h>
#include <locale.h>

#include "file.h"
#include "parse_cmdline.h"
#include "rhash_main.h"

/**
 * Convert a c-string to wide character string using given codepage
 *
 * @param str the string to convert
 * @param codepage the codepage to use
 * @return converted string on success, NULL on fail
 */
static wchar_t* cstr_to_wchar(const char* str, int codepage)
{
	wchar_t* buf;
	int size = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
	if (size == 0) return NULL; /* conversion failed */

	buf = (wchar_t*)rsh_malloc(size * sizeof(wchar_t));
	MultiByteToWideChar(codepage, 0, str, -1, buf, size);
	return buf;
}

/**
 * Convert c-string to wide string using primary or secondary codepage.
 *
 * @param str the C-string to convert
 * @param try_no 0 for primary codepage, 1 for a secondary one
 * @return converted wide string on success, NULL on error
 */
wchar_t* c2w(const char* str, int try_no)
{
	int is_utf = (try_no == (opt.flags & OPT_UTF8 ? 0 : 1));
	int codepage = (is_utf ? CP_UTF8 : (opt.flags & OPT_OEM) ? CP_OEMCP : CP_ACP);
	return cstr_to_wchar(str, codepage);
}

/**
 * Convert C-string path to a wide-string path, prepending a long path prefix
 * if it is needed to access the file.
 *
 * @param str the C-string to convert
 * @param try_no 0 for primary codepage, 1 for a secondary one
 * @return converted wide string on success, NULL on error
 */
wchar_t* c2w_long_path(const char* str, int try_no)
{
	wchar_t* wstr = c2w(str, try_no);
	wchar_t* long_path;
	if (!wstr) return NULL;
	long_path = get_long_path_if_needed(wstr);
	if (!long_path) return wstr;
	free(wstr);
	return long_path;
}

/**
 * Convert a wide character string to c-string using given codepage.
 * Optionally set a flag if conversion failed.
 *
 * @param wstr the wide string to convert
 * @param codepage the codepage to use
 * @param failed pointer to the flag, to on failed conversion, can be NULL
 * @return converted string on success, NULL on fail
 */
char* wchar_to_cstr(const wchar_t* wstr, int codepage, int* failed)
{
	int size;
	char *buf;
	BOOL bUsedDefChar, *lpUsedDefaultChar;
	if (codepage == -1) {
		codepage = (opt.flags & OPT_UTF8 ? CP_UTF8 : (opt.flags & OPT_OEM) ? CP_OEMCP : CP_ACP);
	}
	/* note: lpUsedDefaultChar must be NULL for CP_UTF8, otrherwise WideCharToMultiByte() will fail */
	lpUsedDefaultChar = (failed && codepage != CP_UTF8 ? &bUsedDefChar : NULL);

	size = WideCharToMultiByte(codepage, 0, wstr, -1, 0, 0, 0, 0);
	if (size == 0) {
		if (failed) *failed = 1;
		return NULL; /* conversion failed */
	}
	buf = (char*)rsh_malloc(size);
	WideCharToMultiByte(codepage, 0, wstr, -1, buf, size, 0, lpUsedDefaultChar);
	if (failed) *failed = (lpUsedDefaultChar && *lpUsedDefaultChar);
	return buf;
}

/**
 * Convert wide string to multi-byte c-string using codepage specified
 * by command line options.
 *
 * @param wstr the wide string to convert
 * @return c-string on success, NULL on fail
 */
char* w2c(const wchar_t* wstr)
{
	return wchar_to_cstr(wstr, -1, NULL);
}

/**
 * Convert given C-string from encoding specified by
 * command line options to utf8.
 *
 * @param str the string to convert
 * @return converted string on success, NULL on fail
 */
char* win_to_utf8(const char* str)
{
	char* res;
	wchar_t* buf;

	assert((opt.flags & OPT_ENCODING) != 0);
	if (opt.flags & OPT_UTF8) return rsh_strdup(str);

	if ((buf = c2w(str, 0)) == NULL) return NULL;
	res = wchar_to_cstr(buf, CP_UTF8, NULL);
	free(buf);
	return res;
}

#define UNC_PREFIX_SIZE 4

/**
 * Allocate a wide string containing long file path with UNC prefix,
 * if it is needed to access the path, otherwise return NULL.
 *
 * @param wpath original file path, can be a relative one
 * @return allocated long file path if it is needed to access
 *         the path, NULL otherwise
 */
wchar_t* get_long_path_if_needed(const wchar_t* wpath)
{
	if (wcslen(wpath) > 200
		&& (wpath[0] != L'\\' ||  wpath[1] != L'\\'
		|| wpath[2] != L'?' ||  wpath[3] != L'\\'))
	{
		wchar_t* result;
		DWORD size = GetFullPathNameW(wpath, 0, NULL, NULL);
		if (!size) return NULL;
		result = (wchar_t*)rsh_malloc((size + UNC_PREFIX_SIZE) * sizeof(wchar_t));
		wcscpy(result, L"\\\\?\\");
		size = GetFullPathNameW(wpath, size, result + UNC_PREFIX_SIZE, NULL);
		if (size > 0) return result;
		free(result);
	}
	return NULL;
}

/* the range of error codes for access errors */
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED

/**
 * Convert the GetLastError() value to the assignable to errno.
 *
 * @return errno-compatible error code
 */
static int convert_last_error_to_errno(void)
{
	DWORD error_code = GetLastError();
	switch (error_code)
	{
	case NO_ERROR:
		return 0;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_DRIVE:
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILENAME_EXCED_RANGE:
		return ENOENT;
	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
		return EACCES;
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_FAIL_I24:
	case ERROR_SEEK_ON_DEVICE:
		return EACCES;
	case ERROR_LOCK_VIOLATION:
	case ERROR_DRIVE_LOCKED:
	case ERROR_NOT_LOCKED:
	case ERROR_LOCK_FAILED:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_INVALID_BLOCK:
	case ERROR_NOT_ENOUGH_QUOTA:
		return ENOMEM;
	case ERROR_INVALID_ACCESS:
	case ERROR_INVALID_DATA:
	case  ERROR_INVALID_PARAMETER:
		return EINVAL;
	case ERROR_BROKEN_PIPE:
	case ERROR_NO_DATA:
		return EPIPE;
	case ERROR_DISK_FULL:
		return ENOSPC;
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_NESTING_NOT_ALLOWED:
		return EAGAIN;
	}

	/* try to detect error by range */
	if (MIN_EACCES_RANGE <= error_code && error_code <= MAX_EACCES_RANGE) {
		return EACCES;
	} else {
		return EINVAL;
	}
}

/**
 * Assign errno to the error value converted from the GetLastError().
 */
void set_errno_from_last_file_error(void)
{
	errno = convert_last_error_to_errno();
}

/**
 * Concatenate directory path with filename, unicode version.
 *
 * @param dir_path directory path
 * @param dir_len length of directory path in characters
 * @param filename the file name to append to the directory
 * @return concatenated path
 */
wchar_t* make_pathw(const wchar_t* dir_path, size_t dir_len, wchar_t* filename)
{
	wchar_t* res;
	size_t len;

	if (dir_path == 0) dir_len = 0;
	else {
		/* remove leading path separators from filename */
		while (IS_PATH_SEPARATOR_W(*filename)) filename++;

		if (dir_len == (size_t)-1) dir_len = wcslen(dir_path);
	}
	len = wcslen(filename);

	res = (wchar_t*)rsh_malloc((dir_len + len + 2) * sizeof(wchar_t));
	if (dir_len > 0) {
		memcpy(res, dir_path, dir_len * sizeof(wchar_t));
		if (res[dir_len - 1] != L'\\') {
			/* append path separator to the directory */
			res[dir_len++] = L'\\';
		}
	}

	/* append filename */
	memcpy(res + dir_len, filename, (len + 1) * sizeof(wchar_t));
	return res;
}

/* functions to setup/restore console */

/**
 * Restore console on program exit.
 */
static void restore_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE && rhash_data.saved_cursor_size) {
		/* restore cursor size and visibility */
		cci.dwSize = rhash_data.saved_cursor_size;
		cci.bVisible = 1;
		SetConsoleCursorInfo(hOut, &cci);
	}
}

/**
 * Prepare console on program initialization: change console font codepage
 * according to program options and hide cursor.
 */
void setup_console(void)
{
	/* the default encoding is UTF-8 */
	if ((opt.flags & OPT_ENCODING) == 0)
		opt.flags |= OPT_UTF8;

	if ((opt.flags & OPT_UTF8) != 0)
	{
		/* note: we are using numbers 1 = _fileno(stdout), 2 = _fileno(stderr) */
		/* cause _fileno() is undefined,  when compiling as strict ansi C. */
		if (isatty(1))
		{
			_setmode(1, _O_U8TEXT);
			rhash_data.output_flags |= 1;
		}
		if (isatty(2))
		{
			_setmode(2, _O_U8TEXT);
			rhash_data.output_flags |= 2;
		}
#ifdef USE_GETTEXT
		bind_textdomain_codeset(TEXT_DOMAIN, "utf-8");
#endif /* USE_GETTEXT */
	}
	else
	{
		setlocale(LC_CTYPE, opt.flags & OPT_OEM ? ".OCP" : ".ACP");
	}
}

void hide_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE && GetConsoleCursorInfo(hOut, &cci))
	{
		/* store current cursor size and visibility flag */
		rhash_data.saved_cursor_size = (cci.bVisible ? cci.dwSize : 0);

		/* now hide cursor */
		cci.bVisible = 0;
		SetConsoleCursorInfo(hOut, &cci); /* hide cursor */
		rsh_install_exit_handler(restore_cursor);
	}
}

/**
 * Detect the program directory.
 */
void init_program_dir(void)
{
	wchar_t* program_path = NULL;
	DWORD buf_size;
	DWORD len;
	for (buf_size = 2048;; buf_size += 2048)
	{
		program_path = (wchar_t*)rsh_malloc(sizeof(wchar_t) * buf_size);
		len = GetModuleFileNameW(NULL, program_path, buf_size);
		if (len && len < buf_size) break;
		free(program_path);
		if (!len || buf_size >= 32768) return;
	}
	/* remove trailng file name with the last separator */
	for (; len > 0 && !IS_PATH_SEPARATOR_W(program_path[len]); len--);
	for (; len > 0 && IS_PATH_SEPARATOR_W(program_path[len]); len--);
	program_path[len + 1] = 0;
	if (len == 0) {
		free(program_path);
		return;
	}
	rhash_data.program_dir = program_path;
}

#ifdef USE_GETTEXT
/**
 * Check that the path points to an existing directory.
 *
 * @param path the path to check
 * @return 1 if the argument is a directory, 0 otherwise
 */
static int is_directory(const char* path)
{
	DWORD res = GetFileAttributesA(path);
	return (res != INVALID_FILE_ATTRIBUTES &&
		!!(res & FILE_ATTRIBUTE_DIRECTORY));
}

/**
 * Set the locale directory relative to ${PROGRAM_DIR}/LOCALEDIR.
 */
void setup_locale_dir(void)
{
	wchar_t* short_dir;
	char *program_dir = NULL;
	char *locale_dir;
	DWORD buf_size;
	DWORD res;
	
	if (!rhash_data.program_dir) return;
	buf_size = GetShortPathNameW(rhash_data.program_dir, NULL, 0);
	if (!buf_size) return;
	
	short_dir = (wchar_t*)rsh_malloc(sizeof(wchar_t) * buf_size);
	res = GetShortPathNameW(rhash_data.program_dir, short_dir, buf_size);
	if (res > 0 && res < buf_size)
		program_dir = w2c(short_dir);
	free(short_dir);
	if (!program_dir) return;
	
	locale_dir = make_path(program_dir, "locale");
	free(program_dir);
	if (!locale_dir) return;
	
	if (is_directory(locale_dir))
		bindtextdomain(TEXT_DOMAIN, locale_dir);
	free(locale_dir);
}
#endif /* USE_GETTEXT */

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 * @param args list of arguments 
 */
int win_vfprintf(FILE* out, const char* format, va_list args)
{
	if ((out != stdout || !(rhash_data.output_flags & 1))
		&& (out != stderr || !(rhash_data.output_flags & 2)))
		return vfprintf(out, format, args);
	{
		/* because of using a static buffer, this function
                 * can be used only from a single-thread program */
		static char buffer[8192];
		wchar_t *wstr = NULL;
		int res = vsnprintf(buffer, 8192, format, args);
		if (res < 0 || res >= 8192)
		{
			errno = EINVAL;
			return -1;
		}
		wstr = cstr_to_wchar(buffer, CP_UTF8);
		res = fwprintf(out, L"%s", wstr);
		free(wstr);
		return res;
	}
}

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 */
int win_fprintf(FILE* out, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	return win_vfprintf(out, format, args);
}

size_t win_fwrite(const void *ptr, size_t size, size_t count, FILE *out)
{
	if ((out != stdout || !(rhash_data.output_flags & 1))
		&& (out != stderr || !(rhash_data.output_flags & 2)))
		return fwrite(ptr, size, count, out);
	{
		size_t i;
		const char* buf = (const char*)ptr;
		size *= count;
		if (!size)
			return 0;
		for (i = 0; i < size && buf[i] > 0; i++);
		if (i == size)
		{
			wchar_t* wstr = rsh_malloc(sizeof(wchar_t) * (size + 1));
			for (i = 0; i < size; i++)
				wstr[i] = (wchar_t)buf[i];
			wstr[size] = 0;
			fwprintf(out, L"%s", wstr);
			free(wstr);
			return count;
		}
		for (i = 0; (i + 8) <= size; i += 8)
			fwprintf(out, L"%C%C%C%C%C%C%C%C", buf[i], buf[i + 1], buf[i + 2],
				buf[i + 3], buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7]);
		for (; i < size; i++)
			fwprintf(out, L"%C", buf[i]);
		return count;
	}
}

#endif /* _WIN32 */
#else
typedef int dummy_declaration_required_by_strict_iso_c;
#endif /* defined(_WIN32) || defined(__CYGWIN__) */
