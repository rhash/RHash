/* file_set.c - functions to manipulate a set of files */
#include <assert.h>
#include <ctype.h>  /* isspace */
#include <stddef.h> /* ptrdiff_t */
#include <stdlib.h> /* qsort */
#include <string.h>

#include "file_set.h"
#include "common_func.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "librhash/rhash.h"

/**
 * Generate a hash for a string.
 *
 * @param string the string to hash
 * @return a string hash
 */
static unsigned file_set_make_hash(const char* string)
{
	unsigned hash;
	if (rhash_msg(RHASH_CRC32, string, strlen(string), (unsigned char*)&hash) < 0)
		return 0;
	return hash;
}

/**
 * Set file path of the given item.
 *
 * @param item pointer to the item to change
 * @param filepath the file path to set
 */
static int file_set_item_set_filepath(file_set_item* item, const char* filepath)
{
	if (item->search_filepath != item->filepath)
		free(item->search_filepath);
	free(item->filepath);
	item->filepath = rsh_strdup(filepath);
	if (!item->filepath) return 0;

	/* apply str_tolower if CASE_INSENSITIVE */
	/* Note: strcasecmp() is not used instead of search_filepath due to portability issue */
	/* Note: item->search_filepath is always correctly freed by file_set_item_free() */
	item->search_filepath = (opt.flags & OPT_IGNORE_CASE ? str_tolower(item->filepath) : item->filepath);
	item->hash = file_set_make_hash(item->search_filepath);
	return 1;
}

/**
 * Allocate a file_set_item structure and initialize it with a filepath.
 *
 * @param filepath a filepath to initialize the file_set_item
 * @return allocated file_set_item structure
 */
static file_set_item* file_set_item_new(const char* filepath)
{
	file_set_item *item = (file_set_item*)rsh_malloc(sizeof(file_set_item));
	memset(item, 0, sizeof(file_set_item));

	if (filepath) {
		if (!file_set_item_set_filepath(item, filepath)) {
			free(item);
			return NULL;
		}
	}
	return item;
}

/**
 * Free memory allocated by file_set_item.
 *
 * @param item the item to delete
 */
void file_set_item_free(file_set_item *item)
{
	if (item->search_filepath != item->filepath) {
		free(item->search_filepath);
	}
	free(item->filepath);
	free(item);
}

/**
 * Call-back function to compare two file items by search_filepath, using hashes
 *
 * @param pp_rec1 the first item to compare
 * @param pp_rec2 the second item to compare
 * @return 0 if items are equal, -1 if pp_rec1 &lt; pp_rec2, 1 otherwise
 */
static int crc_pp_rec_compare(const void *pp_rec1, const void *pp_rec2)
{
	const file_set_item *rec1 = *(file_set_item *const *)pp_rec1;
	const file_set_item *rec2 = *(file_set_item *const *)pp_rec2;
	if (rec1->hash != rec2->hash) return (rec1->hash < rec2->hash ? -1 : 1);
	return strcmp(rec1->search_filepath, rec2->search_filepath);
}

/**
 * Compare two file items by filepath.
 *
 * @param rec1 pointer to the first file_set_item structure
 * @param rec2 pointer to the second file_set_item structure
 * @return 0 if files have the same filepath, and -1 or 1 (strcmp result) if not
 */
static int path_compare(const void *rec1, const void *rec2)
{
	return strcmp((*(file_set_item *const *)rec1)->filepath,
		(*(file_set_item *const *)rec2)->filepath);
}

/**
 * Sort given file_set using hashes of search_filepath for fast binary search.
 *
 * @param set the file_set to sort
 */
void file_set_sort(file_set *set)
{
	if (set->array) qsort(set->array, set->size, sizeof(file_set_item*), crc_pp_rec_compare);
}

/**
 * Sort files in the specified file_set by file path.
 *
 * @param set the file-set to sort
 */
void file_set_sort_by_path(file_set *set)
{
	qsort(set->array, set->size, sizeof(file_set_item*), path_compare);
}

/**
 * Create and add a file_set_item with given filepath to given file_set
 *
 * @param set the file_set to add the item to
 * @param filepath the item file path
 */
void file_set_add_name(file_set *set, const char* filepath)
{
	file_set_item* item = file_set_item_new(filepath);
	if (item) file_set_add(set, item);
}

/**
 * Find a file path in the file_set.
 *
 * @param set the file_set to search
 * @param filepath the file path to search for
 * @return 1 if filepath is found, 0 otherwise
 */
int file_set_exist(file_set *set, const char* filepath)
{
	int a, b, c;
	int cmp, res = 0;
	unsigned hash;
	char* search_filepath;

	if (!set->size) return 0; /* not found */
	assert(set->array != NULL);

	/* apply str_tolower if case shall be ignored */
	search_filepath = (opt.flags & OPT_IGNORE_CASE ?
		str_tolower(filepath) : (char*)filepath);

	/* generate hash to speedup the search */
	hash = file_set_make_hash(search_filepath);

	/* fast binary search */
	for (a = -1, b = (int)set->size; (a + 1) < b;) {
		file_set_item *item;

		c = (a + b) / 2;
		assert(0 <= c && c < (int)set->size);

		item = (file_set_item*)set->array[c];
		if (hash != item->hash) {
			cmp = (hash < item->hash ? -1 : 1);
		} else {
			cmp = strcmp(search_filepath, item->search_filepath);
			if (cmp == 0) {
				res = 1; /* file path has been found */
				break;
			}
		}
		if (cmp < 0) b = c;
		else a = c;
	}
	if (search_filepath != filepath) free(search_filepath);
	return res;
}
