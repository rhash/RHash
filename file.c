/* file.c - file abstraction layer */

/* use 64-bit off_t.
 * these macros must be defined before any included file */
#undef _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include "file.h"
#include "common_func.h"
#include "parse_cmdline.h"
#include "platform.h"
#include "win_utils.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>  /* _O_RDONLY, _O_BINARY, posix_fadvise */

#if defined(_WIN32) || defined(__CYGWIN__)
# include <windows.h>
#if !defined(__CYGWIN__)
# include <share.h> /* for _SH_DENYWR */
#endif
# include <io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IS_ANY_SLASH(c) ((c) == '/' || (c) == '\\')
#define IS_ANY_TSLASH(c) ((c) == RSH_T('/') || (c) == RSH_T('\\'))
#define IS_DOT_STR(s) ((s)[0] == '.' && (s)[1] == 0)
#define IS_DOT_TSTR(s) ((s)[0] == '.' && (s)[1] == 0)

#ifdef _WIN32
/* auxiliary function */
static int str_is_ascii(const char* str)
{
	for (; *str; str++)
		if ((unsigned char)*str >= 0x80)
			return 0;
	return 1;
}
#endif

/*=========================================================================
 * Path functions
 *=========================================================================*/

/**
 * Return file name without path.
 *
 * @param path file path
 * @return file name
 */
static const char* get_basename(const char* path)
{
	const char* p;
	if (!path)
		return NULL;
	for (p = path + strlen(path); p > path && !IS_PATH_SEPARATOR(*(p - 1)); p--);
	return p;
}

/**
 * Return filepath, obtained by concatinating a directory path and a sub-path.
 *
 * @param dir_path (nullable) directory path
 * @param sub_path the filepath to append to the directory
 * @param user_path_separator flag, 1 to use user-defined path separator,
 *        0 to use system path separator
 * @return concatinated file path
 */
char* make_path(const char* dir_path, const char* sub_path, int user_path_separator)
{
	char* buf;
	size_t dir_len;
	assert(sub_path);
	if (sub_path[0] == '.' && IS_ANY_SLASH(sub_path[1]))
		sub_path += 2;
	if (!dir_path)
		return rsh_strdup(sub_path);
	/* remove leading path delimiters from sub_path */
	for (; IS_ANY_SLASH(*sub_path); sub_path++);
	if (dir_path[0] == 0 || IS_DOT_STR(dir_path)) {
		/* do not extend sub_path for dir_path="." */
		return rsh_strdup(sub_path);
	}
	/* remove trailing path delimiters from the directory path */
	for (dir_len = strlen(dir_path); dir_len > 0 && IS_ANY_SLASH(dir_path[dir_len - 1]); dir_len--);
	/* copy directory path */
	buf = (char*)rsh_malloc(dir_len + strlen(sub_path) + 2);
	memcpy(buf, dir_path, dir_len);
	/* insert path separator */
	buf[dir_len++] = (user_path_separator && opt.path_separator ? opt.path_separator : SYS_PATH_SEPARATOR);
	strcpy(buf + dir_len, sub_path); /* append sub_path */
	return buf;
}

#ifdef _WIN32
/**
 * Return wide-string filepath, obtained by concatinating a directory path and a sub-path.
 *
 * @param dir_path (nullable) directory path
 * @param dir_len length of directory path in characters
 * @param sub_path the filepath to append to the directory
 * @return concatinated file path
 */
tpath_t make_wpath(ctpath_t dir_path, size_t dir_len, ctpath_t sub_path)
{
	wchar_t* result;
	size_t len;
	if (dir_path == 0 || IS_DOT_TSTR(dir_path))
		dir_len = 0;
	else {
		if (IS_UNC_PREFIX(sub_path))
			sub_path += UNC_PREFIX_SIZE;
		if (sub_path[0] == L'.' && IS_PATH_SEPARATOR_W(sub_path[1]))
			sub_path += 2;
		/* remove leading path separators from sub_path */
		for (; IS_PATH_SEPARATOR_W(*sub_path); sub_path++);
		if (dir_len == (size_t)-1)
			dir_len = wcslen(dir_path);
	}
	len = wcslen(sub_path);
	result = (wchar_t*)rsh_malloc((dir_len + len + 2) * sizeof(wchar_t));
	if (dir_len > 0) {
		memcpy(result, dir_path, dir_len * sizeof(wchar_t));
		if (result[dir_len - 1] != L'\\' && sub_path[0]) {
			/* append path separator to the directory */
			result[dir_len++] = L'\\';
		}
	}
	/* append sub_path */
	memcpy(result + dir_len, sub_path, (len + 1) * sizeof(wchar_t));
	return result;
}

