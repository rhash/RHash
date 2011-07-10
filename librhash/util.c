/* util.c - memory, vector and strings utility functions
 *
 * Copyright: 2010 Alexey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 */

#include <unistd.h>
#include <stdlib.h> /* size_t for vc6.0 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

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
	if(res) wcscpy(res, str);
#endif

	if(!res) {
		rsh_report_error(srcfile, srcline, "wcsdup(\"%u\") failed\n", (wcslen(str) + 1));
		rsh_exit(2);
	}
	return res;
}

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
 *                   on each element when the vector is destructed
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
 * @param destructor function to free vector items,
 *                   NULL if items doesn't need to be freed
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
 * @param destructor function to free vector items,
 *                   NULL if items doesn't need to be freed
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
	/* check if vect contains enough space for next item */
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
 * @param item the size of a vector item
 */
void rsh_vector_item_add_empty(struct vector_t* vect, size_t item_size)
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
 * @param pointer to the string buffer to destroy
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
 * string buffer. The array is fully copied even if it contains the '\0'
 * character. The function ensures the string buffer still contains
 * null-terminated string.
 *
 * @param str pointer to the string buffer
 * @param text the text to append
 * @param len number of character to append.
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
