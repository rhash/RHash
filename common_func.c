/* common_func.c */

#include "common_func.h" /* should be included before the C library files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "librhash/hex.h"
#include "win_utils.h"
#include "parse_cmdline.h"

/**
 * Print a 0-terminated string representation of a 64-bit number to char buffer.
 */
void sprintI64(char *dst, uint64_t number, int max_width)
{
  char buf[24];
  size_t len;
  char *p = buf + 23;
  *(p--) = 0; /* last symbol should be '\0' */
  if(number == 0) {
    *(p--) = '0';
  } else {
    for(; p >= buf && number != 0; p--, number /= 10) {
      *p = '0' + (char)(number % 10);
    }
  }
  len = buf + 22 - p;
  if((size_t)max_width > len) {
    memset(dst, 0x20, max_width - len);
    dst += max_width - len;
  }
  memcpy(dst, p+1, len+1);
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

/* unsafe characters are "<>{}[]%#/|\^~`@:;?=&+ */
#define IS_GOOD_URL_CHAR(c) (((unsigned char)(c) < 128 && isalnum(c)) || strchr("$-_.!*'(),", c))

/**
 * URL-encode given string.
 * 
 * @param dst buffer to recieve result or NULL to calculate encoded string size
 * @param filename the file name
 * @return the length of the result string
 */
int urlencode(char *dst, const char *name)
{
  const char *start;
  if(!dst) {
    int len;
    for(len = 0; *name; name++) len += (IS_GOOD_URL_CHAR(*name) ? 1 : 3);
    /* ed2k://|file|<fname>|<fsize>|2E398E5533AE4A83475B1AF001C6CEE6|h=RKLBEXT4O2H4RZER676WAVWGACIHQ56Z|/ */
    return len;
  }
  /* encode URL as specified by RFC 1738 */
  for(start = dst; *name; name++) {
    if( IS_GOOD_URL_CHAR(*name) ) {
      *dst++ = *name;
    } else {
      *dst++ = '%';
      dst = rhash_print_hex_byte(dst, *name, 'A');
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
  while(isspace(*str)) str++;
  while(isspace(*last) && last > str) *(last--) = 0;
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
  if(len > 0 && !IS_PATH_SEPARATOR(buf[len-1])) 
    buf[len++] = SYS_PATH_SEPARATOR;

  /* append filename */
  strcpy(buf+len, filename);
  return buf;
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

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Return ticks in milliseconds for time intervals measurement.
 * This function should be not precise but the fastest one 
 * to retrive internal clock value.
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
