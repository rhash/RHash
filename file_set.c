/* file_set.c */
#include <stdlib.h> /* qsort */
#include <stdio.h>  /* fopen */
#include <stddef.h>  /* ptrdiff_t */
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
  item->search_filepath = (opt.flags&OPT_IGNORE_CASE ? str_tolower(item->filepath) : item->filepath);
  item->hash = rhash_get_crc32_str(0, item->search_filepath);
  return 1;
}

/**
 * Call-back function to compare two file items by search_filepath, using hashes
 *
 * @param pp_rec1 the first  item to compare
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

/* bit flags to denote type of hexadecimal/base32 hash string */
#define F_HEX 1
#define F_BASE32 2

/**
 * Test if a character is a hexadecimal/base32 digit.
 *
 * @param c the character to test
 * @return result of the test, a comination of flags F_HEX and F_BASE32
 */
static int test_hash_char(char c)
{
  return (IS_HEX(c) ? F_HEX : 0) | (IS_BASE32(c) ? F_BASE32 : 0);
}

/**
 * Detect if given string contains a hexadecimal or base32 hash.
 *
 * @param ptr the pointer to start scanning from
 * @param end pointer to scan to
 * @param p_len pointer to a number to store length of detected hash string
 * @return type of detected hash as combination of F_HEX and F_BASE32 flags
 */
static int get_hash_type(char **ptr, char *end, int *p_len)
{
  int len = 0;
  int char_type = 0, next_type = (F_HEX | F_BASE32);

  if(*ptr < end) {
    /* search forward (but no more then 129 symbols) */
    if((end - *ptr) >= 129) end = *ptr + 129;
    for(; (next_type &= test_hash_char(**ptr)) && *ptr <= end; len++, (*ptr)++) {
      char_type = next_type;
    }
  } else {
    /* search backward (but no more then 129 symbols) */
    if((*ptr-end) >= 129) end = *ptr - 129;
    for(; (next_type &= test_hash_char(**ptr)) && *ptr >= end; len++, (*ptr)--) {
      char_type = next_type;
    }
  }
  *p_len = len;
  return char_type;
}

/**
 * Test that the given string contain a hexadecimal or base32 hash string
 * of one of supported hash sums.
 *
 * @param ptr the pointer to start scanning from
 * @param end pointer to scan to
 * @param p_len pointer to a number to store length of detected hash string
 * @return possible type of detected hash as algorithm RHASH id
 */
static int test_hash_string(char **ptr, char *end, int *p_len)
{
  int len = 0;
  int hash_type = 0;
  int char_type = get_hash_type(ptr, end, &len);


  if(len == 32 && char_type) {
    hash_type = (char_type == F_BASE32 ? RHASH_AICH : 
        char_type == F_HEX ? RHASH_MD5_ED2K_MIXED_UP : RHASH_MD5_AICH_MIXED_UP | RHASH_MD5);
  } else if((char_type & F_BASE32) != 0 && len == 39) {
    hash_type = RHASH_TTH;
  } else if((char_type & F_HEX) != 0) {
    hash_type = (len == 8 ? RHASH_CRC32 : len == 32 ? RHASH_MD5_ED2K_MIXED_UP :
        len == 40 ? RHASH_SHA1 : len == 48 ? RHASH_TIGER : len == 56  ? RHASH_SHA224 :
        len == 64  ? RHASH_GOST : len == 96 ? RHASH_SHA384 : len == 128 ? RHASH_WHIRLPOOL : 0);
  }

  if(hash_type != 0) *p_len = len;
  return hash_type;
}

/** 
 * Store a sum into sums structure (its type is guessed by sum length).
 *
 * @param sums the structure to store hash sum to
 * @param str hexadecimal string representation of the sum
 * @param len length of the sum
 */