/**
 * Return wide-string filepath, obtained by concatinating a directory path and a sub-path.
 * Windows UNC path is returned if the resulting path is too long.
 *
 * @param dir_path (nullable) directory path
 * @param sub_path the filepath to append to the directory
 * @return concatinated file path
 */
static tpath_t make_wpath_unc(ctpath_t dir_path, wchar_t* sub_path)
{
	wchar_t* path = make_wpath(dir_path, (size_t)-1, sub_path);
	wchar_t* long_path = get_long_path_if_needed(path);
	if (!long_path)
		return path;
	free(path);
	return long_path;
}
#endif /* _WIN32 */

/**
 * Compare paths.
 *
 * @param path the first path
 * @param file the second path
 * @return 1 if paths a equal, 0 otherwise
 */
int are_paths_equal(ctpath_t path, file_t* file)
{
	ctpath_t fpath;
	if (!path || !file || !file->real_path) return 0;
	fpath = file->real_path;
	if (path[0] == RSH_T('.') && IS_ANY_TSLASH(path[1])) path += 2;
	if (fpath[0] == RSH_T('.') && IS_ANY_TSLASH(fpath[1])) fpath += 2;

	for (; *path; ++path, ++fpath) {
		if (*path != *fpath && (!IS_ANY_TSLASH(*path) || !IS_ANY_TSLASH(*fpath))) {
			/* paths are different */
			return 0;
		}
	}
	/* check if both paths terminated */
	return (*path == *fpath);
}

#ifndef _WIN32
/**
 * Convert a windows file path to a UNIX one, replacing '\\' by '/'.
 *
 * @param path the path to convert
 * @return converted path
 */
static void convert_backslashes_to_unix(char* path)
{
	for (; *path; path++) {
		if (*path == '\\')
			*path = '/';
	}
}
#endif /* _WIN32 */

/**
 * Check if a path points to a regular file.
 *
 * @param path the path to check
 * @return 1 if file exists an is a regular file, 0 otherwise
 */
int is_regular_file(const char* path)
{
	int is_regular = 0;
	file_t file;
	file_init_by_print_path(&file, NULL, path, FileInitReusePath);
	if (file_stat(&file, 0) >= 0) {
		is_regular = FILE_ISREG(&file);
	}
	file_cleanup(&file);
	return is_regular;
}

/*=========================================================================
 * file_t functions
 *=========================================================================*/

enum FileMemoryModeBits {
	FileDontFreeRealPath = 0x1000,
	FileDontFreePrintPath = 0x2000,
	FileDontFreeNativePath = 0x4000,
	FileMemoryModeMask = (FileDontFreeRealPath | FileDontFreePrintPath | FileDontFreeNativePath),
	FileIsAsciiPrintPath = 0x10000,
	FileDontUsePrintPath = 0x20000,
	FileDontUseNativePath = 0x40000,
	FileConversionMask = (FileIsAsciiPrintPath | FileDontUsePrintPath | FileDontUseNativePath)
};

/**
 * Initialize file_t structure, associating it with the given file path.
 *
 * @param file the file_t structure to initialize
 * @param path the file path
 * @param init_flags initialization flags
 */
int file_init(file_t* file, ctpath_t path, unsigned init_flags)
{
#ifdef _WIN32
	tpath_t long_path = get_long_path_if_needed(path);
#endif
	memset(file, 0, sizeof(*file));
	if (path[0] == RSH_T('.') && IS_ANY_TSLASH(path[1]))
		path += 2;
	file->real_path = (tpath_t)path;
	file->mode = (init_flags & FileMaskModeBits) | FileDontFreeRealPath;
	if (((init_flags & FileMaskUpdatePrintPath) && opt.path_separator) IF_WINDOWS( || long_path))
	{
		/* initialize print_path using the path argument */
		if (!file_get_print_path(file, FPathUtf8 | (init_flags & FileMaskUpdatePrintPath)))
		{
			IF_WINDOWS(free(long_path));
			return -1;
		}
	}
#ifdef _WIN32
	if (long_path)
	{
		file->real_path = long_path;
		file->mode = init_flags & FileMaskModeBits;
	}
	else
#endif
	{
		if ((init_flags & FileInitReusePath) == 0)
		{
			file->mode = init_flags & FileMaskModeBits;
			file->real_path = rsh_tstrdup(path);
#ifndef _WIN32
			if ((init_flags & FileInitUseRealPathAsIs) == 0)
				convert_backslashes_to_unix(file->real_path);
#endif
		}
	}
	if ((init_flags & (FileInitRunFstat | FileInitRunLstat)) &&
			file_stat(file, (init_flags & FileInitRunLstat)) < 0)
		return -1;
	return 0;
}

