/* file.c - file abstraction layer */

/* use 64-bit off_t.
 * these macros must be defined before any included file */
#undef _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "file.h"
#include "common_func.h"
#include "win_utils.h"

#if defined( _WIN32) || defined(__CYGWIN__)
# include <windows.h>
# include <share.h> /* for _SH_DENYWR */
# include <fcntl.h>  /* _O_RDONLY, _O_BINARY */
# include <io.h>
#endif

#ifdef __cplusplus
extern "C" {
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
const char* get_basename(const char* path)
{
	const char *p = path + strlen(path) - 1;
	for (; p >= path && !IS_PATH_SEPARATOR(*p); p--);
	return (p+1);
}

/**
 * Return allocated buffer with the directory part of the path.
 * The buffer must be freed by calling free().
 *
 * @param path file path
 * @return directory
 */
char* get_dirname(const char* path)
{
	const char *p = path + strlen(path) - 1;
	char *res;
	for (; p > path && !IS_PATH_SEPARATOR(*p); p--);
	if ((p - path) > 1) {
		res = (char*)rsh_malloc(p-path+1);
		memcpy(res, path, p-path);
		res[p-path] = 0;
		return res;
	} else {
		return rsh_strdup(".");
	}
}

/**
 * Assemble a filepath from its directory and filename.
 *
 * @param dir_path directory path
 * @param filename the file name
 * @return assembled file path
 */
char* make_path(const char* dir_path, const char* filename)
{
	char* buf;
	size_t len;
	assert(dir_path);
	assert(filename);

	/* remove leading path separators from filename */
	while (IS_PATH_SEPARATOR(*filename)) filename++;

	if (dir_path[0] == '.' && dir_path[1] == 0) {
		/* do not extend filename for dir_path="." */
		return rsh_strdup(filename);
	}

	/* copy directory path */
	len = strlen(dir_path);
	buf = (char*)rsh_malloc(len + strlen(filename) + 2);
	strcpy(buf, dir_path);

	/* separate directory from filename */
	if (len > 0 && !IS_PATH_SEPARATOR(buf[len-1])) {
		buf[len++] = SYS_PATH_SEPARATOR;
	}

	/* append filename */
	strcpy(buf+len, filename);
	return buf;
}

#define IS_ANY_SLASH(c) ((c) == RSH_T('/') || (c) == RSH_T('\\'))

/**
 * Compare paths.
 *
 * @param a the first path
 * @param b the second path
 * @return 1 if paths a equal, 0 otherwise
 */
int are_paths_equal(ctpath_t a, ctpath_t b)
{
	if (!a || !b) return 0;
	if (a[0] == RSH_T('.') && IS_ANY_SLASH(a[1])) a += 2;
	if (b[0] == RSH_T('.') && IS_ANY_SLASH(b[1])) b += 2;
	
	for (; *a; ++a, ++b) {
		if (*a != *b && (!IS_ANY_SLASH(*b) || !IS_ANY_SLASH(*a))) {
			/* paths are different */
			return 0;
		}
	}
	/* check if both paths terminated */
	return (*a == *b);
}

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
	file_init(&file, path, FILE_OPT_DONT_FREE_PATH);
	if (file_stat(&file, 0) >= 0) {
		is_regular = FILE_ISREG(&file);
	}
	file_cleanup(&file);
	return is_regular;
}

/**
 * Check if a file exists at the specified path.
 *
 * @param path the path to check
 * @return 1 if file exists, 0 otherwise.
 */
int if_file_exists(const char* path)
{
	int exists;
	file_t file;
	file_init(&file, path, FILE_OPT_DONT_FREE_PATH);
	exists = (file_stat(&file, 0) >= 0);
	file_cleanup(&file);
	return exists;
}

/*=========================================================================
 * file_t functions
 *=========================================================================*/