static void put_hash_sum(struct rhash_sums_t* sums, const char* str, unsigned hash_type, size_t length)
{
  unsigned char *psum = 0;
  unsigned sum_id = hash_type;

  /* if hash_type contains single bit */
  if(0 == (hash_type & (hash_type - 1)) && (hash_type & RHASH_ALL_HASHES)) {
    psum = rhash_get_digest_ptr(sums, hash_type);
  }
  if(!psum) {
    if(hash_type == RHASH_TTH || (hash_type & RHASH_AICH) != 0) {
      psum = (hash_type == RHASH_TTH ? sums->tth_digest : sums->aich_digest);
      if(hash_type != RHASH_TTH) sum_id = RHASH_AICH;
    }
    else if( hash_type & (RHASH_MD5 | RHASH_ED2K | RHASH_IS_MIXED) ) {
      if( (hash_type & RHASH_IS_MIXED) != 0 ) {
        hash_type |= ((sums->flags&RHASH_MD5) == 0 ? RHASH_MD5 : RHASH_ED2K);
      }
      sum_id = RHASH_MD5;
      psum = (hash_type&RHASH_MD5 ? sums->md5_digest : hash_type&RHASH_ED2K ? sums->ed2k_digest : 0);
    }
  }

  if(psum) {
    int len = rhash_get_digest_size(sum_id) * 2;
    assert(len >= 0);

    /* parse hexadecimal or base32 formated hash sum */
    if( (int)length == len ) {
      rhash_hex_to_byte(str, psum, (int)length);
    } else {
      rhash_base32_to_byte(str, psum, (int)length);
    }
  }

  /* note: no checking done for repeated sums (like <file> <crc32> <crc32>) */
  sums->flags |= hash_type;
}

#ifndef _WIN32
/** 
 * Convert a windows file path to a unix one, replacing backslashes 
 * by shlashes.
 *
 * @param path the path to convert
 * @return converted path
 */
static void process_backslashes(char* path)
{
  for(;*path;path++) {
    if(*path == '\\') *path = '/';
  }
}
#else /* _WIN32 */
#define process_backslashes(path)
#endif /* _WIN32 */

/**
 * Try to parse a bsd-formated line.
 * (md5|sha1|crc32|tiger|tth|whirlpool|ed2k|aich) \( <filename> \) = ... 
 *
 * @param line the line to parse
 * @param sum the rhash_sums_t structure to store parsed hash sums
 * @return 1 on success, 0 otherwise
 */
static int parse_bsd_format(char* line, const char** filename, struct rhash_sums_t* sums)
{
  /* NOTE: Starting and trailing spaces must be already removed from the line. */
  char* e;
  char* sum_ptr;
  const char* func_name;
  int sum_flag, parsed_length;
  size_t len;
  unsigned hash_type;

  sum_flag = 0;
  len = strlen(line);
  switch(tolower(*line)) {
    case 'a':
      sum_flag = RHASH_AICH;
      break;
    case 'b':
      sum_flag = RHASH_BTIH;
      break;
    case 'c':
      sum_flag = RHASH_CRC32;
      break;
    case 'm':
      if(len >=3 && line[2] == '5') {
        sum_flag = RHASH_MD5;
      } else {
        sum_flag = RHASH_MD4;
      }
      break;
    case 's':
      if(tolower(line[1]) == 'h') {
        if(len >= 4 && line[3] == '1') sum_flag = RHASH_SHA1;
        else if(len >= 7) {
          sum_flag = (line[5] == '2' ? RHASH_SHA224 : line[5] == '5' ? RHASH_SHA256 :
            line[5] == '8' ? RHASH_SHA384 : RHASH_SHA512);
        }
      } else if(len >= 10) {
        sum_flag = (line[7] == '1' ? RHASH_SNEFRU128 : RHASH_SNEFRU256);
      }
      break;
    case 't':
      if(tolower(line[1]) == 'i') {
        sum_flag = RHASH_TIGER;
      } else {
        sum_flag = RHASH_TTH;
      }
      break;
    case 'w':
      sum_flag = RHASH_WHIRLPOOL;
      break;
    case 'r':
      sum_flag = RHASH_RIPEMD160;
      break;
    case 'h':
      sum_flag = RHASH_HAS160;
      break;
    case 'g':
      if(len >= 14 && line[4] == '-') {
        sum_flag = RHASH_GOST_CRYPTOPRO;
      } else {
        sum_flag = RHASH_GOST;
      }
      break;
    case 'e':
      sum_flag = (len < 9 || line[2] == '2' ? RHASH_ED2K :
          line[6] == '2' ? RHASH_EDONR256 : RHASH_EDONR512);
      break;
  }
  if(sum_flag == 0) return 0;

  func_name = rhash_get_name(sum_flag);
  len  = rhash_get_hash_length(sum_flag);
  assert(func_name != 0 && len > 0);

  for(; *func_name; line++, func_name++)
    if(toupper(*line) != *func_name) return 0;

  /* skip whitespaces */
  while(isspace(*line)) line++;

  /* check for '(' */
  if(*(line++) != '(') return 0;

  /* skip whitespaces */
  while(isspace(*line)) line++;

  e = line + strlen(line)-1;
  sum_ptr = e-len+1;
  if(sum_ptr <= line) return 0;

  /* check for hash sum */
  hash_type = test_hash_string(&e, sum_ptr-1, &parsed_length);
  if(hash_type == 0 || (int)len != parsed_length) return -1;

  /* skip whitespaces */
  while(isspace(*e)) e--;

  /* check for '=' from the end */
  if(*(e--) != '=') return -1;

  /* skip whitespaces */
  while(isspace(*e)) e--;

  /* check for ')' from the end */
  if(*(e--) != ')') return -1;

  /* set the filename and terminate it with '\0' */
  *filename = line;
  e[1] = '\0';
  process_backslashes(line);

  /* store parsed hash sum */
  put_hash_sum(sums, sum_ptr, sum_flag, len);
  return 1;
}

