/* file_set.c */
#include <stdlib.h> /* qsort */
#include <stdio.h>  /* fopen */
#include <stddef.h> /* ptrdiff_t */
#include <string.h>
#include <ctype.h>  /* isspace */
#include <assert.h>

#include "librhash/hex.h"
#include "librhash/crc32.h"
#include "common_func.h"
#include "crc_print.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "output.h"
#include "file_set.h"

/**
 * Allocate a file_item structure and initialize it with a filepath.
 *
 * @param filepath a filepath to initialize the file_item
 * @return allocated file_item structure
 */
file_item* file_item_new(const char* filepath)
{
	file_item *item = (file_item*)rsh_malloc(sizeof(file_item));
	memset(item, 0, sizeof(file_item));

	if(filepath) {
		if(!file_item_set_filepath(item, filepath)) {
			free(item);
			return NULL;
		}
	}
	return item;
}

/**
 * Free memory allocated by file_item.
 *
 * @param item the item to delete
 */
void file_item_free(file_item *item)
{
	if(item->search_filepath != item->filepath) {
		free(item->search_filepath);
	}
	free(item->filepath);
	free(item);
}

/**
 * Set file path of the given item.
 *
 * @param item pointer to the item to change
 * @param filepath the file path to set
 */
int file_item_set_filepath(file_item* item, const char* filepath)
{
	if(item->search_filepath != item->filepath)
		free(item->search_filepath);
	free(item->filepath);
	item->filepath = rsh_strdup(filepath);
	if(!item->filepath) return 0;

	/* apply str_tolower if CASE_INSENSITIVE */
	/* Note: strcasecmp() is not used instead of search_filepath due to portability issue */
	/* Note: item->search_filepath is always correctly freed by file_item_free() */
	item->search_filepath = (opt.flags & OPT_IGNORE_CASE ? str_tolower(item->filepath) : item->filepath);
	item->hash = rhash_get_crc32_str(0, item->search_filepath);
	return 1;
}

/**
 * Call-back function to compare two file items by search_filepath, using hashes
 *
 * @param pp_rec1 the first item to compare
 * @param pp_rec2 the second item to compare
 * @return 0 if items are equal, -1 if pp_rec1 < pp_rec2, 1 otherwise
 */
static int crc_pp_rec_compare(const void *pp_rec1, const void *pp_rec2)
{
	const file_item *rec1 = *(file_item *const *)pp_rec1, *rec2 = *(file_item *const *)pp_rec2;
	if(rec1->hash != rec2->hash) return (rec1->hash < rec2->hash ? -1 : 1);
	return strcmp(rec1->search_filepath, rec2->search_filepath);
}

/**
 * Sort given file_set using hashes of search_filepath for fast binary search.
 *
 * @param set the file_set to sort
 */
void file_set_sort(file_set *set)
{
	if(set->array) qsort(set->array, set->size, sizeof(file_item*), crc_pp_rec_compare);
}

/**
 * Create and add a file_item with given filepath to given file_set
 *
 * @param set the file_set to add the item to
 * @param filepath the item file path
 */
void file_set_add_name(file_set *set, const char* filepath)
{
	file_item* item = file_item_new(filepath);
	if(item) file_set_add(set, item);
}

/**
 * Find given file path in the file_set
 *
 * @param set the file_set to search
 * @param filepath the file path to search for
 * @return the found file_item or NULL if it was not found
 */
file_item* file_set_find(file_set *set, const char* filepath)
{
	int a, b, c;
	unsigned hash;
	char* search_filepath;

	if(!set->size) return NULL;
	/*assert(set->array);*/

	/* apply str_tolower if case shall be ignored */
	search_filepath =
		( opt.flags&OPT_IGNORE_CASE ? str_tolower(filepath) : (char*)filepath );

	/* generate hash to speedup the search */
	hash = rhash_get_crc32_str(0, search_filepath);

	/* fast binary search */
	for(a = -1, b = set->size; (a + 1) < b;) {
		file_item *item;
		int cmp;

		c = (a + b) / 2;
		/*assert(0 <= c && c < (int)set->size);*/

		item = (file_item*)set->array[c];
		if(hash != item->hash) {
			cmp = (hash < item->hash ? -1 : 1);
		} else {
			cmp = strcmp(search_filepath, item->search_filepath);
			if(cmp == 0) {
				if(search_filepath != filepath) free(search_filepath);
				return item; /* file was found */
			}
		}
		if(cmp < 0) b = c;
		else a = c;
	}
	if(search_filepath != filepath) free(search_filepath);
	return NULL;
}
