/* common_func.c - functions used almost everywhere */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#if defined( _WIN32) || defined(__CYGWIN__)
# include <windows.h>
#endif

#include "common_func.h"
#include "parse_cmdline.h"
#include "version.h"
#include "win_utils.h"

/*=========================================================================
 * String functions
 *=========================================================================*/

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
	if (number == 0) {
		*(p--) = '0';
	} else {
		for (; p >= buf && number != 0; p--, number /= 10) {
			*p = '0' + (char)(number % 10);
		}
	}
	len = buf + 22 - p;
	if ((size_t)min_width > len) {
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
	for (len = 0; num; len++, num /= 10);
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
	if (!dst) {
		int len;
		for (len = 0; *name; name++) len += (IS_GOOD_URL_CHAR(*name) ? 1 : 3);
		return len;
	}
	/* encode URL as specified by RFC 1738 */
	for (start = dst; *name; name++) {
		if ( IS_GOOD_URL_CHAR(*name) ) {
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
	if (buf) {
		for (p = buf; *p; p++) *p = tolower(*p);
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
	while (isspace((unsigned char)*str)) str++;
	while (isspace((unsigned char)*last) && last > str) *(last--) = 0;
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
	for (; *str; str++) {
		if (((unsigned char)*str) < 32 && ((1 << (unsigned char)*str) & ~0x2600)) {
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
	for (; *str; str++) {
		if ((*str & 0xc0) != 0x80) length++;
	}
	return length;
}

/*=========================================================================
* Program version information
*=========================================================================*/

const char* get_version_string(void)
{
	static const char* version_string = VERSION;
	return version_string;
}

const char* get_bt_program_name(void)
{
	static const char* bt_program_name = PROGRAM_NAME "/" VERSION;
	return bt_program_name;
}

/*=========================================================================
 * Timer functions
 *=========================================================================*/

/**
 * Return real-value representing number of seconds
 * stored in the given timeval structure.
 * The function is used with timers, when printing time statistics.
 *
 * @param delta time delta to be converted
 * @return number of seconds
 */
static double rsh_fsec(timedelta_t* timer)
{
#if defined( _WIN32) || defined(__CYGWIN__)
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return (double)*timer / freq.QuadPart;
#else
	return ((double)timer->tv_usec / 1000000.0) + timer->tv_sec;
#endif
}

#if defined( _WIN32) || defined(__CYGWIN__)
#define get_timedelta(delta) QueryPerformanceCounter((LARGE_INTEGER*)delta)
#else
#define get_timedelta(delta) gettimeofday(delta, NULL)
#endif

void rsh_timer_start(timedelta_t* timer)
{
	get_timedelta(timer);
}

double rsh_timer_stop(timedelta_t* timer)
{
	timedelta_t end;
	get_timedelta(&end);
#if defined( _WIN32) || defined(__CYGWIN__)
	*timer = end - *timer;
#else
	timer->tv_sec  = end.tv_sec  - timer->tv_sec - (end.tv_usec >= timer->tv_usec ? 0 : 1);
	timer->tv_usec = end.tv_usec + (end.tv_usec >= timer->tv_usec ? 0 : 1000000 ) - timer->tv_usec;
#endif
	return rsh_fsec(timer);
}

unsigned rhash_get_ticks(void)
{
#if defined( _WIN32) || defined(__CYGWIN__)
	return GetTickCount();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/*=========================================================================
 * Custom program exit function
 *=========================================================================*/

struct rhash_exit_handlers_t
{
	unsigned handlers_count;
	exit_handler_t handlers[4];
} rhash_exit_handlers = { 0, { 0 } };

/**
* Install a handler to be called on program exit.
*
* @param handler the hadler to add
*/
void rsh_install_exit_handler(exit_handler_t handler)
{
	if (rhash_exit_handlers.handlers_count >= (sizeof(rhash_exit_handlers.handlers) / sizeof(rhash_exit_handlers.handlers[0])))
	{
		assert(!"to many handlers");
		rsh_exit(2);
	}
	rhash_exit_handlers.handlers[rhash_exit_handlers.handlers_count] = handler;
	rhash_exit_handlers.handlers_count++;
}

/**
* Remove the last installed exit handler.
*/
void rsh_remove_exit_handler(void)
{
	if (rhash_exit_handlers.handlers_count == 0)
	{
		assert(rhash_exit_handlers.handlers_count > 0 && "no handlers installed");
		rsh_exit(2);
	}
	rhash_exit_handlers.handlers_count--;
}

/**
* Call all installed exit handlers, starting from the latest one, and exit the program.
*
* @param code the program exit code
*/
void rsh_exit(int code)
{
	while (rhash_exit_handlers.handlers_count > 0)
		rhash_exit_handlers.handlers[--rhash_exit_handlers.handlers_count]();
	exit(code);
}

/*=========================================================================
 * Error reporting functions
 *=========================================================================*/

static void report_error_default(const char* srcfile, int srcline,
	const char* format, ...);

void (*rsh_report_error)(const char* srcfile, int srcline,
	const char* format, ...) = report_error_default;

/**
 * Print given library failure to stderr.
 *
 * @param srcfile source file to report error on fail
 * @param srcline source code line to be reported on fail
 * @param format printf-formatted error message
 */
static void report_error_default(const char* srcfile, int srcline, const char* format, ...)
{
	va_list ap;
	rsh_fprintf(stderr, "RHash: error at %s:%u: ", srcfile, srcline);
	va_start(ap, format);
	rsh_vfprintf(stderr, format, ap); /* report the error to stderr */
	va_end(ap);
}

/*=========================================================================
 * Memory functions
 *=========================================================================*/

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
	if (!res) {
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
	if (!res) {
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
	if (res) strcpy(res, str);
#endif

	if (!res) {
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
wchar_t* rhash_wcsdup(const wchar_t* str, const char* srcfile, int srcline)
{
#ifndef __STRICT_ANSI__
	wchar_t* res = wcsdup(str);
#else
	wchar_t* res = (wchar_t*)malloc((wcslen(str) + 1) * sizeof(wchar_t));
	if (res) wcscpy(res, str);
#endif

	if (!res) {
		rsh_report_error(srcfile, srcline, "wcsdup(\"%u\") failed\n", (wcslen(str) + 1));
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
	if (!res) {
		rsh_report_error(srcfile, srcline, "realloc(%p, %u) failed\n", mem, (unsigned)size);
		rsh_exit(2);
	}
	return res;
}

/*=========================================================================
 * Containers
 *=========================================================================*/

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
	if (!vect) return;
	if (vect->destructor) {
		unsigned i;
		for (i=0; i<vect->size; i++) vect->destructor(vect->array[i]);
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
	if (vect->size >= vect->allocated) {
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
	if (vect->size >= vect->allocated) {
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

/*=========================================================================
 * String buffer functions
 *=========================================================================*/

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
	if (ptr) {
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
	if (new_size >= (size_t)str->allocated) {
		if (new_size < 64) new_size = 64;
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
