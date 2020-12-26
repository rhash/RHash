/* win_utils.c - utility functions for Windows and CygWin */
#if defined(_WIN32) || defined(__CYGWIN__)

/* Fix #138: require to use MSVCRT implementation of *printf functions */
#define __USE_MINGW_ANSI_STDIO 0
#include "win_utils.h"
#include <windows.h>

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

#include "file.h"
#include "parse_cmdline.h"
#include <share.h> /* for _SH_DENYWR */
#include <fcntl.h> /* for _O_RDONLY, _O_BINARY */
#include <io.h> /* for isatty */
#include <assert.h>
#include <errno.h>
#include <locale.h>

struct console_data_t
{
	unsigned console_flags;
	unsigned saved_cursor_size;
	unsigned primary_codepage;
	unsigned secondary_codepage;
	wchar_t format_buffer[4096];
	wchar_t printf_result[65536];
	wchar_t program_dir[32768];
};
struct console_data_t console_data;

/**
 * Convert a c-string as wide character string using given codepage
 * and print it to the given buffer.
 *
 * @param str the string to convert
 * @param codepage the codepage to use
 * @param buffer buffer to print the string to
 * @param buf_size buffer size in bytes
 * @return converted string on success, NULL on fail with error code stored in errno
 */
static wchar_t* cstr_to_wchar_buffer(const char* str, int codepage, wchar_t* buffer, size_t buf_size)
{
	if (MultiByteToWideChar(codepage, 0, str, -1, buffer, buf_size / sizeof(wchar_t)) != 0)
		return buffer;
	set_errno_from_last_file_error();
	return NULL; /* conversion failed */
}

/**
 * Convert a c-string to wide character string using given codepage.
 * The result is allocated with malloc and must be freed by caller.
 *
 * @param str the string to convert
 * @param codepage the codepage to use
 * @param exact_conversion non-zero to require exact encoding conversion, 0 otherwise
 * @return converted string on success, NULL on fail with error code stored in errno
 */
static wchar_t* cstr_to_wchar(const char* str, int codepage, int exact_conversion)
{
	DWORD flags = (exact_conversion ? MB_ERR_INVALID_CHARS : 0);
	int size = MultiByteToWideChar(codepage, flags, str, -1, NULL, 0);
	if (size != 0) {
		wchar_t* buf = (wchar_t*)rsh_malloc(size * sizeof(wchar_t));
		if (MultiByteToWideChar(codepage, flags, str, -1, buf, size) != 0)
			return buf;
	}
	set_errno_from_last_file_error();
	return NULL; /* conversion failed */
}

/**
 * Convert a wide character string to c-string using given codepage.
 * The result is allocated with malloc and must be freed by caller.
 *
 * @param wstr the wide string to convert
 * @param codepage the codepage to use
 * @param exact_conversion non-zero to require exact encoding conversion, 0 otherwise
 * @return converted string on success, NULL on fail with error code stored in errno
 */
static char* wchar_to_cstr(const wchar_t* wstr, int codepage, int exact_conversion)
{
	int size;
	BOOL bUsedDefChar = FALSE;
	BOOL* lpUsedDefaultChar = (exact_conversion && codepage != CP_UTF8 ? &bUsedDefChar : NULL);
	if (codepage == -1) {
		codepage = (opt.flags & OPT_UTF8 ? CP_UTF8 : (opt.flags & OPT_ENC_DOS) ? CP_OEMCP : CP_ACP);
	}
	size = WideCharToMultiByte(codepage, 0, wstr, -1, NULL, 0, 0, NULL);
	/* size=0 is returned only on fail, cause even an empty string requires buffer size=1 */
	if (size != 0) {
		char* buf = (char*)rsh_malloc(size);
		if (WideCharToMultiByte(codepage, 0, wstr, -1, buf, size, 0, lpUsedDefaultChar) != 0) {
			if (!bUsedDefChar)
				return buf;
			free(buf);
			errno = EILSEQ;
			return NULL;
		}
		free(buf);
	}
	set_errno_from_last_file_error();
	return NULL;
}