#ifdef _WIN32
static int file_statw(file_t* file);

/**
 * Detect path encoding, by trying file_statw() the file in available encodings.
 * The order of encodings is detected by init_flags bit mask.
 * On success detection file->real_path is allocated.
 *
 * @param file the file to store
 * @param dir_path (nullable) directory path to prepend to printable path
 * @param print_path printable path, which encoding shall be detected
 * @param init_flags bit flags, helping to detect the encoding
 * @return encoding on success, -1 on fail with error code stored in errno
 */
static int detect_path_encoding(file_t* file, wchar_t* dir_path, const char* print_path, unsigned init_flags)
{
	static unsigned encoding_flags[4] = { ConvertUtf8ToWcs | ConvertExact, ConvertNativeToWcs | ConvertExact,
		ConvertUtf8ToWcs, ConvertNativeToWcs };
	wchar_t* last_path = NULL;
	unsigned convert_path = (dir_path ? 0 : ConvertPath);
	int ascii = str_is_ascii(print_path);
	int primary_path_index = ((opt.flags & OPT_UTF8) || (init_flags & FileInitUtf8PrintPath) || ascii ? 0 : 1);
	int step = ((init_flags & FileInitUtf8PrintPath) || ascii ? 2 : 1);
	int i;
	assert(file && !file->real_path);
	file->mode &= ~FileMaskStatBits;
	if (ascii)
		file->mode |= FileIsAsciiPrintPath;
	/* detect encoding in two or four steps */
	for (i = 0; i < 4; i += step) {
		int path_index = i ^ primary_path_index;
		wchar_t* path = convert_str_to_wcs(print_path, encoding_flags[path_index] | convert_path);
		if (!path) {
			if (!last_path)
				continue;
			file->real_path = last_path;
			return primary_path_index;
		}
		if (dir_path) {
			file->real_path = make_wpath_unc(dir_path, path);
			free(path);
		} else
			file->real_path = path;
		if (i < 2) {
			if (file_statw(file) == 0 || errno == EACCES) {
				free(last_path);
				return (path_index & 1);
			}
			if (i == 0) {
				if (step == 2)
					return primary_path_index;
				last_path = file->real_path;
				continue;
			}
			free(file->real_path);
			file->real_path = last_path;
			if(file->real_path)
				return primary_path_index;
		} else if (file->real_path) {
			return (path_index & 1);
		}
		assert(last_path == NULL);
	}
	errno = EILSEQ;
	return -1;
}
#endif

/**
 * Initialize file_t structure from a printable file path.
 *
 * @param file the file_t structure to initialize
 * @param prepend_dir the directory to prepend to the print_path, to construct the file path, can be NULL
 * @param print_path the printable representation of the file path
 * @param init_flags initialization flags
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int file_init_by_print_path(file_t* file, file_t* prepend_dir, const char* print_path, unsigned init_flags)
{
	assert(print_path);
	assert(!prepend_dir || prepend_dir->real_path);
	memset(file, 0, sizeof(file_t));
	file->mode = (init_flags & FileMaskModeBits);
	if (init_flags & (FileIsStdStream | FileIsData)) {
		file->print_path = print_path;
		file->mode |= FileDontFreePrintPath | FileIsAsciiPrintPath;
		return 0;
	}
	if (print_path[0] == '.' && IS_PATH_SEPARATOR(print_path[1]))
		print_path += 2;
#ifdef _WIN32
	{
		const char** primary_path;
		wchar_t* dir_path = (prepend_dir && !IS_DOT_TSTR(prepend_dir->real_path) ? prepend_dir->real_path : NULL);
		int encoding = detect_path_encoding(file, dir_path, print_path, init_flags);
		if (encoding < 0)
			return -1;
		if (encoding == 0) {
			primary_path = &file->print_path;
		} else {
			primary_path = &file->native_path;
		}
		if ((init_flags & (FileInitReusePath | FileMaskUpdatePrintPath)) == FileInitReusePath) {
			*primary_path = print_path;
			file->mode |= (encoding == 0 ? FileDontFreePrintPath : FileDontFreeNativePath);
		} else {
			*primary_path = rsh_strdup(print_path);
		}
	}
#else
	if (!prepend_dir || IS_DOT_STR(prepend_dir->real_path)) {
		file_init(file, print_path, init_flags & (FileInitReusePath | FileMaskModeBits));
	} else {
		file->real_path = make_path(prepend_dir->real_path, print_path, 0);
		file->mode = init_flags & FileMaskModeBits;
	}
	assert(file->print_path == NULL);
	if ((init_flags & (FileInitReusePath | FileMaskUpdatePrintPath)) == FileInitReusePath) {
		file->print_path = print_path;
		file->mode |= FileDontFreePrintPath;
	} else {
		file->print_path = rsh_strdup(print_path);
	}
#endif
	/* note: FileMaskUpdatePrintPath flags are used only with file_init() */
	assert((init_flags & FileMaskUpdatePrintPath) == 0);
	if ((init_flags & (FileInitRunFstat | FileInitRunLstat)) &&
			file_stat(file, (init_flags & FileInitRunLstat)) < 0)
		return -1;
	return 0;
}

