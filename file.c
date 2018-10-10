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
#if !defined(__CYGWIN__)
# include <share.h> /* for _SH_DENYWR */
#endif
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

/*=========================================================================
 * file_t functions
 *=========================================================================*/

/**
 * Initialize file_t structure, associating it with the given file path.
 *
 * @param file the file_t structure to initialize
 * @param path the file path
 * @param init_flags initialization flags
 */
void file_init(file_t* file, const char* path, int init_flags)
{
	memset(file, 0, sizeof(*file));
	file->mode = (unsigned)init_flags;
	if ((init_flags & FILE_OPT_DONT_FREE_PATH) != 0) {
		file->path = (char*)path;
	} else {
		file->path = rsh_strdup(path);
	}
}

#ifdef _WIN32
/**
 * Initialize file_t structure, associating it with the given file path.
 *
 * @param file the file_t structure to initialize
 * @param tpath the file path
 * @param init_flags initialization flags
 */
void file_tinit(file_t* file, ctpath_t tpath, int init_flags)
{
	memset(file, 0, sizeof(*file));
	file->mode = (unsigned)init_flags & ~FILE_OPT_DONT_FREE_PATH;
	if ((init_flags & FILE_OPT_DONT_FREE_PATH) != 0) {
		file->wpath = (wchar_t*)tpath;
		file->mode |= FILE_OPT_DONT_FREE_WPATH;
	} else {
		file->wpath = rsh_wcsdup(tpath);
	}
}

/**
 * Get the path of the file as a c-string.
 * On Windows lossy unicode conversion can be applied.
 *
 * @param file the file to get the path
 * @return the path of the file
 */
