/* file.h - file abstraction layer */
#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdio.h>
#include <wchar.h> /* for wchar_t */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef wchar_t file_tchar;
# define SYS_PATH_SEPARATOR '\\'
# define ALIEN_PATH_SEPARATOR '/'
# define IS_PATH_SEPARATOR(c) ((c) == '\\' || (c) == '/')
# define IS_PATH_SEPARATOR_W(c) ((c) == L'\\' || (c) == L'/')
#else
typedef  char file_tchar;
# define SYS_PATH_SEPARATOR '/'
# define ALIEN_PATH_SEPARATOR '\\'
# define IS_PATH_SEPARATOR(c) ((c) == '/')
#endif /* _WIN32 */
typedef file_tchar* tpath_t;
typedef const file_tchar* ctpath_t;

/* Generic path functions */
char* make_path(const char* dir, const char* filename, int user_path_separator);
#ifdef _WIN32
tpath_t make_wpath(ctpath_t dir_path, size_t dir_len, ctpath_t sub_path);
# define make_tpath(dir_path, sub_path) make_wpath(dir_path, (size_t)-1, sub_path)
#else
# define make_tpath(dir_path, sub_path) make_path(dir_path, sub_path, 0)
#endif /* _WIN32 */

int is_regular_file(const char* path); /* shall be deprecated */

/**
 * Portable file information.
 */
typedef struct file_t
{
	tpath_t real_path;
	const char* print_path;
#ifdef _WIN32
	const char* native_path; /* print_path in native encoding */
#endif
	char* data;
	uint64_t size;
	uint64_t mtime;
	unsigned mode;
} file_t;

/* bit constants for the file_t.mode bit mask */
enum FileModeBits {
	FileIsDir   = 0x01,
	FileIsLnk   = 0x02,
	FileIsReg   = 0x04,
	FileIsInaccessible = 0x08,
	FileIsRoot  = 0x10,
	FileIsData  = 0x20,
	FileIsList  = 0x40,
	FileIsStdStream = 0x80,
	FileIsStdin = FileIsStdStream,
	FileContentIsUtf8 = 0x100,
	FileInitReusePath = 0x1000,
	FileInitUtf8PrintPath = 0x2000,
	FileInitRunFstat = 0x4000,
	FileInitRunLstat = 0x8000,
	FileInitUpdatePrintPathLastSlash = 0x10000,
	FileInitUpdatePrintPathSlashes = 0x20000,
	FileInitUseRealPathAsIs = 0x40000,
	FileMaskUpdatePrintPath = (FileInitUpdatePrintPathLastSlash | FileInitUpdatePrintPathSlashes),
	FileMaskStatBits = (FileIsDir | FileIsLnk | FileIsReg | FileIsInaccessible),
	FileMaskIsSpecial = (FileIsData | FileIsList | FileIsStdStream),
	FileMaskModeBits = (FileMaskStatBits | FileIsRoot | FileMaskIsSpecial | FileContentIsUtf8)
};

#define FILE_ISDIR(file)   ((file)->mode & FileIsDir)
#define FILE_ISLNK(file)   ((file)->mode & FileIsLnk)
#define FILE_ISREG(file)   ((file)->mode & FileIsReg)
#define FILE_ISBAD(file)   ((file)->mode & FileIsInaccessible)
#define FILE_ISDATA(file)  ((file)->mode & FileIsData)
#define FILE_ISLIST(file)  ((file)->mode & FileIsList)
#define FILE_ISSTDIN(file) ((file)->mode & FileIsStdin)
#define FILE_ISSTDSTREAM(file) ((file)->mode & FileIsStdStream)
#define FILE_ISSPECIAL(file) ((file)->mode & (FileMaskIsSpecial))
#define FILE_IS_IN_UTF8(file) ((file)->mode & (FileContentIsUtf8))

/* file functions */
int file_init(file_t* file, ctpath_t path, unsigned init_flags);
int file_init_by_print_path(file_t* file, file_t* prepend_dir, const char* print_path, unsigned init_flags);
void file_cleanup(file_t* file);
void file_clone(file_t* file, const file_t* orig_file);
void file_swap(file_t* first, file_t* second);
int are_paths_equal(ctpath_t path, struct file_t* file);

enum FileGetPrintPathFlags {
	FPathPrimaryEncoding = 0,
	FPathUtf8 = 1,
	FPathNative = 2,
	FPathBaseName = 4,
	FPathNotNull = 8
};
const char* file_get_print_path(file_t* file, unsigned flags);

enum FileModifyOperations {
	FModifyAppendSuffix,
	FModifyInsertBeforeExtension,
	FModifyRemoveExtension,
	FModifyGetParentDir
};
int file_modify_path(file_t* dst, file_t* src, const char* str, int operation);

enum FileStatModes {
	FNoMode    = 0,
	FUseLstat  = FileInitRunLstat
};
int file_stat(file_t* file, int fstat_flags);

enum FileFOpenModes {
	FOpenRead  = 1,
	FOpenWrite = 2,
	FOpenRW    = 3,
	FOpenBin   = 4,
	FOpenMask  = 7
};
FILE* file_fopen(file_t* file, int fopen_flags);

int file_rename(const file_t* from, const file_t* to);
int file_move_to_bak(file_t* file);
int file_is_readable(file_t* file);

/**
 * A file list iterator.
 */
typedef struct file_list_t
{
	FILE* fd;
	file_t current_file;
	unsigned state;
} file_list_t;

int  file_list_open(file_list_t* list, file_t* file);
int  file_list_read(file_list_t* list);
void file_list_close(file_list_t* list);

#ifndef _WIN32
# define dirent_get_tname(d) ((d)->d_name)
# define rsh_topendir(p) opendir(p)
#else
/* readdir structures and functions */
# define DIR WIN_DIR
# define dirent win_dirent
# define opendir  win_opendir
# define readdir  win_readdir
# define closedir win_closedir
# define dirent_get_tname(d) ((d)->d_wname)
# define rsh_topendir(p) win_wopendir(p)

/* dirent struct for windows to traverse directory content */
struct win_dirent
{
	char*     d_name;   /* file name */
	wchar_t*  d_wname;  /* file name in Unicode (UTF-16) */
	int       d_isdir;  /* non-zero if file is a directory */
};

struct WIN_DIR_t;
typedef struct WIN_DIR_t WIN_DIR;

WIN_DIR* win_opendir(const char*);
WIN_DIR* win_wopendir(const wchar_t*);
struct win_dirent* win_readdir(WIN_DIR*);
void win_closedir(WIN_DIR*);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* FILE_H */