/**
 * Convert c-string to wide string using encoding and transformation specified by flags.
 * The result is allocated with malloc and must be freed by caller.
 *
 * @param str the C-string to convert
 * @param flags bit-mask containing the following bits: ConvertToPrimaryEncoding, ConvertToSecondaryEncoding,
 *              ConvertToUtf8, ConvertToNative, ConvertPath, ConvertExact
 * @return converted wide string on success, NULL on fail with error code stored in errno
 */
wchar_t* convert_str_to_wcs(const char* str, unsigned flags)
{
	int is_utf8 = flags & (opt.flags & OPT_UTF8 ? (ConvertToUtf8 | ConvertToPrimaryEncoding) : (ConvertToUtf8 | ConvertToSecondaryEncoding));
	int codepage = (is_utf8 ? CP_UTF8 : (opt.flags & OPT_ENC_DOS) ? CP_OEMCP : CP_ACP);
	wchar_t* wstr = cstr_to_wchar(str, codepage, (flags & ConvertExact));
	if (wstr && (flags & ConvertPath)) {
		wchar_t* long_path = get_long_path_if_needed(wstr);
		if (long_path) {
			free(wstr);
			return long_path;
		}
	}
	return wstr;
}

/**
 * Convert wide string to c-string using encoding and transformation specified by flags.
 *
 * @param str the C-string to convert
 * @param flags bit-mask containing the following bits: ConvertToPrimaryEncoding, ConvertToSecondaryEncoding,
 *              ConvertToUtf8, ConvertToNative, ConvertPath, ConvertExact
 * @return converted wide string on success, NULL on fail with error code stored in errno
 */
char* convert_wcs_to_str(const wchar_t* wstr, unsigned flags)
{
	int is_utf8 = flags & (opt.flags & OPT_UTF8 ? (ConvertToUtf8 | ConvertToPrimaryEncoding) : (ConvertToUtf8 | ConvertToSecondaryEncoding));
	int codepage = (is_utf8 ? CP_UTF8 : (opt.flags & OPT_ENC_DOS) ? CP_OEMCP : CP_ACP);
	/* skip path UNC prefix, if found */
	if ((flags & ConvertPath) && IS_UNC_PREFIX(wstr))
		wstr += UNC_PREFIX_SIZE;
	return wchar_to_cstr(wstr, codepage, (flags & ConvertExact));
}

/**
 * Convert c-string encoding, the encoding is specified by flags.
 *
 * @param str the C-string to convert
 * @param flags bit-mask containing the following bits: ConvertToUtf8, ConvertToNative, ConvertExact
 * @return converted wide string on success, NULL on fail with error code stored in errno
 */
char* convert_str_encoding(const char* str, unsigned flags)
{
	int convert_from = (flags & ConvertToUtf8 ? ConvertNativeToWcs : ConvertUtf8ToWcs) | (flags & ConvertExact);
	wchar_t* wstr;
	assert((flags & ~(ConvertToUtf8 | ConvertToNative | ConvertExact)) == 0); /* disallow invalid flags */
	if ((flags & (ConvertToUtf8 | ConvertToNative)) == 0) {
		errno = EINVAL;
		return NULL; /* error: no conversion needed */
	}
	wstr = convert_str_to_wcs(str, convert_from);
	if (wstr) {
		char* res = convert_wcs_to_str(wstr, flags);
		free(wstr);
		return res;
	}
	return NULL;
}

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
	if (!IS_UNC_PREFIX(wpath) && wcslen(wpath) > 200) {
		DWORD size = GetFullPathNameW(wpath, 0, NULL, NULL);
		if (size > 0) {
			wchar_t* result = (wchar_t*)rsh_malloc((size + UNC_PREFIX_SIZE) * sizeof(wchar_t));
			wcscpy(result, L"\\\\?\\");
			size = GetFullPathNameW(wpath, size, result + UNC_PREFIX_SIZE, NULL);
			if (size > 0)
				return result;
			free(result);
		}
	}
	return NULL;
}

/* the range of error codes for access errors */
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED

/**
 * Convert the GetLastError() value to errno-compatible code.
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
	case ERROR_INVALID_NAME:
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
	case ERROR_INSUFFICIENT_BUFFER:
		return ENOMEM;
	case ERROR_INVALID_ACCESS:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_PARAMETER:
	case ERROR_INVALID_FLAGS:
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
	case ERROR_NO_UNICODE_TRANSLATION:
		return EILSEQ;
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

/* functions to setup/restore console */

