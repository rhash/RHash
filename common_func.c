/* common_func.c */

#include "common_func.h" /* should be included before the C library files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <wchar.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "win_utils.h"
#include "parse_cmdline.h"

/**
 * Print a 0-terminated string representation of a 64-bit number to
 * a string buffer.
 *
 * @param dst the string buffer to write the number to
 * @param number the 64-bit number to output
 * @param min_width the minimum width, the number must take
 */
void sprintI64(char *dst, uint64_t number, int min_width)
{
	char buf[24]; /* internal buffer to output the number to */
	size_t len;
	char *p = buf + 23; /* start filling from the buffer end */
	*(p--) = 0; /* last symbol should be '\0' */
	if(number == 0) {
		*(p--) = '0';
	} else {
		for(; p >= buf && number != 0; p--, number /= 10) {
			*p = '0' + (char)(number % 10);
		}
	}
	len = buf + 22 - p;
	if((size_t)min_width > len) {
		memset(dst, 0x20, min_width - len); /* fill by spaces */
		dst += min_width - len;
	}
	memcpy(dst, p+1, len+1); /* copy the number to the output buffer */
}

/**
 * Calculate length of decimal representation of given 64-bit integer.
 *
 * @param num integer to calculate the length for
 * @return length of decimal representation
 */
int int_len(uint64_t num)
{
	int len;
	for(len = 0; num; len++, num /= 10);
	return (len == 0 ? 1 : len); /* note: int_len(0) == 1 */
}

/**
 * Convert a byte to a hexadecimal string. The result, consisting of two
 * hexadecimal digits is stored into a buffer.
 *
 * @param dst  the buffer to receive two symbols of hex representation
 * @param byte the byte to decode
 * @param upper_case flag to print string in uppercase
 * @return pointer to the next char in buffer (dst+2)
 */
static char* print_hex_byte(char *dst, const unsigned char byte, int upper_case)
{
	const char add = (upper_case ? 'A' - 10 : 'a' - 10);
	unsigned char c = (byte >> 4) & 15;
	*dst++ = (c > 9 ? c + add : c + '0');
	c = byte & 15;
	*dst++ = (c > 9 ? c + add : c + '0');
	return dst;
}

/* unsafe characters are "<>{}[]%#/|\^~`@:;?=&+ */
#define IS_GOOD_URL_CHAR(c) (isalnum((unsigned char)c) || strchr("$-_.!'(),", c))

/**
 * URL-encode a string.
 *
 * @param dst buffer to receive result or NULL to calculate
 *    the lengths of encoded string
 * @param filename the file name
 * @return the length of the result string
 */
int urlencode(char *dst, const char *name)
{
	const char *start;
	if(!dst) {
		int len;
		for(len = 0; *name; name++) len += (IS_GOOD_URL_CHAR(*name) ? 1 : 3);
		return len;
	}
	/* encode URL as specified by RFC 1738 */
	for(start = dst; *name; name++) {
		if( IS_GOOD_URL_CHAR(*name) ) {
			*dst++ = *name;
		} else {
			*dst++ = '%';
			dst = print_hex_byte(dst, *name, 'A');
		}
	}
	*dst = 0;
	return (int)(dst - start);
}

/**
 * Convert given string to lower case.
 * The result string will be allocated by malloc.
 * The allocated memory should be freed by calling free().
 *
 * @param str a string to convert
 * @return converted string allocated by malloc
 */
char* str_tolower(const char* str)
{
	char* buf = rsh_strdup(str);
	char* p;
	if(buf) {
		for(p = buf; *p; p++) *p = tolower(*p);
	}
	return buf;
}

/**
 * Remove spaces from the begin and the end of the string.
 *
 * @param str the modifiable buffer with the string
 * @return trimmed string
 */
char* str_trim(char* str)
{
	char* last = str + strlen(str) - 1;
	while(isspace((unsigned char)*str)) str++;
	while(isspace((unsigned char)*last) && last > str) *(last--) = 0;
	return str;
}