const char* file_cpath(file_t* file)
{
	if (!file->path && file->wpath) file->path = w2c(file->wpath);
	return file->path;
}
#endif

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
void file_path_append(file_t* dst, const file_t* src, const char* suffix)
{
	size_t src_len;
	size_t dst_len;
	memset(dst, 0, sizeof(*dst));
#ifdef _WIN32
	if (src->wpath)
	{
		wchar_t* wsuffix = c2w(suffix, 0);
		assert(wsuffix != 0);
		src_len = wcslen(src->wpath);
		dst_len = src_len + wcslen(wsuffix) + 1;
		dst->wpath = (wchar_t*)rsh_malloc(dst_len * sizeof(wchar_t));
		wcscpy(dst->wpath, src->wpath);
		wcscpy(dst->wpath + src_len, wsuffix);
		dst->path = w2c(dst->wpath); /* for legacy file handling */
		return;
	}
#endif
	assert(!!src->path);
	src_len = strlen(src->path);
	dst_len = src_len + strlen(suffix) + 1;
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
		file->wpath = c2w_long_path(file->path, i);
		if (file->wpath == NULL) continue;
		if (file_statw(file) == 0) return 0; /* success */
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
 * Open the file and return its decriptor.
 *
 * @param file the file information, including the path
 * @param fopen_flags bitmask consisting of FileFOpenModes bits
 * @return file descriptor on success, NULL on error
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

/**
 * Open file at the specified path and return its decriptor.
 *
 * @param tpath the file path
 * @param tmode the mode for file access
 * @return file descriptor on success, NULL on error
 */
FILE* rsh_tfopen(ctpath_t tpath, file_tchar* tmode)
{
#ifdef _WIN32
	return _wfsopen(tpath, tmode, _SH_DENYNO);
#else
	return fopen(tpath, tmode);
#endif
}

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

/**
 * Rename a given file to *.bak, if it exists.
 *
 * @param file the file to move
 * @return 0 on success, -1 on error and errno is set
 */
int file_move_to_bak(file_t* file)
{
	if (file_stat(file, 0) >= 0) {
		int res;
		int save_errno;
		file_t bak_file;
		file_path_append(&bak_file, file, ".bak");
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
 * Check if the given file can't be opened with exclusive write access.
 *
 * @param file the file
 * @return 1 if the file is locked and can't be exclusively opened, 0 otherwise
 */
static int can_not_open_exclusive(wchar_t* wpath)
{
	int fd = _wsopen(wpath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0);
	if (fd < 0) return 1;
	_close(fd);
	return 0;
}

/**
 * Check if given file is write-locked, i.e. can not be opened
 * with exclusive write access.
 *
 * @param file the file
 * @return 1 if file can't be opened, 0 otherwise
 */
int file_is_write_locked(file_t* file)
{
	int i, res = 0;
	if (file->wpath)
		return can_not_open_exclusive(file->wpath);
	for (i = 0; i < 2 && !res; i++) {
		file->wpath = c2w_long_path(file->path, i);
		if(file->wpath && can_not_open_exclusive(file->wpath)) return 1;
		free(file->wpath);
	}
	file->wpath = NULL;
	return 0;
}
#endif


/*=========================================================================
 * file-list functions
 *=========================================================================*/

/**
 * Open a file, containing a list of file paths, to iterate over those paths
 * using the file_list_read() function.
 *
 * @param list the file_list_t structure to initialize
 * @param file_path the file to open
 * @return 0 on success, -1 on error and set errno
 */
int file_list_open(file_list_t* list, file_t* file_path)
{
	memset(list, 0, sizeof(file_list_t));
	if (!!(file_path->mode & FILE_IFSTDIN))
	{
		list->fd = stdin;
		return 0;
	}
	list->fd = file_fopen(file_path, FOpenRead | FOpenBin);
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
	NotFirstLine = 1
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
		char *p;
		char* line = buf;
		char *buf_back = buf + sizeof(buf) - 1;
		/* detect and skip BOM */
		if (buf[0] == (char)0xEF && buf[1] == (char)0xBB && buf[2] == (char)0xBF && !(list->state & NotFirstLine))
			line += 3;
		list->state |= NotFirstLine;
		for (p = line; p < buf_back && *p && *p != '\r' && *p != '\n'; p++);
		*p = 0;
		if (*line == '\0') continue; /* skip empty lines */
		file_init(&list->current_file, line, 0);
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
 * @return pointer to directory stream. On error, NULL is returned,
 *         and errno is set appropriately.
 */
WIN_DIR* win_opendir(const char* dir_path)
{
	WIN_DIR* d;
	wchar_t* wpath;

	/* append '\*' to the dir_path */
	size_t len = strlen(dir_path);
	char *path = (char*)malloc(len + 3);
	if (!path) return NULL; /* failed, malloc also set errno = ENOMEM */
	strcpy(path, dir_path);
	strcpy(path + len, "\\*");

	d = (WIN_DIR*)malloc(sizeof(WIN_DIR));
	if (!d) {
		free(path);
		return NULL;
	}
	memset(d, 0, sizeof(WIN_DIR));

	wpath = c2w_long_path(path, 0);
	d->hFind = (wpath != NULL ?
		FindFirstFileW(wpath, &d->findFileData) : INVALID_HANDLE_VALUE);
	free(wpath);

	if (d->hFind == INVALID_HANDLE_VALUE && GetLastError() != ERROR_ACCESS_DENIED) {
		wpath = c2w_long_path(path, 1); /* try to use secondary codepage */
		if (wpath) {
			d->hFind = FindFirstFileW(wpath, &d->findFileData);
			free(wpath);
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
	wchar_t *wpath = make_pathw(dir_path, (size_t)-1, L"*");
	d = (WIN_DIR*)rsh_malloc(sizeof(WIN_DIR));

	d->hFind = FindFirstFileW(wpath, &d->findFileData);
	free(wpath);
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
	int failed;

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

		if (d->findFileData.cFileName[0] == L'.' &&
			(d->findFileData.cFileName[1] == 0 ||
			(d->findFileData.cFileName[1] == L'.' &&
			d->findFileData.cFileName[2] == 0))) {
				/* simplified implementation, skips '.' and '..' names */
				continue;
		}

		d->dir.d_name = filename = wchar_to_cstr(d->findFileData.cFileName, WIN_DEFAULT_ENCODING, &failed);

		if (filename && !failed) {
			d->dir.d_wname = d->findFileData.cFileName;
			d->dir.d_isdir = (0 != (d->findFileData.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY));
			return &d->dir;
		}
		/* quietly skip an invalid filename and repeat the search */
		if (filename) {
			free(filename);
			d->dir.d_name = NULL;
		}
	}
}
#endif /* _WIN32 */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

