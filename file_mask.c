/* file_mask.c - matching file against a list of file masks */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_mask.h"
#include "common_func.h"

/**
 * Convert the given string to lower-case then put it into
 * the specified array of 'file masks'.
 *
 * @param arr array of file masks
 * @param mask a string to add
 */
static void file_mask_add(file_mask_array* vect, const char* mask)
{
	rsh_vector_add_ptr(vect, str_tolower(mask));
}

/**
 * Construct array from a comma-separated list of strings.
 *
 * @param comma_separated_list the comma-separated list of strings
 * @return constructed array
 */
file_mask_array* file_mask_new_from_list(const char* comma_separated_list)
{
	file_mask_array* vect = file_mask_new();
	file_mask_add_list(vect, comma_separated_list);
	return vect;
}

/**
 * Split the given string by comma and put the parts into array.
 *
 * @param vect the array to put the parsed elements to
 * @param comma_separated_list the string to split
 */
void file_mask_add_list(file_mask_array* vect, const char* comma_separated_list)
{
	char *buf, *cur, *next;
	if (!comma_separated_list || !*comma_separated_list) {
		return;
	}
	buf = rsh_strdup(comma_separated_list);
	for (cur = buf; cur && *cur; cur = next) {
		next = strchr(cur, ',');
		if (next) *(next++) = '\0';
		if (*cur != '\0') file_mask_add(vect, cur);
	}
	free(buf);
}

/**
 * Match a given name against a list of string trailers.
 * Usually used to match a filename against list of file extensions.
 *
 * @param vect the array of string trailers
 * @param name the name to match
 * @return 1 if matched, 0 otherwise
 */
int file_mask_match(file_mask_array* vect, const char* name)
{
	unsigned i;
	int res = 0;
	size_t len, namelen;
	char* buf;

	/* all names should match against an empty array */
	if (!vect || !vect->size) return 1;

	/* get a lowercase name version to ignore case when matching */
	buf = str_tolower(name);
	namelen = strlen(buf);
	for (i = 0; i < vect->size; i++) {
		len = strlen((char*)vect->array[i]);
		if (namelen >= len && memcmp(buf + namelen - len, vect->array[i], len) == 0) {
			res = 1; /* matched */
			break;
		}
	}
	free(buf);
	return res;
}