/**
 * Fill a buffer with NULL-terminated string consisting
 * solely of a given repeated character.
 *
 * @param buf  the modifiable buffer to fill
 * @param ch   the character to fill string with
 * @param length the length of the string to construct
 * @return the buffer
 */
char* str_set(char* buf, int ch, int length)
{
	memset(buf, ch, length);
	buf[length] = '\0';
	return buf;
}

/**
 * Concatenates two strings and returns allocated buffer with result.
 *
 * @param orig original string
 * @param append the string to append
 * @return the buffer
 */
char* str_append(const char* orig, const char* append)
{
	size_t len1 = strlen(orig);
	size_t len2 = strlen(append);
	char* res = (char*)rsh_malloc(len1 + len2 + 1);

	/* concatenate two strings */
	memcpy(res, orig, len1);
	memcpy(res + len1, append, len2 + 1);
	return res;
}

/**
 * Check if a string is a binary string, which means the string contain
 * a character with ACII code below 0x20 other than '\r', '\n', '\t'.
 *
 * @param str a string to check
 * @return non zero if string is binary
 */
int is_binary_string(const char* str)
{
	for(; *str; str++) {
		if(((unsigned char)*str) < 32 && ((1 << (unsigned char)*str) & ~0x2600)) {
			return 1;
		}
	}
	return 0;
}

/**
 * Count number of utf8 characters in a 0-terminated string
 *
 * @param str the string to measure
 * @return number of utf8 characters in the string
 */
size_t strlen_utf8_c(const char *str)
{
	size_t length = 0;
	for(; *str; str++) {
		if((*str & 0xc0) != 0x80) length++;
	}
	return length;
}

/**
* Exit the program, with restoring console state.
*
* @param code the program exit code
*/
void rhash_exit(int code)
{
	IF_WINDOWS(restore_console());
	exit(code);
}

/* FILE FUNCTIONS */

/**
 * Return filename without path.
 *
 * @param path file path
 * @return filename
 */