/**
 * Transform the given file path, according to passed flags.
 *
 * @param path the file path to transform
 * @param flags bitmask containing FPathBaseName, FPathNotNull and FileMaskUpdatePrintPath bit flags
 * @return transformed path
 */
static const char* handle_rest_of_path_flags(const char* path, unsigned flags)
{
	if (path == NULL)
		return ((flags & FPathNotNull) ? (errno == EINVAL ? "(null)" : "(encoding error)") : NULL);
	if ((flags & FileMaskUpdatePrintPath) != 0 && opt.path_separator) {
		char* p = (char*)path - 1 + strlen(path);
		for (; p >= path; p--) {
			if (IS_ANY_SLASH(*p)) {
				*p = opt.path_separator;
				if ((flags & FileInitUpdatePrintPathLastSlash) != 0)
					break;
			}
		}
	}
	return (flags & FPathBaseName ? get_basename(path) : path);
}

/**
 * Get the print path of the file in utf8 or in a native encoding.
 * Transformations specified by flags are applied.
 * Encoding conversion on Windows can be lossy.
 *
 * @param file the file to get the path
 * @param flags bitmask containing FPathUtf8, FPathNative, FPathBaseName, FPathNotNull
 *              and FileMaskUpdatePrintPath bit flags
 * @return transformed print path of the file. If FPathNotNull flag is not specified,
 *         then NULL is returned on function fail with error code stored in errno.
 *         If FPathNotNull flag is set, then error code is transformed to returned string.
 */
const char* file_get_print_path(file_t* file, unsigned flags)
{
#ifdef _WIN32
	unsigned convert_to;
	unsigned dont_use_bit;
	int is_utf8 = (opt.flags & OPT_UTF8 ? !(flags & FPathNative) : flags & FPathUtf8);
	const char* secondary_path;
	const char** primary_path = (is_utf8 || (file->mode & FileIsAsciiPrintPath) ? &file->print_path : &file->native_path);
	if (*primary_path)
		return handle_rest_of_path_flags(*primary_path, flags);
	if (is_utf8) {
		convert_to = ConvertToUtf8;
		dont_use_bit = FileDontUsePrintPath;
		secondary_path = file->native_path;
	} else {
		convert_to = ConvertToNative;
		dont_use_bit = FileDontUseNativePath;
		secondary_path = file->print_path;
	}
	if (secondary_path) {
		if ((file->mode & dont_use_bit) == 0) {
			*primary_path = convert_str_encoding(secondary_path, convert_to);
			if (!*primary_path)
				file->mode |= dont_use_bit;
		} else
			errno = EILSEQ;
		return handle_rest_of_path_flags(*primary_path, flags);
	}
	if (!file->real_path) {
		errno = EINVAL;
		return handle_rest_of_path_flags(NULL, flags);
	}
	*primary_path = convert_wcs_to_str(file->real_path, convert_to | ConvertPath);
	if (!*primary_path)
		return handle_rest_of_path_flags(NULL, flags);
	if (str_is_ascii(*primary_path)) {
		file->mode |= FileIsAsciiPrintPath;
		if (primary_path != &file->print_path) {
			file->print_path = *primary_path;
			file->native_path = NULL;
			primary_path = &file->print_path;
		}
	}
	return handle_rest_of_path_flags(*primary_path, flags);
#else
	if (!file->print_path && !file->real_path)
		errno = EINVAL;
	if (!file->print_path && (flags & FileMaskUpdatePrintPath))
		file->print_path = rsh_strdup(file->real_path);
	return handle_rest_of_path_flags((file->print_path ? file->print_path : file->real_path), flags);
#endif
}

/**
 * Free the memory allocated by the fields of the file_t structure.
 *
 * @param file the file_t structure to clean
 */