void file_init(file_t* file, const char* path, int finit_flags)
{
	memset(file, 0, sizeof(*file));
	if ((finit_flags & FILE_OPT_DONT_FREE_PATH) != 0) {
		file->path = (char*)path;
		file->mode = (unsigned)finit_flags;
	} else {
		file->path = rsh_strdup(path);
	}
}

/**
 * Free the memory allocated by the fields of the file_t structure.
 *
 * @param file the file_t structure to clean
 */
void file_cleanup(file_t* file)
{
	if ((file->mode & FILE_OPT_DONT_FREE_PATH) == 0)
		free(file->path);
	file->path = NULL;

#ifdef _WIN32
	if ((file->mode & FILE_OPT_DONT_FREE_WPATH) == 0)
		free(file->wpath);
	file->wpath = NULL;
#endif /* _WIN32 */

	file->mtime = file->size = 0;
	file->mode = 0;
}

/**
 * Append the specified suffix to the src file path.
 *
 * @param dst result of appending
 * @param src the path to append the suffix to
 * @param suffix the suffix to append
 */
void file_path_append(file_t* dst, file_t* src, const char* suffix)
{
	memset(dst, 0, sizeof(*dst));
#ifdef _WIN32
	if (src->wpath)
	{
		wchar_t* wsuffix = c2w(suffix, 0);
		assert(wsuffix != 0);
		size_t src_len = wcslen(src->wpath);
		size_t dst_len = src_len + wcslen(wsuffix) + 1;
		dst->wpath = (wchar_t*)rsh_malloc(dst_len * sizeof(wchar_t));
		wcscpy(dst->wpath, src->wpath);
		wcscpy(dst->wpath + src_len, wsuffix);
		dst->path = w2c(dst->wpath); /* for legacy file handling */
		return;
	}
#endif
	assert(!!file->path);
	size_t src_len = strlen(src->path);
	size_t dst_len = src_len + strlen(suffix) + 1;
	dst->path = (char*)rsh_malloc(dst_len);
	strcpy(dst->path, src->path);
	strcpy(dst->path + src_len, suffix);
}

#ifdef _WIN32
/**
 * Retrieve file information (type, size, mtime) into file_t fields.
 *
 * @param file the file information
 * @return 0 on success, -1 on error
 */
static int file_statw(file_t* file)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	wchar_t* long_path = get_long_path_if_needed(file->wpath);

	/* read file attributes */
	if (GetFileAttributesExW((long_path ? long_path : file->wpath), GetFileExInfoStandard, &data)) {
		uint64_t u;
		file->size  = (((uint64_t)data.nFileSizeHigh) << 32) + data.nFileSizeLow;
		file->mode |= (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_IFDIR : FILE_IFREG);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
			file->mode |= FILE_IFLNK;

		/* the number of 100-nanosecond intervals since January 1, 1601 */
		u = (((uint64_t)data.ftLastWriteTime.dwHighDateTime) << 32) + data.ftLastWriteTime.dwLowDateTime;
		/* convert to second and subtract the epoch difference */
		file->mtime = u / 10000000 - 11644473600LL;
		free(long_path);
		return 0;
	}
	free(long_path);
	set_errno_from_last_file_error();
	return -1;
}
#endif

/**
 * Retrieve file information (type, size, mtime) into file_t fields.
 *
 * @param file the file information
 * @param fstat_flags bitmask consisting of FileStatModes bits
 * @return 0 on success, -1 on error
 */