const char* get_basename(const char* path)
{
	const char *p = path + strlen(path) - 1;
	for(; p >= path && !IS_PATH_SEPARATOR(*p); p--);
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
	for(; p > path && !IS_PATH_SEPARATOR(*p); p--);
	if((p - path) > 1) {
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
 * @param filename file name
 * @return filepath
 */
char* make_path(const char* dir_path, const char* filename)
{
	char* buf;
	size_t len;
	assert(dir_path);
	assert(filename);

	/* remove leading path separators from filename */
	while(IS_PATH_SEPARATOR(*filename)) filename++;

	/* copy directory path */
	len = strlen(dir_path);
	buf = (char*)rsh_malloc(len + strlen(filename) + 2);
	strcpy(buf, dir_path);

	/* separate directory from filename */
	if(len > 0 && !IS_PATH_SEPARATOR(buf[len-1])) {
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
 */
int are_paths_equal(const rsh_tchar* a, const rsh_tchar* b)
{
	if (!a || !b) return 0;
	if (a[0] == RSH_T('.') && IS_ANY_SLASH(a[1])) a += 2;
	if (b[0] == RSH_T('.') && IS_ANY_SLASH(b[1])) b += 2;
	
	for (; *a; ++a, ++b)
	{
		if (*a != *b && (!IS_ANY_SLASH(*b) || !IS_ANY_SLASH(*a)))
		{
			/* paths are different */
			return 0;
		}
		
	}
	/* check if both paths terminated */
	return (*a == *b);
}

/**
 * Print time formated as hh:mm.ss YYYY-MM-DD to a file stream.
 *
 * @param out the stream to print time to
 * @param time the time to print
 */
void print_time(FILE *out, time_t time)
{
	struct tm *t = localtime(&time);
	static struct tm zero_tm;
	if(t == NULL) {
		/* if strange day, then print `00:00.00 1900-01-00' */
		t = &zero_tm;
		t->tm_hour = t->tm_min = t->tm_sec =
		t->tm_year = t->tm_mon = t->tm_mday = 0;
	}
	fprintf(out, "%02u:%02u.%02u %4u-%02u-%02u", t->tm_hour, t->tm_min,
		t->tm_sec, (1900+t->tm_year), t->tm_mon+1, t->tm_mday);
}

void print_time64(FILE *out, uint64_t time)
{
	print_time(out, (time_t)time);
}

/**
 * Return ticks in milliseconds for time intervals measurement.
 * This function should be not precise but the fastest one
 * to retrieve internal clock value.
 *
 * @return ticks count in milliseconds
 */
unsigned rhash_get_ticks(void)
{
#ifdef _WIN32
	return GetTickCount();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

#ifdef _WIN32
/**
 * Fill file information in the file_t structure.
 *
 * @param file the file information
 * @return 0 on success, -1 on error
 */
int rsh_file_statw(file_t* file)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	/* read file attributes */
	if(GetFileAttributesExW(file->wpath, GetFileExInfoStandard, &data)) {
		uint64_t u;
		file->size  = (((uint64_t)data.nFileSizeHigh) << 32) + data.nFileSizeLow;
		file->mode = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_IFDIR : 0);

		/* the number of 100-nanosecond intervals since January 1, 1601 */
		u = (((uint64_t)data.ftLastWriteTime.dwHighDateTime) << 32) + data.ftLastWriteTime.dwLowDateTime;
		/* convert to second and subtract the epoch difference */
		file->mtime = u / 10000000 - 11644473600LL;
		return 0;
	}
	set_errno_from_last_file_error();
	return -1;
}
#endif

/**
 * Fill file information in the file_t structure.
 *
 * @param file the file information
 * @param use_lstat nonzero if lstat() shall be used
 * @return 0 on success, -1 on error
 */
int rsh_file_stat2(file_t* file, int use_lstat)
{
#ifdef _WIN32
	int i;
	(void)use_lstat; /* ignore on windows */

	file->mtime = 0;
	if(file->wpath) {
		free(file->wpath);
		file->wpath = NULL;
	}

	for(i = 0; i < 2; i++) {
		file->wpath = c2w(file->path, i);
		if(file->wpath == NULL) continue;

		/* return on success */
		if (rsh_file_statw(file) == 0) return 0;

		free(file->wpath);
		file->wpath = NULL;
	}
	/* NB: usually errno is set by the rsh_file_statw() call */
	if (!errno) errno = EINVAL;
	return -1;
#else
	struct rsh_stat_struct st;
	int res = -1;
	res = (use_lstat ? lstat(file->path, &st) : rsh_stat(file->path, &st));
	file->size  = st.st_size;
	file->mtime = st.st_mtime;

	file->mode  = 0;
	if(S_ISDIR(st.st_mode)) file->mode |= FILE_IFDIR;

	return res;
#endif /* _WIN32 */
}

/**
 * Fill file information in the file_t structure.
 *
 * @param file the file information
 * @return 0 on success, -1 on error
 */
int rsh_file_stat(file_t* file)
{
	return rsh_file_stat2(file, 0);
}

void rsh_file_cleanup(file_t* file)
{
	free(file->path);
	file->path = NULL;

#ifdef _WIN32
	free(file->wpath);
	file->wpath = NULL;
#endif /* _WIN32 */

	file->mtime = file->size = 0;
	file->mode = 0;
}

/* program exit and error reporting functions */

static void report_error_default(const char* srcfile, int srcline,
	const char* format, ...);

void (*rsh_exit)(int code) = exit;
void (*rsh_report_error)(const char* srcfile, int srcline,
	const char* format, ...) = report_error_default;

/**
 * Print given library failure to stderr.
 *
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @param format printf-formated error message
 */
static void report_error_default(const char* srcfile, int srcline, const char* format, ...)
{
	va_list ap;
	fprintf(stderr, "RHash: error at %s:%u: ", srcfile, srcline);
	va_start(ap, format);
	vfprintf(stderr, format, ap); /* report the error to stderr */
	va_end(ap);
}

/* MEMORY FUNCTIONS */

/**
 * Allocates a buffer via malloc with reporting memory error to stderr.
 *
 * @param size size of the block to allocate
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @return allocated block
 */
void* rhash_malloc(size_t size, const char* srcfile, int srcline)
{
	void* res = malloc(size);
	if(!res) {
		rsh_report_error(srcfile, srcline, "%s(%u) failed\n", "malloc", (unsigned)size);
		rsh_exit(2);
	}
	return res;
}

/**
 * Allocates a buffer via calloc with reporting memory error to stderr.
 *
 * @param num number of elements to be allocated
 * @param size size of elements
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @return allocated block
 */
void* rhash_calloc(size_t num, size_t size, const char* srcfile, int srcline)
{
	void* res = calloc(num, size);
	if(!res) {
		rsh_report_error(srcfile, srcline, "calloc(%u, %u) failed\n", (unsigned)num, (unsigned)size);
		rsh_exit(2);
	}
	return res;
}


/**
 * Duplicate c-string with reporting memory error to stderr.
 *
 * @param str the zero-terminated string to duplicate
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @return allocated memory buffer with copied string
 */
char* rhash_strdup(const char* str, const char* srcfile, int srcline)
{
#ifndef __STRICT_ANSI__
	char* res = strdup(str);
#else
	char* res = (char*)malloc(strlen(str)+1);
	if(res) strcpy(res, str);
#endif

	if(!res) {
		rsh_report_error(srcfile, srcline, "strdup(\"%s\") failed\n", str);
		rsh_exit(2);
	}
	return res;
}

#ifdef _WIN32
/**
 * Duplicate wide string with reporting memory error to stderr.
 *
 * @param str the zero-terminated string to duplicate
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @return allocated memory buffer with copied string
 */
wchar_t* rhash_wcsdup(wchar_t* str, const char* srcfile, int srcline)
{
	wchar_t* res = wcsdup(str);

	if(!res) {
		rsh_report_error(srcfile, srcline, "wcsdup() failed\n");
		rsh_exit(2);
	}
	return res;
}
#endif

/**
 * Reallocates a buffer via realloc with reporting memory error to stderr.
 *
 * @param mem a memory block to re-allocate
 * @param size the new size of the block
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @return re-allocated memory buffer
 */
void* rhash_realloc(void* mem, size_t size, const char* srcfile, int srcline)
{
	void* res = realloc(mem, size);
	if(!res) {
		rsh_report_error(srcfile, srcline, "realloc(%p, %u) failed\n", mem, (unsigned)size);
		rsh_exit(2);
	}
	return res;
}

/* vector functions */

/**
 * Allocate an empty vector.
 *
 * @param destructor pointer to the cleanup/deallocate function called
 *                   on each element when the vector is destructed,
 *                   NULL if items doesn't need to be freed
 * @return allocated vector
 */
vector_t* rsh_vector_new(void (*destructor)(void*))
{
	vector_t* ptr = (vector_t*)rsh_malloc(sizeof(vector_t));
	memset(ptr, 0, sizeof(vector_t));
	ptr->destructor = destructor;
	return ptr;
}

/**
 * Allocate an empty vector of pointers to memory blocks,
 * which will be deallocated at destruction time by calling free().
 *
 * @return allocated vector
 */
struct vector_t* rsh_vector_new_simple(void)
{
	return rsh_vector_new(free);
}

/**
 * Release memory allocated by vector, but the vector structure itself.
 *
 * @param vect the vector to free
 */
void rsh_vector_destroy(vector_t* vect)
{
	if(!vect) return;
	if(vect->destructor) {
		unsigned i;
		for(i=0; i<vect->size; i++) vect->destructor(vect->array[i]);
	}
	free(vect->array);
	vect->size = vect->allocated = 0;
	vect->array = 0;
}

/**
 * Release all memory allocated by vector.
 *
 * @param vect the vector to free
 */
void rsh_vector_free(vector_t* vect)
{
	rsh_vector_destroy(vect);
	free(vect);
}

/**
 * Add an item to vector.
 *
 * @param vect vector to add item to
 * @param item the item to add
 */
void rsh_vector_add_ptr(vector_t* vect, void* item)
{
	/* check if vect contains enough space for the next item */
	if(vect->size >= vect->allocated) {
		size_t size = (vect->allocated==0 ? 128 : vect->allocated * 2);
		vect->array = (void**)rsh_realloc(vect->array, size * sizeof(void*));
		vect->allocated = size;
	}
	/* add new item to the vector */
	vect->array[vect->size] = item;
	vect->size++;
}

/**
 * Add a sized item to vector.
 *
 * @param vect pointer to the vector to add item to
 * @param item_size the size of a vector item
 */
void rsh_vector_add_empty(struct vector_t* vect, size_t item_size)
{
	/* check if vect contains enough space for next item */
	if(vect->size >= vect->allocated) {
		size_t size = (vect->allocated==0 ? 128 : vect->allocated * 2);
		vect->array = (void**)rsh_realloc(vect->array, size * item_size);
		vect->allocated = size;
	}
	vect->size++;
}

/**
 * Initialize empty blocks vector.
 *
 * @param bvector pointer to the blocks vector
 */
void rsh_blocks_vector_init(blocks_vector_t* bvector)
{
	memset(bvector, 0, sizeof(*bvector));
	bvector->blocks.destructor = free;
}

/**
 * Free memory allocated by blocks vector, the function
 * doesn't deallocate memory additionally allocated for each element.
 *
 * @param bvector pointer to the blocks vector
 */
void rsh_blocks_vector_destroy(blocks_vector_t* bvector)
{
	rsh_vector_destroy(&bvector->blocks);
}

/* STRING BUFFER FUNCTIONS */

/**
 * Allocate an empty string buffer.
 *
 * @return allocated string buffer
 */
strbuf_t* rsh_str_new(void)
{
	strbuf_t *res = (strbuf_t*)malloc(sizeof(strbuf_t));
	memset(res, 0, sizeof(strbuf_t));
	return res;
}

/**
 * Free memory allocated by string buffer object
 *
 * @param ptr pointer to the string buffer to destroy
 */
void rsh_str_free(strbuf_t* ptr)
{
	if(ptr) {
		free(ptr->str);
		free(ptr);
	}
}

/**
 * Grow, if needed, internal buffer of the given string to ensure it contains
 * at least new_size number bytes.
 *
 * @param str pointer to the string-buffer object
 * @param new_size number of bytes buffer must contain
 */
void rsh_str_ensure_size(strbuf_t *str, size_t new_size)
{
	if(new_size >= (size_t)str->allocated) {
		if(new_size < 64) new_size = 64;
		str->str = (char*)rsh_realloc(str->str, new_size);
		str->allocated = new_size;
	}
}

/**
 * Append a sequence of single-byte characters of the specified length to
 * string buffer. The array is fully copied even if it contains the '\\0'
 * character. The function ensures the string buffer still contains
 * null-terminated string.
 *
 * @param str pointer to the string buffer
 * @param text the text to append
 * @param length number of character to append.
 */
void rsh_str_append_n(strbuf_t *str, const char* text, size_t length)
{
	rsh_str_ensure_length(str, str->len + length + 1);
	memcpy(str->str + str->len, text, length);
	str->len += length;
	str->str[str->len] = '\0';
}

/**
 * Append a null-terminated string to the string string buffer.
 *
 * @param str pointer to the string buffer
 * @param text the null-terminated string to append
 */
void rsh_str_append(strbuf_t *str, const char* text)
{
	rsh_str_append_n(str, text, strlen(text));
}