void file_cleanup(file_t* file)
{
	if (!(file->mode & FileDontFreeRealPath))
		free(file->real_path);
	file->real_path = NULL;
	if (!(file->mode & FileDontFreePrintPath))
		free((char*)file->print_path);
	file->print_path = NULL;

#ifdef _WIN32
	if ((file->mode & FileDontFreeNativePath) == 0)
		free((char*)file->native_path);
	file->native_path = NULL;
#endif /* _WIN32 */

	free(file->data);
	file->data = NULL;
	file->mtime = 0;
	file->size = 0;
	file->mode = 0;
}

/**
 * Clone existing file_t structure to another.
 *
 * @param file the file_t structure to clone to
 * @param orig_file the file to clone
 */
void file_clone(file_t* file, const file_t* orig_file)
{
	memset(file, 0, sizeof(*file));
	file->mode = orig_file->mode & FileMaskModeBits;
	if (orig_file->real_path)
		file->real_path = rsh_tstrdup(orig_file->real_path);
	if (orig_file->print_path)
		file->print_path = rsh_strdup(orig_file->print_path);
#ifdef _WIN32
	if (orig_file->native_path)
		file->native_path = rsh_strdup(orig_file->native_path);
#endif
}

/**
 * Swap members of two file_t structures.
 *
 * @param first the first file
 * @param second the second file
 */
void file_swap(file_t* first, file_t* second)
{
	file_t tmp;
	memcpy(&tmp, first, sizeof(file_t));
	memcpy(first, second, sizeof(file_t));
	memcpy(second, &tmp, sizeof(file_t));
}

/**
 * Get a modified file path.
 *
 * @param path the file path to modify
 * @param str the string to insert into/append to the source file path
 * @param operation the operation determinating how to modify the file path, can be one of the values
 *                  FModifyAppendSuffix, FModifyInsertBeforeExtension, FModifyRemoveExtension, FModifyGetParentDir
 * @return allocated and modified file path on success, NULL on fail
 */
static char* get_modified_path(const char* path, const char* str, int operation)
{
	size_t start_pos = (size_t)-1;
	size_t end_pos = (size_t)-1;
	if (!path)
		return NULL;
	if (operation != FModifyAppendSuffix) {
		if (operation == FModifyGetParentDir) {
			end_pos = strlen(path);
			start_pos = (end_pos > 0 ? end_pos - 1 : 0);
			for (; start_pos > 0 && !IS_ANY_SLASH(path[start_pos]); start_pos--);
			if (start_pos == 0 && !IS_ANY_SLASH(path[start_pos]))
				return rsh_strdup(".");
			for (; start_pos > 0 && IS_ANY_SLASH(path[start_pos]); start_pos--);
			start_pos++;
		} else {
			char* point = strrchr(path, '.');
			if (!point)
				return NULL;
			start_pos = point - path;
			if (operation == FModifyInsertBeforeExtension)
				end_pos = start_pos;
		}
	}
	return str_replace_n(path, start_pos, end_pos, str);
}

#ifdef _WIN32
/**
 * Get a modified file path.
 *
 * @param path the file path to modify
 * @param str the string to insert into/append to the source file path
 * @param operation the operation determinating how to modify the file path, can be one of the values
 *                  FModifyAppendSuffix, FModifyInsertBeforeExtension, FModifyRemoveExtension, FModifyGetParentDir
 * @return allocated and modified file path on success, NULL on fail
 */
static tpath_t get_modified_tpath(ctpath_t path, const char* str, int operation)
{
	size_t start_pos = (size_t)-1;
	size_t end_pos = (size_t)-1;
	if (!path)
		return NULL;
	if (operation != FModifyAppendSuffix) {
		if (operation == FModifyGetParentDir) {
			end_pos = wcslen(path);
			start_pos = (end_pos > 0 ? end_pos - 1 : 0);
			for (; start_pos > 0 && !IS_ANY_TSLASH(path[start_pos]); start_pos--);
			if (start_pos == 0 && !IS_ANY_TSLASH(path[start_pos]))
				return rsh_wcsdup(L".");
			for (; start_pos > 0 && IS_ANY_TSLASH(path[start_pos]); start_pos--);
			start_pos++;
		} else {
			rsh_tchar* point = wcsrchr(path, L'.');
			if (!point)
				return NULL;
			start_pos = point - path;
			if (operation == FModifyInsertBeforeExtension)
				end_pos = start_pos;
		}
	}
	return wcs_replace_n(path, start_pos, end_pos, str);
}
#else
# define get_modified_tpath get_modified_path
#endif