/**
 * Try to parse given line as a magnet-link.
 *
 * @param line the magnet link to parse
 * @param sum the rhash_sums_t structure to store parsed hash sums
 * @return 1 on success, 0 otherwise
 */
static int parse_magnet_link(char* line, const char** filename, struct rhash_sums_t* sums)
{
  const char* prefix[]  = { "xl=", "dn=", "xt=urn:" };
  char* ptr = line;
  char* filename_end = 0;

  if(strncmp(ptr, "magnet:?", 8) != 0) {
    return 0;
  }
  ptr += 8;

  /* parse urn substrings */
  while(*ptr) {
    int index, hash_id;
    size_t length;
    char* param;

    for(index = 0; index < 3; index++) {
      length = strlen(prefix[index]);
      if(strncmp(ptr, prefix[index], length) == 0) {
        ptr += length;
        break;
      }
    }
    if(index >= 3) {
      continue; /* skip unknown type of parameter */
    }

    param = ptr;

    /* switch to the next parameter */
    for(; *ptr && *ptr != '&'; ptr++);
    length = (ptr - param);
    if(*ptr == '&') ptr++;

    if(index == 1) {
      *filename = param;
      filename_end = param + length; /* don't modify buffer until it is correctly parsed */
    }

    /* note: file size (xl=...) is not verified */
    if(index <= 1) continue;
    assert(index == 2);

    /* detect parameter sum */
    for(index = 0; index < RHASH_HASH_COUNT; index++) {
      const char* urn = hash_info_table[index].urn;
      size_t len = strlen(urn);
      if(strncmp(param, urn, len) == 0 && param[len] == ':') {
        param += len + 1;
        length -= len + 1;
        break;
      }
    }
    if(index >= RHASH_HASH_COUNT) {
      if(opt.flags & OPT_VERBOSE) {
        log_msg("warning: unknown hash in magnet link: %s\n", param);
      }
      continue;
    }
    hash_id = 1 << index;
    put_hash_sum(sums, param, hash_id, (unsigned)length);
  }
  if(filename_end) *filename_end = 0;
  return (filename_end ? 1 : 0);
}

/**
 * Try to given ed2k-link.
 *
 * @param line the ed2k link to parse
 * @param sum the rhash_sums_t structure to store parsed hash sums
 * @return 1 on success, 0 otherwise
 */