/**
 * Restore console on program exit.
 */
static void restore_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE && console_data.saved_cursor_size) {
		/* restore cursor size and visibility */
		cci.dwSize = console_data.saved_cursor_size;
		cci.bVisible = 1;
		SetConsoleCursorInfo(hOut, &cci);
	}
}

/**
 * Hide console cursor.
 */
void hide_cursor(void)
{
	CONSOLE_CURSOR_INFO cci;
	HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE && GetConsoleCursorInfo(hOut, &cci))
	{
		/* store current cursor size and visibility flag */
		console_data.saved_cursor_size = (cci.bVisible ? cci.dwSize : 0);

		/* now hide cursor */
		cci.bVisible = 0;
		SetConsoleCursorInfo(hOut, &cci); /* hide cursor */
		rsh_install_exit_handler(restore_cursor);
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

	console_data.console_flags = 0;
	console_data.saved_cursor_size = 0;
	console_data.primary_codepage = (opt.flags & OPT_UTF8 ? CP_UTF8 : (opt.flags & OPT_ENC_DOS) ? CP_OEMCP : CP_ACP);
	console_data.secondary_codepage = (!(opt.flags & OPT_UTF8) ? CP_UTF8 : (opt.flags & OPT_ENC_DOS) ? CP_OEMCP : CP_ACP);

	/* note: we are using numbers 1 = _fileno(stdout), 2 = _fileno(stderr) */
	/* cause _fileno() is undefined, when compiling as strict ansi C. */
	if (isatty(1))
		console_data.console_flags |= 1;
	if (isatty(2))
		console_data.console_flags |= 2;

	if ((opt.flags & OPT_UTF8) != 0)
	{
		if (console_data.console_flags & 1)
			_setmode(1, _O_U8TEXT);
		if (console_data.console_flags & 2)
			_setmode(2, _O_U8TEXT);

#ifdef USE_GETTEXT
		bind_textdomain_codeset(TEXT_DOMAIN, "utf-8");
#endif /* USE_GETTEXT */
	}
	else
	{
		setlocale(LC_CTYPE, opt.flags & OPT_ENC_DOS ? ".OCP" : ".ACP");
	}
}

wchar_t* get_program_dir(void)
{
	return console_data.program_dir;
}

/**
 * Check if the given stream is connected to console.
 *
 * @param stream the stream to check
 * @return 1 if the stream is connected to console, 0 otherwise
 */
int win_is_console_stream(FILE* stream)
{
	return ((stream == stdout && (console_data.console_flags & 1))
		|| (stream == stderr && (console_data.console_flags & 2)));
}

/**
 * Detect the program directory.
 */