/**
 * Initialize a (destination) file by modifying the path of another (source) file.
 *
 * @param dst destination file
 * @param src source file
 * @param str the string to insert into/append to the source file path
 * @param operation the operation to do on src file, can be one of the values
 *                  FModifyAppendSuffix, FModifyInsertBeforeExtension, FModifyRemoveExtension, FModifyGetParentDir
 * @return 0 on success, -1 on fail
 */
int file_modify_path(file_t* dst, file_t* src, const char* str, int operation)
{
	if ((src->mode & (FileIsStdStream | FileIsData)) != 0)
		return -1;
	assert(operation == FModifyRemoveExtension || operation == FModifyGetParentDir || str);
	assert(operation == FModifyAppendSuffix || operation == FModifyInsertBeforeExtension || !str);
	memcpy(dst, src, sizeof(file_t));
	dst->mode &= ~FileMemoryModeMask;
	dst->print_path = NULL;
	IF_WINDOWS(dst->native_path = NULL);
	dst->real_path = get_modified_tpath(src->real_path, str, operation);
	if (!dst->real_path)
		return -1;
	dst->print_path = get_modified_path(src->print_path, str, operation);
	IF_WINDOWS(dst->native_path = get_modified_path(src->native_path, str, operation));
	return 0;
}

#ifdef _WIN32
/**
 * Retrieve file information (type, size, mtime) into file_t fields.
 *
 * @param file the file information
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int file_statw(file_t* file)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	/* read file attributes */
	if (GetFileAttributesExW(file->real_path, GetFileExInfoStandard, &data)) {
		uint64_t u;
		file->size  = (((uint64_t)data.nFileSizeHigh) << 32) + data.nFileSizeLow;
		file->mode |= (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FileIsDir : FileIsReg);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
			file->mode |= FileIsLnk;

		/* the number of 100-nanosecond intervals since January 1, 1601 */
		u = (((uint64_t)data.ftLastWriteTime.dwHighDateTime) << 32) + data.ftLastWriteTime.dwLowDateTime;
		/* convert to seconds and subtract the epoch difference */
		file->mtime = u / 10000000 - 11644473600LL;
		return 0;
	}
	file->mode |= FileIsInaccessible;
	set_errno_from_last_file_error();
	return -1;
}
#endif

/**
 * Retrieve file information (type, size, mtime) into file_t fields.
 *
 * @param file the file information
 * @param fstat_flags bitmask consisting of FileStatModes bits
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int file_stat(file_t* file, int fstat_flags)
{
#ifdef _WIN32
	(void)fstat_flags; /* ignore on windows */
#else
	struct stat st;
#endif
	file->size  = 0;
	file->mtime = 0;
	file->mode &= ~FileMaskStatBits;
	if (FILE_ISDATA(file) || FILE_ISSTDSTREAM(file))
		return 0;
	else if (!file->real_path) {
		file->mode |= FileIsInaccessible;
		errno = EINVAL;
		return -1;
	}
#ifdef _WIN32
	return file_statw(file);
#else
	if (stat(file->real_path, &st)) {
		file->mode |= FileIsInaccessible;
		return -1;
	}
	file->size  = st.st_size;
	file->mtime = st.st_mtime;

	if (S_ISDIR(st.st_mode)) {
		file->mode |= FileIsDir;
	} else if (S_ISREG(st.st_mode)) {
		/* it's a regular file or a symlink pointing to a regular file */
		file->mode |= FileIsReg;
	}

	if ((fstat_flags & FUseLstat) && lstat(file->real_path, &st) == 0) {
		if (S_ISLNK(st.st_mode))
			file->mode |= FileIsLnk; /* it's a symlink */
	}
	return 0;
#endif
}

/**
 * Open the file and return its decriptor.
 *
 * @param file the file information, including the path
 * @param fopen_flags bitmask consisting of FileFOpenModes bits
 * @return file descriptor on success, NULL on fail with error code stored in errno
 */
FILE* file_fopen(file_t* file, int fopen_flags)
{
	const file_tchar* possible_modes[8] = { 0, RSH_T("r"), RSH_T("w"), RSH_T("r+"),
		0, RSH_T("rb"), RSH_T("wb"), RSH_T("r+b") };
	const file_tchar* mode = possible_modes[fopen_flags & FOpenMask];
	FILE* fd;
	assert((fopen_flags & FOpenRW) != 0);
	if (!file->real_path) {
		errno = EINVAL;
		return NULL;
	}
#ifdef _WIN32
	{
		fd = _wfsopen(file->real_path, mode, _SH_DENYNO);
		if (!fd && errno == EINVAL)
			errno = ENOENT;
		return fd;
	}
#else
	fd = fopen(file->real_path, mode);
# if _POSIX_C_SOURCE >= 200112L && !defined(__STRICT_ANSI__)
	if(fd)
		posix_fadvise(fileno(fd), 0, 0, POSIX_FADV_SEQUENTIAL);
# endif /* _POSIX_C_SOURCE >= 200112L && !defined(__STRICT_ANSI__) */
	return fd;
#endif
}

