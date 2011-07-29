/* file_mask.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common_func.h"
#include "file_mask.h"

/**
 * Convert a string to a lower-case and put it into array of file-masks.
 *
 * @param arr array of file masks
 * @param mask a string to add
 */
static void file_mask_add(file_mask_array* arr, const char* mask)
{
	rsh_vector_add_ptr(arr, str_tolower(mask));
}

/**
 * Construct array from a comma-separated list of strings.
 *
 * @param comma_separated_list the comma-separated list of strings
 * @return constructed array
 */
file_mask_array* file_mask_new_from_list(const char* comma_separated_list)
{
	file_mask_array* array = file_mask_new();
	file_mask_add_list(array, comma_separated_list);
	return array;
}

/**
 * Parse string consisting of comma-delimited list of elements an
 * add them to array.
 *
 * @param array the array to put parsed elements to
 * @param comma_separated_list tre string to parse
 */
void file_mask_add_list(file_mask_array* array, const char* comma_separated_list)
{
	char *buf, *cur, *next;
	if(!comma_separated_list || !*comma_separated_list) {
		return;
	}
	buf = rsh_strdup(comma_separated_list);
	for(cur = buf; cur && *cur; cur = next) {
		next = strchr(cur, ',');
		if(next) *(next++) = '\0';
		if(*cur != '\0') file_mask_add(array, cur);
	}
	free(buf);
}

/**
 * Match a given name against a list of string trailers.
 * Usually used to match a filename against list of file extensions.
 *
 * @param arr  the array of string trailers
 * @param name the name to match
 */
int file_mask_match(file_mask_array* arr, const char* name)
{
	unsigned i;
	int res = 0;
	size_t len, namelen;
	char* buf;
	/* all names should match against an empty array */
	if(!arr || !arr->size) return 1;

	/* get a lowercase name version to ignore case when matching */
	buf = str_tolower(name);
	namelen = strlen(buf);
	for(i = 0; i<arr->size; i++) {
		len = strlen((char*)arr->array[i]);
		if(namelen >= len && memcmp(buf + namelen - len, arr->array[i], len) == 0) {
			res = 1; /* matched */
			break;
		}
	}
	free(buf);
	return res;
}