void init_program_dir(void)
{
	DWORD buf_size = sizeof(console_data.program_dir) / sizeof(console_data.program_dir[0]);
	DWORD length;
	length = GetModuleFileNameW(NULL, console_data.program_dir, buf_size);
	if (length == 0 || length >= buf_size) {
		console_data.program_dir[0] = 0;
		return;
	}
	/* remove trailng file name with the last path separator */
	for (; length > 0 && !IS_PATH_SEPARATOR_W(console_data.program_dir[length]); length--);
	for (; length > 0 && IS_PATH_SEPARATOR_W(console_data.program_dir[length]); length--);
	console_data.program_dir[length + 1] = 0;
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
 * Set the locale directory relative to ${PROGRAM_DIR}/locale.
 */
void setup_locale_dir(void)
{
	wchar_t* short_dir;
	char* program_dir = NULL;
	char* locale_dir;
	DWORD buf_size;
	DWORD res;

	if (!console_data.program_dir[0]) return;
	buf_size = GetShortPathNameW(console_data.program_dir, NULL, 0);
	if (!buf_size) return;

	short_dir = (wchar_t*)rsh_malloc(sizeof(wchar_t) * buf_size);
	res = GetShortPathNameW(console_data.program_dir, short_dir, buf_size);
	if (res > 0 && res < buf_size)
		program_dir = convert_wcs_to_str(short_dir, ConvertToPrimaryEncoding);
	free(short_dir);
	if (!program_dir) return;

	locale_dir = make_path(program_dir, "locale", 0);
	free(program_dir);
	if (!locale_dir) return;

	if (is_directory(locale_dir))
		bindtextdomain(TEXT_DOMAIN, locale_dir);
	free(locale_dir);
}
#endif /* USE_GETTEXT */

#define USE_CSTR_ARGS 0
#define USE_WSTR_ARGS 1

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 * @param str_type wide/c-string type of string arguments
 * @param args list of arguments
 * @return the number of characters printed, -1 on error
 */
static int win_vfprintf_encoded(FILE* out, const char* format, int str_type, va_list args)
{
	int res;
	if (!win_is_console_stream(out)) {
		return vfprintf(out, (format ? format : "%s"), args);
	} else if (str_type == USE_CSTR_ARGS) {
		/* thread-unsafe code: using a static buffer */
		static char buffer[8192];
		res = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
		if (res >= 0) {
			wchar_t *wstr = cstr_to_wchar_buffer(buffer, console_data.primary_codepage, console_data.printf_result, sizeof(console_data.printf_result));
			res = (wstr ? fwprintf(out, L"%s", wstr) : -1);
		}
	} else {
		wchar_t* wformat = (!format || (format[0] == '%' && format[1] == 's' && !format[2]) ? L"%s" :
			cstr_to_wchar_buffer(format, console_data.primary_codepage, console_data.format_buffer, sizeof(console_data.format_buffer)));
		res = vfwprintf(out, (wformat ? wformat : L"[UTF8 conversion error]\n"), args);
		assert(str_type == USE_WSTR_ARGS);
	}
	/* fix: win7 incorrectly sets _IOERR(=0x20) flag of the stream for UTF-8 encoding, so clear it */
	if (res >= 0)
		clearerr(out);
	return res;
}

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 * @param args list of arguments
 * @return the number of characters printed, -1 on error
 */
int win_vfprintf(FILE* out, const char* format, va_list args)
{
	return win_vfprintf_encoded(out, format, USE_CSTR_ARGS, args);
}

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 * @return the number of characters printed, -1 on error
 */
int win_fprintf(FILE* out, const char* format, ...)
{
	int res;
	va_list args;
	va_start(args, format);
	res = win_vfprintf_encoded(out, format, USE_CSTR_ARGS, args);
	va_end(args);
	return res;
}

/**
 * Print formatted data to the specified file descriptor,
 * handling proper printing UTF-8 strings to Windows console.
 *
 * @param out file descriptor
 * @param format data format string
 * @return the number of characters printed, -1 on error
 */
int win_fprintf_warg(FILE* out, const char* format, ...)
{
	int res;
	va_list args;
	va_start(args, format);
	res = win_vfprintf_encoded(out, format, USE_WSTR_ARGS, args);
	va_end(args);
	return res;
}

/**
 * Write a buffer to a stream.
 *
 * @param ptr pointer to the buffer to write
 * @param size size of an items in the buffer
 * @param count the number of items to write
 * @param out the stream to write to
 * @return the number of items written, -1 on error
 */
size_t win_fwrite(const void* ptr, size_t size, size_t count, FILE* out)
{
	if (!win_is_console_stream(out))
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
			int res;
			for (i = 0; i < size; i++)
				wstr[i] = (wchar_t)buf[i];
			wstr[size] = L'\0';
			res = fwprintf(out, L"%s", wstr);
			free(wstr);
			if (res < 0)
				return res;
		}
		else
		{
			for (i = 0; (i + 8) <= size; i += 8)
				if (fwprintf(out, L"%C%C%C%C%C%C%C%C", buf[i], buf[i + 1], buf[i + 2],
						buf[i + 3], buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7]) < 0)
					return -1;
			for (; i < size; i++)
				if (fwprintf(out, L"%C", buf[i]) < 0)
					return -1;
		}
		/* fix: win7 incorrectly sets _IOERR(=0x20) flag of the stream for UTF-8 encoding, so clear it */
		clearerr(out);
		return count;
	}
}

#endif /* _WIN32 */
#else
typedef int dummy_declaration_required_by_strict_iso_c;
#endif /* defined(_WIN32) || defined(__CYGWIN__) */