/**
 * Rename or move the file. The source and destination paths should be on the same device.
 *
 * @param from the source file
 * @param to the destination path
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int file_rename(const file_t* from, const file_t* to)
{
#ifdef _WIN32
	if (!from->real_path || !to->real_path) {
		errno = EINVAL;
		return -1;
	}
	/* Windows: file must be removed before overwriting it */
	_wunlink(to->real_path);
	return _wrename(from->real_path, to->real_path);
#else
	return rename(from->real_path, to->real_path);
#endif
}

/**
 * Rename a given file to *.bak, if it exists.
 *
 * @param file the file to move
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int file_move_to_bak(file_t* file)
{
	if (file_stat(file, 0) >= 0) {
		int res;
		int save_errno;
		file_t bak_file;
		file_modify_path(&bak_file, file, ".bak", FModifyAppendSuffix);
		res = file_rename(file, &bak_file);
		save_errno = errno;
		file_cleanup(&bak_file);
		if (res < 0)
			errno = save_errno;
		return res;
	}
	return -1;
}

#ifdef _WIN32
/**
 * Check if the specified path points to a readable file.
 *
 * @param real_path file path
 * @param is_readable pointer to the result, it is set to 1, if the file is readable, to 0 otherwise
 * @return 1 if the file with such path exists, 0 otherwise
 */
static int real_path_is_readable(wchar_t* real_path, int* is_readable)
{
	/* note: using _wsopen, since _waccess doesn't check permissions */
	int fd = _wsopen(real_path, _O_RDONLY | _O_BINARY, _SH_DENYNO);
	*is_readable = (fd >= 0);
	if (fd >= 0) {
		_close(fd);
		return 1;
	}
	return (errno == EACCES);
}
#endif

/**
 * Check if the given file can't be opened for reading.
 *
 * @param file the file
 * @return 1 if the file can be opened for reading, 0 otherwise
 */
int file_is_readable(file_t* file)
{
#ifdef _WIN32
	if (file->real_path) {
		int is_readable;
		(void)real_path_is_readable(file->real_path, &is_readable);
		return is_readable;
	}
	return 0;
#else
	return (access(file->real_path, R_OK) == 0);
#endif
}


/*=========================================================================
 * file-list functions
 *=========================================================================*/

/**
 * Open a file, containing a list of file paths, to iterate over those paths
 * using the file_list_read() function.
 *
 * @param list the file_list_t structure to initialize
 * @param file the file to open
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int file_list_open(file_list_t* list, file_t* file)
{
	memset(list, 0, sizeof(file_list_t));
	if (FILE_ISSTDIN(file)) {
		list->fd = stdin;
		return 0;
	}
	list->fd = file_fopen(file, FOpenRead | FOpenBin);
	return (list->fd ? 0 : -1);
}

/**
 * Close file_list_t and free allocated memory.
 */
void file_list_close(file_list_t* list)
{
	if (list->fd) {
		fclose(list->fd);
		list->fd = 0;
	}
	file_cleanup(&list->current_file);
}

enum FileListStateBits {
	NotFirstLine = 1,
	FileListHasBom = FileInitUtf8PrintPath
};

/**
 * Iterate over file list.
 *
 * @param list the file list to iterate over
 * @return 1 if the next file has been obtained, 0 on EOF or error
 */
int file_list_read(file_list_t* list)
{
	char buf[2048];
	file_cleanup(&list->current_file);
	while(fgets(buf, 2048, list->fd)) {
		char* p;
		char* line = buf;
		char* buf_back = buf + sizeof(buf) - 1;
		/* detect and skip BOM */
		if (STARTS_WITH_UTF8_BOM(buf)) {
			line += 3;
			if (!(list->state & NotFirstLine))
				list->state |= FileListHasBom;
		}
		list->state |= NotFirstLine;
		for (p = line; p < buf_back && *p && *p != '\r' && *p != '\n'; p++);
		*p = 0;
		if (*line == '\0')
			continue; /* skip empty lines */
		file_init_by_print_path(&list->current_file, NULL, line,
			(list->state & FileInitUtf8PrintPath) | FileInitRunFstat);
		return 1;
	}
	return 0;
}

/****************************************************************************
 *                           Directory functions                            *
 ****************************************************************************/