static int parse_ed2k_link(char* line, const char** filename, struct rhash_sums_t* sums)
{
  char* ptr = line;
  char* filename_end = 0;
  int index;

  if(strncmp(ptr, "ed2k://|file|", 13) != 0) {
    return 0;
  }
  ptr += 13;

  /* parse urn substrings */
  for(index = 0; *ptr && *ptr != '/' && index <= 3; index++) {
    int hash_id = 0, char_type;
    int hlen;
    ptrdiff_t length;
    char *param, *end;

    if(index == 3 && ptr[0] == 'h' && ptr[1] == '=') ptr += 2;
    end = param = ptr;

    /* switch to the next parameter */
    for(; *ptr && *ptr != '|'; ptr++);
    length = (ptr - param);
    if(*ptr != '|') break;
    ptr++;


    if(index == 0) {
      *filename = param;
      filename_end = param + length; /* don't modify buffer until it is correctly parsed */
    }

    /* note: file size is not parsed or verified */
    if(index <= 1) continue;

    char_type = (get_hash_type(&end, ptr, &hlen) & (index == 2 ? F_HEX : F_BASE32));
    if(length != 32 || !char_type) {
      if(opt.flags & OPT_VERBOSE) {
        log_msg("warning: can't parse hash in ed2k link: %s\n", param);
      }
      return 0;
    }

    hash_id = (index == 2 ? RHASH_ED2K : RHASH_AICH);
    put_hash_sum(sums, param, hash_id, (unsigned)length);
  }
  if((*ptr != '/' && *ptr != '\0') || !filename_end) return 0;
  *filename_end = 0;
  return 1;
}

/**
 * Parse a line of a generic crc file to extract crc sums and filename.
 *
 * @param line a modifiable buffer containing the line to parse
 * @param filename pointer to recive parsed filename
 * @param sums buffer to recieve parsed sums
 * @param check_eol true if check for trailing '\n' required
 * @return 0 on success, negative value on error
 */
int parse_crc_file_line(char* line, const char** filename, struct rhash_sums_t* sums, int check_eol)
{
  int count;
  int res;
  char* p = line;
  char* e = line + strlen(line) - 1;
  *filename = NULL;

  if(is_binary_string(line)) {
    return -2;
  }
  /* return if EOL not found at the end of the line */
  if( (*e != '\n' && check_eol) || e <= line) return -1;

  /* note: it's simpler then to use str_tim, cause 'e' is used below */
  while(isspace(*e) && e > line) *(e--) = 0; /* remove trailing white spaces */
  while(isspace(*p)) p++;  /* skip white spaces at the start of the line */

  /* try to parse BSD formated line or magnet/ed2k links */
  if( (res = parse_bsd_format(p, filename, sums)) ||
      (res = parse_magnet_link(p, filename, sums)) ||
      (res = parse_ed2k_link(p, filename, sums)) ) {
    return (res > 0 ? 0 : -1);
  }

  /* parse lines with filename preceding hash sums */
  for(count = 0; e > p;) {
    int len;
    
    /* search for hash sum from the end of the line */
    unsigned hash_type = test_hash_string(&e, p, &len);
    int stop = (!hash_type || e <= p || !isspace(*e));

    if(hash_type && (!stop || count == 0)) {
      put_hash_sum(sums, e + 1, hash_type, len);
      count++;
    }
    if(stop) break;

    /* skip hash sum and preceding white spaces */
    while(isspace(*e) && e > p) *(e--) = 0;
    
    *filename = line;
  }

  if(count > 0) {
    if(*filename) {
      process_backslashes((char*)*filename);
    }
    return 0;
  }

  /* parse lines with hash sums preceding a filename */
  e = p + strlen(p) - 1;
  for(count = 0; p < e; count++) {
    int len;
      
    /* search for hash sum */
    unsigned hash_type = test_hash_string(&p, e, &len);

    if(!hash_type || p >= e || !isspace(*p)) break;
    put_hash_sum(sums, p - len, hash_type, len);

    /* skip processed hash sum and following white spaces */
    while(isspace(*p) && p < e) p++;
    
    /* remove preceding star '*' from filename */
    if(p && *p == '*') p++;
    *filename = p;
  }
  if(*filename) {
    process_backslashes((char*)*filename);
  }
  return 0;
}