int file_stat(file_t* file, int fstat_flags)
{
#ifdef _WIN32
	int i;
	(void)fstat_flags; /* ignore on windows */

	file->size  = 0;
	file->mtime = 0;
	file->mode &= (FILE_OPT_DONT_FREE_PATH | FILE_OPT_DONT_FREE_WPATH | FILE_IFROOT | FILE_IFSTDIN);
	if (file->wpath)
		return file_statw(file);

	for (i = 0; i < 2; i++) {
		file->wpath = c2w(file->path, i);
		if (file->wpath == NULL) continue;

		/* return on success */
		if (file_statw(file) == 0) return 0;

		free(file->wpath);
		file->wpath = NULL;
	}
	assert(errno != 0);
	return -1;
#else
	struct stat st;
	int res = 0;
	file->size  = 0;
	file->mtime = 0;
	file->mode  &= (FILE_OPT_DONT_FREE_PATH | FILE_IFROOT | FILE_IFSTDIN);

	if ((fstat_flags & FUseLstat) != 0) {
		if (lstat(file->path, &st) < 0) return -1;
		if (S_ISLNK(st.st_mode))
			file->mode |= FILE_IFLNK; /* it's a symlink */
	}
	else
		res = stat(file->path, &st);

	if (res == 0) {
		file->size  = st.st_size;
		file->mtime = st.st_mtime;

		if (S_ISDIR(st.st_mode)) {
			file->mode |= FILE_IFDIR;
		} else if (S_ISREG(st.st_mode)) {
			/* it's a regular file or a symlink pointing to a regular file */
			file->mode |= FILE_IFREG;
		}
	}
	return res;
#endif
}


/**
 * Retrieve file information (type, size, mtime) into file_t fields.
 *
 * @param file the file information
 * @param fopen_flags bitmask consisting of FileFOpenModes bits
 * @return 0 on success, -1 on error
 */
FILE* file_fopen(file_t* file, int fopen_flags)
{
	const file_tchar* possible_modes[8] = { 0, RSH_T("r"), RSH_T("w"), RSH_T("r+"),
		0, RSH_T("rb"), RSH_T("wb"), RSH_T("r+b") };
	const file_tchar* mode = possible_modes[fopen_flags & FOpenMask];
	assert((fopen_flags & FOpenRW) != 0);
	assert((fopen_flags & FOpenRW) != 0);
#ifdef _WIN32
	if (!file->wpath)
	{
		int i;
		FILE* fd = 0;
		for (i = 0; i < 2; i++) {
			file->wpath = c2w_long_path(file->path, i);
			if (file->wpath == NULL) continue;
			fd = _wfsopen(file->wpath, mode, _SH_DENYNO);
			if (fd || errno != ENOENT) break;
			free(file->wpath);
			file->wpath = 0;
		}
		return fd;
	}
	return _wfsopen(file->wpath, mode, _SH_DENYNO);
#else
	return fopen(file->path, mode);
#endif
}

#ifdef _WIN32
FILE* rsh_tfopen(ctpath_t tpath, file_tchar* tmode)
{
	return _wfsopen(tpath, tmode, _SH_DENYNO);
}
#endif

/**
 * Rename or move the file. The source and destination paths should be on the same device.
 *
 * @param from the source file
 * @param to the destination path
 * @return 0 on success, -1 on error
 */
int file_rename(file_t* from, file_t* to)
{
#ifdef _WIN32
	if (from->wpath && to->wpath)
	{
		/* Windows: file must be removed before overwriting it */
		_wunlink(to->wpath);
		return _wrename(from->wpath, to->wpath);
	}
	assert(from->path && to->path);
	unlink(to->path);
#endif
	return rename(from->path, to->path);
}

#ifdef _WIN32
static int win_can_open_exclusive(wchar_t* wpath)
{
	int fd = _wsopen(wpath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0);
	if (fd < 0) return 0;
	_close(fd);
	return 1;
}

/**
 * Check if given file can be opened with exclusive write access.
 *
 * @param path path to the file
 * @return 1 if file can be opened, 0 otherwise
 */
int file_is_write_locked(file_t* file)
{
	int i, res = 0;
	if (file->wpath)
		return win_can_open_exclusive(file->wpath);
	for (i = 0; i < 2 && !res; i++) {
		file->wpath = c2w_long_path(file->path, i);
		if(file->wpath && win_can_open_exclusive(file->wpath)) return 1;
		free(file->wpath);
	}
	file->wpath = NULL;
	return 0;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