#ifdef _WIN32
struct WIN_DIR_t
{
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	struct win_dirent dir;
	int state; /* 0 - not started, -1 - ended, >=0 file index */
};

/**
 * Open directory iterator for reading the directory content.
 *
 * @param dir_path directory path
 * @return pointer to directory stream, NULL on fail with error code stored in errno
 */
WIN_DIR* win_opendir(const char* dir_path)
{
	WIN_DIR* d;
	wchar_t* real_path;

	/* append '\*' to the dir_path */
	size_t len = strlen(dir_path);
	char* path = (char*)malloc(len + 3);
	if (!path) return NULL; /* failed, malloc also set errno = ENOMEM */
	strcpy(path, dir_path);
	strcpy(path + len, "\\*");

	d = (WIN_DIR*)malloc(sizeof(WIN_DIR));
	if (!d) {
		free(path);
		return NULL;
	}
	memset(d, 0, sizeof(WIN_DIR));

	real_path = convert_str_to_wcs(path, (ConvertToPrimaryEncoding | ConvertExact | ConvertPath));
	d->hFind = (real_path != NULL ?
		FindFirstFileW(real_path, &d->findFileData) : INVALID_HANDLE_VALUE);
	free(real_path);

	if (d->hFind == INVALID_HANDLE_VALUE && GetLastError() != ERROR_ACCESS_DENIED) {
		/* try the secondary codepage */
		real_path = convert_str_to_wcs(path, (ConvertToSecondaryEncoding | ConvertExact | ConvertPath));
		if (real_path) {
			d->hFind = FindFirstFileW(real_path, &d->findFileData);
			free(real_path);
		}
	}
	free(path);

	if (d->hFind == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED) {
		free(d);
		errno = EACCES;
		return NULL;
	}
	set_errno_from_last_file_error();

	d->state = (d->hFind == INVALID_HANDLE_VALUE ? -1 : 0);
	d->dir.d_name = NULL;
	return d;
}

/**
 * Open a directory for reading its content.
 * For simplicity the function supposes that dir_path points to an
 * existing directory and doesn't check for this error.
 * The Unicode version of the function.
 *
 * @param dir_path directory path
 * @return pointer to directory iterator
 */
WIN_DIR* win_wopendir(const wchar_t* dir_path)
{
	WIN_DIR* d;

	/* append '\*' to the dir_path */
	wchar_t* real_path = make_wpath_unc(dir_path, L"*");
	d = (WIN_DIR*)rsh_malloc(sizeof(WIN_DIR));

	d->hFind = FindFirstFileW(real_path, &d->findFileData);
	free(real_path);
	if (d->hFind == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED) {
		free(d);
		errno = EACCES;
		return NULL;
	}

	/* note: we suppose if INVALID_HANDLE_VALUE was returned, then the file listing is empty */
	d->state = (d->hFind == INVALID_HANDLE_VALUE ? -1 : 0);
	d->dir.d_name = NULL;
	return d;
}

/**
 * Close a directory iterator.
 *
 * @param d pointer to the directory iterator
 */
void win_closedir(WIN_DIR* d)
{
	if (d->hFind != INVALID_HANDLE_VALUE) {
		FindClose(d->hFind);
	}
	free(d->dir.d_name);
	free(d);
}

/**
 * Read a directory content.
 *
 * @param d pointer to the directory iterator
 * @return directory entry or NULL if no entries left
 */
struct win_dirent* win_readdir(WIN_DIR* d)
{
	char* filename;

	if (d->state == -1) return NULL;
	if (d->dir.d_name != NULL) {
		free(d->dir.d_name);
		d->dir.d_name = NULL;
	}

	for (;;) {
		if (d->state > 0) {
			if ( !FindNextFileW(d->hFind, &d->findFileData) ) {
				/* the directory listing has ended */
				d->state = -1;
				return NULL;
			}
		}
		d->state++;

		if (d->findFileData.cFileName[0] == L'.' && (d->findFileData.cFileName[1] == 0 ||
				(d->findFileData.cFileName[1] == L'.' && d->findFileData.cFileName[2] == 0)))
			continue; /* simplified implementation, skips '.' and '..' names */

		d->dir.d_name = filename = convert_wcs_to_str(d->findFileData.cFileName, (ConvertToPrimaryEncoding | ConvertExact));
		if (filename) {
			d->dir.d_wname = d->findFileData.cFileName;
			d->dir.d_isdir = (0 != (d->findFileData.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY));
			return &d->dir;
		}
		/* quietly skip the file and repeat the search, if filename conversion failed */
	}
}
#endif /* _WIN32 */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

