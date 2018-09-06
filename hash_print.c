/* hash_print.c - output hash sums using printf-like format */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#ifdef _WIN32
# include <fcntl.h>
# include <io.h>
#endif /* _WIN32 */

#include "hash_print.h"
#include "calc_sums.h"
#include "common_func.h"
#include "file.h"
#include "parse_cmdline.h"
#include "win_utils.h"
#include "librhash/rhash.h"

/*=========================================================================
* Formatted output functions and structures
*=========================================================================*/

/**
 * The table with information about hash functions.
 */
print_hash_info hash_info_table[32];

/**
 * Possible types of a print_item.
 */
enum {
	PRINT_ED2K_LINK = 0x100000,
	PRINT_FLAG_UPPERCASE = 0x200000,
	PRINT_FLAG_RAW    = 0x0400000,
	PRINT_FLAG_HEX    = 0x0800000,
	PRINT_FLAG_BASE32 = 0x1000000,
	PRINT_FLAG_BASE64 = 0x2000000,
	PRINT_FLAG_PAD_WITH_ZERO = 0x4000000,
	PRINT_FLAGS_ALL = PRINT_FLAG_UPPERCASE | PRINT_FLAG_PAD_WITH_ZERO | PRINT_FLAG_RAW
		| PRINT_FLAG_HEX | PRINT_FLAG_BASE32 | PRINT_FLAG_BASE64,
	PRINT_STR = 0x10000000,
	PRINT_ZERO,
	PRINT_NEWLINE,
	PRINT_FILEPATH,
	PRINT_BASENAME,
	PRINT_URLNAME,
	PRINT_SIZE,
	PRINT_MTIME /*PRINT_ATIME, PRINT_CTIME*/
};

/* parse a token following a percent sign '%' */
static print_item* parse_percent_item(const char** str);

/**
 * Allocate new print_item.
 *
 * @param flags the print_item flags
 * @param hash_id optional hash_id
 * @param data optional string to store
 * @return allocated print_item
 */
static print_item* new_print_item(unsigned flags, unsigned hash_id, const char *data)
{
	print_item* item = (print_item*)rsh_malloc(sizeof(print_item));
	item->flags = flags;
	item->hash_id = hash_id;
	item->width = 0;
	item->data = (data ? rsh_strdup(data) : NULL);
	item->next = NULL;
	return item;
}

/**
 * Parse an escaped sequence in a printf-like format string.
 *
 * @param pformat pointer to the sequence, the pointer
 *   is changed to point to the next symbol after parsed sequence
 * @return result character
 */
static char parse_escaped_char(const char **pformat)
{
	const char* start = *pformat;
	switch ( *((*pformat)++) ) {
		case '0': return '\0';
		case 't': return '\t';
		case 'r': return '\r';
		case 'n': return '\n';
		case '\\': return '\\';
		case 'x':
			/* \xNN byte with hexadecimal value NN (1 to 2 digits) */
			if ( IS_HEX(**pformat) ) {
				int ch;
				ch = (**pformat <= '9' ? **pformat & 15 : (**pformat + 9) & 15);
				(*pformat) ++;
				if (IS_HEX(**pformat)) {
					/* read the second digit */
					ch = 16 * ch + (**pformat <= '9' ? **pformat & 15 : (**pformat + 9) & 15);
					(*pformat)++;
				}
				return ch;
			}
			break;
		default:
			(*pformat)--;
			/* \NNN - character with octal value NNN (1 to 3 digits) */
			if ('0' < **pformat && **pformat <= '7') {
				int ch = *((*pformat)++) - '0';
				if ('0' <= **pformat && **pformat <= '7') {
					ch = ch * 8 + *((*pformat)++) - '0';
					if ('0' <= **pformat && **pformat <= '7')
						ch = ch * 8 + *((*pformat)++) - '0';
				}
				return (char)ch;
			}
	}
	*pformat = start;
	return '\\';
}

/**
 * Parse format string.
 *
 * @return a print_item list with parsed information
 */
print_item* parse_print_string(const char* format, unsigned *sum_mask)
{
	char *buf, *p;
	print_item *list = NULL, **tail, *item = NULL;

	buf = p = (char*)rsh_malloc( strlen(format) + 1 );
	tail = &list;
	*sum_mask = 0;

	for (;;) {
		while (*format && *format != '%' && *format != '\\')
			*(p++) = *(format++);

		if (*format == '\\') {
			format++;
			*p = parse_escaped_char(&format);
			if (*p == '\0') {
				item = new_print_item(PRINT_ZERO, 0, NULL);
#ifdef _WIN32
			} else if (*p == '\n') {
				item = new_print_item(PRINT_NEWLINE, 0, NULL);
#endif
			} else {
				p++;
				continue;
			}
		} else if (*format == '%') {
			if ( *(++format) == '%' ) {
				*(p++) = *format++;
				continue;
			} else {
				item = parse_percent_item(&format);
				if (!item) {
					*(p++) = '%';
					continue;
				}
				if (item->hash_id)
					*sum_mask |= item->hash_id;
			}
		}
		if (p > buf || (!*format && list == NULL && item == NULL)) {
			*p = '\0';
			*tail = new_print_item(PRINT_STR, 0, buf);
			tail = &(*tail)->next;
			p = buf;
		}
		if (item) {
			*tail = item;
			tail = &item->next;
			item = NULL;
		}
		if (!*format)
			break;
	};
	free(buf);
	return list;
}

/**
 * Convert given case-insensitive name to a printf directive id
 *
 * @param name printf directive name (not a 0-terminated)
 * @param length name length
 * @return directive id on success, 0 on fail
 */
static unsigned printf_name_to_id(const char* name, size_t length, unsigned *flags)
{
	char buf[20];
	size_t i;
	print_hash_info *info = hash_info_table;
	unsigned bit;

	if (length > (sizeof(buf)-1)) return 0;
	for (i = 0; i < length; i++) buf[i] = tolower(name[i]);

	/* check for old '%{urlname}' directive for compatibility */
	if (length == 7 && memcmp(buf, "urlname", 7) == 0) {
		*flags = PRINT_URLNAME;
		return 0;
	} else if (length == 5 && memcmp(buf, "mtime", 5) == 0) {
		*flags = PRINT_MTIME;
		return 0;
	}

	for (bit = 1; bit <= RHASH_ALL_HASHES; bit = bit << 1, info++) {
		if (memcmp(buf, info->short_name, length) == 0 &&
			info->short_name[length] == 0) return bit;
	}
	return 0;
}

/**
 * Parse a token followed by a percent sign in a printf-like format string.
 *
 * @return a print_item with parsed information
 */
print_item* parse_percent_item(const char** str)
{
	const char* format = *str;
	const char* p = NULL;
	unsigned hash_id = 0;
	unsigned modifier_flags = 0;
	int id_found = 0;
	int width = 0;
	int pad_with_zero_bit = 0;
	print_item* item = NULL;

	static const char *short_hash = "CMHTGWRAE";
	static const char *short_other = "Llpfus";
	static const unsigned hash_ids[] = {
		RHASH_CRC32, RHASH_MD5, RHASH_SHA1, RHASH_TTH, RHASH_GOST,
		RHASH_WHIRLPOOL, RHASH_RIPEMD160, RHASH_AICH, RHASH_ED2K
	};
	static const unsigned other_flags[] = {
		PRINT_ED2K_LINK, PRINT_ED2K_LINK, PRINT_FILEPATH, PRINT_BASENAME,
		PRINT_URLNAME, PRINT_SIZE
	};
	/* detect the padding by zeros */
	if (*format == '0') {
		pad_with_zero_bit = PRINT_FLAG_PAD_WITH_ZERO;
		format++;
	}

	/* parse the 'b','B','x' and '@' flags */
	if (*format == 'x') {
		modifier_flags |= PRINT_FLAG_HEX;
		format++;
	} else if (*format == 'b') {
		modifier_flags |= PRINT_FLAG_BASE32;
		format++;
	} else if (*format == 'B') {
		modifier_flags |= PRINT_FLAG_BASE64;
		format++;
	} else if (*format == '@') {
		modifier_flags |= PRINT_FLAG_RAW;
		format++;
	}
	for (; isdigit((unsigned char)*format); format++) width = 10 * width + (*format - '0');

	/* if a complicated token enconuntered */
	if (*format == '{') {
		/* parse the token of the kind "%{some-token}" */
		const char* p = ++format;
		for (; isalnum((unsigned char)*p) || (*p == '-'); p++);
		if (*p == '}') {
			hash_id = printf_name_to_id(format, (int)(p - (format)), &modifier_flags);
			format--;
			if (hash_id || modifier_flags == PRINT_URLNAME || modifier_flags == PRINT_MTIME) {
				/* set uppercase flag if the first letter of printf-entity is uppercase */
				modifier_flags |= (format[1] & 0x20 ? 0 : PRINT_FLAG_UPPERCASE);
				format = p;
				id_found = 1;
			}
		} else {
			format--;
		}
	}

	/* if still not found a token denoting a hash function */
	if (!id_found) {
		const char upper = *format & ~0x20;
		/* if the string terminated just after the '%' character */
		if ( *format == '\0' ) return NULL;
		/* look for a known token */
		if ( (p = strchr(short_hash, upper)) ) {
			assert( (p - short_hash) < (int)(sizeof(hash_ids) / sizeof(unsigned)) );
			hash_id = hash_ids[p - short_hash];
			modifier_flags |= (*format & 0x20 ? 0 : PRINT_FLAG_UPPERCASE);
		}
		else if ( (p = strchr(short_other, *format)) ) {
			assert( (p - short_other) < (int)(sizeof(other_flags) / sizeof(unsigned)) );
			modifier_flags = other_flags[p - short_other];

			if (modifier_flags == PRINT_ED2K_LINK) {
				modifier_flags |= (*p & 0x20 ? 0 : PRINT_FLAG_UPPERCASE);
				hash_id = RHASH_ED2K | RHASH_AICH;
			}
		} else {
			return 0; /* no valid token found */
		}
	}
	modifier_flags |= pad_with_zero_bit;

	item = new_print_item(modifier_flags, hash_id, NULL);
	item->width = width;
	*str = ++format;
	return item;
}

/**
 * Print EDonkey 2000 url for given file to a stream.
 *
 * @param out the stream where to print url to
 * @param filename the file name
 * @param filesize the file size
 * @param sums the file hash sums
 */
static void fprint_ed2k_url(FILE* out, struct file_info *info, int print_type)
{
	const char *filename = get_basename(file_info_get_utf8_print_path(info));
	int upper_case = (print_type & PRINT_FLAG_UPPERCASE ? RHPR_UPPERCASE : 0);
	int len = urlencode(NULL, filename) + int_len(info->size) + (info->sums_flags & RHASH_AICH ? 84 : 49);
	char* buf = (char*)rsh_malloc( len + 1 );
	char* dst = buf;

	assert(info->sums_flags & (RHASH_ED2K|RHASH_AICH));
	assert(info->rctx);

	strcpy(dst, "ed2k://|file|");
	dst += 13;
	dst += urlencode(dst, filename);
	*dst++ = '|';
	sprintI64(dst, info->size, 0);
	dst += strlen(dst);
	*dst++ = '|';
	rhash_print(dst, info->rctx, RHASH_ED2K, upper_case);
	dst += 32;
	if ((info->sums_flags & RHASH_AICH) != 0) {
		strcpy(dst, "|h=");
		rhash_print(dst += 3, info->rctx, RHASH_AICH, RHPR_BASE32 | upper_case);
		dst += 32;
	}
	strcpy(dst, "|/");
	rsh_fprintf(out, "%s", buf);
	free(buf);
}

/**
 * Output aligned uint64_t number to specified output stream.
 *
 * @param out the stream to output to
 * @param filesize the 64-bit integer to output, usually a file size
 * @param width minimal width of integer to output
 * @param flag =1 if the integer shall be prepended by zeros
 */
static void fprintI64(FILE* out, uint64_t filesize, int width, int zero_pad)
{
	char *buf = (char*)rsh_malloc(width > 40 ? width + 1 : 41);
	int len = int_len(filesize);
	sprintI64(buf, filesize, width);
	if (len < width && zero_pad) {
		memset(buf, '0', width-len);
	}
	rsh_fprintf(out, "%s", buf);
	free(buf);
}

/**
* Print time formatted as hh:mm.ss YYYY-MM-DD to a file stream.
*
* @param out the stream to print the time to
* @param time the time to print
*/
static void print_time(FILE *out, time_t time)
{
	struct tm *t = localtime(&time);
	static struct tm zero_tm;
	if (t == NULL) {
		/* if a strange day, then print `00:00.00 1900-01-00' */
		t = &zero_tm;
		t->tm_hour = t->tm_min = t->tm_sec =
			t->tm_year = t->tm_mon = t->tm_mday = 0;
	}
	rsh_fprintf(out, "%02u:%02u.%02u %4u-%02u-%02u", t->tm_hour, t->tm_min,
		t->tm_sec, (1900 + t->tm_year), t->tm_mon + 1, t->tm_mday);
}

/**
* Print time formatted as hh:mm.ss YYYY-MM-DD to a file stream.
*
* @param out the stream to print the time to
* @param time the time to print
*/
static void print_time64(FILE *out, uint64_t time)
{
	print_time(out, (time_t)time);
}

/**
 * Print formatted file information to given output stream.
 *
 * @param out the stream to print information to
 * @param list the format according to which information shall be printed
 * @param info the file information
 */
void print_line(FILE* out, print_item* list, struct file_info *info)
{
	const char* basename = get_basename(info->print_path), *tmp;
	char *url = NULL, *ed2k_url = NULL;
	char buffer[130];
#ifdef _WIN32
	/* switch to binary mode to correctly output binary hashes */
	int out_fd = _fileno(out);
	int old_mode = (out_fd > 0 && !isatty(out_fd) ? _setmode(out_fd, _O_BINARY) : -1);
#endif

	for (; list; list = list->next) {
		int print_type = list->flags & ~(PRINT_FLAGS_ALL);
		size_t len;

		/* output a hash function digest */
		if (list->hash_id && print_type != PRINT_ED2K_LINK) {
			unsigned hash_id = list->hash_id;
			int print_flags = (list->flags & PRINT_FLAG_UPPERCASE ? RHPR_UPPERCASE : 0)
				| (list->flags & PRINT_FLAG_RAW ? RHPR_RAW : 0)
				| (list->flags & PRINT_FLAG_BASE32 ? RHPR_BASE32 : 0)
				| (list->flags & PRINT_FLAG_BASE64 ? RHPR_BASE64 : 0)
				| (list->flags & PRINT_FLAG_HEX ? RHPR_HEX : 0);
			if ((hash_id == RHASH_GOST || hash_id == RHASH_GOST_CRYPTOPRO) && (opt.flags & OPT_GOST_REVERSE))
				print_flags |= RHPR_REVERSE;

			len = rhash_print(buffer, info->rctx, hash_id, print_flags);
			assert(len < sizeof(buffer));

			/* output the hash, exit on fail */
			rsh_fwrite(buffer, 1, len, out);
			continue;
		}

		/* output other special items: filepath, URL-encoded filename etc. */
		switch (print_type) {
			case PRINT_STR:
				rsh_fprintf(out, "%s", list->data);
				break;
			case PRINT_ZERO: /* the '\0' character */
				rsh_fprintf(out, "%c", 0);
				break;
#ifdef _WIN32
			case PRINT_NEWLINE:
				rsh_fprintf(out, "%s", "\r\n");
				break;
#endif
			case PRINT_FILEPATH:
				rsh_fprintf(out, "%s", info->print_path);
				break;
			case PRINT_BASENAME: /* the filename without directory */
				rsh_fprintf(out, "%s", basename);
				break;
			case PRINT_URLNAME: /* URL-encoded filename */
				if (!url) {
					tmp = get_basename(file_info_get_utf8_print_path(info));
					url = (char*)rsh_malloc(urlencode(NULL, tmp) + 1);
					urlencode(url, tmp);
				}
				rsh_fprintf(out, "%s", url);
				break;
			case PRINT_MTIME: /* the last-modified tine of the filename */
				print_time64(out, info->file->mtime);
				break;
			case PRINT_SIZE: /* file size */
				fprintI64(out, info->size, list->width, (list->flags & PRINT_FLAG_PAD_WITH_ZERO));
				break;
			case PRINT_ED2K_LINK:
				fprint_ed2k_url(out, info, list->flags);
				break;
		}
	}
	free(url);
	free(ed2k_url);
	fflush(out);
#ifdef _WIN32
	if (old_mode >= 0)
		_setmode(out_fd, old_mode);
#endif
}

/**
 * Release memory allocated by given print_item list.
 *
 * @param list the list to free
 */
void free_print_list(print_item* list)
{
	while (list) {
		print_item* next = list->next;
		free((char*)list->data);
		free(list);
		list = next;
	}
}

/*=========================================================================
* initialization of internal data
*=========================================================================*/

/**
 * Initialize information about hashes, stored in the
 * hash_info_table global variable.
 */
void init_hash_info_table(void)
{
	unsigned bit;
	unsigned short_opt_mask = RHASH_CRC32 | RHASH_MD5 | RHASH_SHA1 | RHASH_TTH | RHASH_ED2K |
		RHASH_AICH | RHASH_WHIRLPOOL | RHASH_RIPEMD160 | RHASH_GOST | OPT_ED2K_LINK;
	char* short_opt = "cmhteawrgl";
	print_hash_info *info = hash_info_table;
	unsigned fullmask = RHASH_ALL_HASHES | OPT_ED2K_LINK;

	memset(hash_info_table, 0, sizeof(hash_info_table));

	for (bit = 1; bit <= fullmask; bit = bit << 1) {
		const char *p;
		char *e, *d;

		if (!(bit & fullmask))
			continue;

		info->short_char = ((bit & short_opt_mask) != 0 && *short_opt ?
			*(short_opt++) : 0);

		info->name = (bit & RHASH_ALL_HASHES ? rhash_get_name(bit) : "ED2K-LINK");
		assert(strlen(info->name) < 15);
		p = info->name;
		d = info->short_name;
		e = info->short_name + 15; /* buffer overflow protection */

		if (memcmp(info->name, "SHA", 3) == 0) {
			strcpy(d, p);
			for (; *d && d < e; d++) {
				if ('A' <= *d && *d <= 'Z') {
					*d |= 0x20;
				}
			}
		} else {
			for (; *p && d < e; p++) {
				if (*p != '-' || p[1] >= '9') {
					*(d++) = (*p | 0x20);
				}
			}
		}
		*d = 0;
		++info;
	}
}

/**
 * Initialize printf string according to program options.
 * The function is called only when a printf format string is not specified
 * from command line, so it should be constructed from other options.
 *
 * @param out a string buffer to place the resulting format string into.
 */
void init_printf_format(strbuf_t* out)
{
	const char* fmt, *tail = 0;
	unsigned bit, index = 0;
	int uppercase;
	char up_flag;
	unsigned force_base32_mask = 0;

	if (!opt.fmt) {
		/* print SFV header for CRC32 or if no hash sums options specified */
		opt.fmt = (opt.sum_flags == RHASH_CRC32 || !opt.sum_flags ? FMT_SFV : FMT_SIMPLE);
	}
	uppercase = ((opt.flags & OPT_UPPERCASE) ||
		(!(opt.flags & OPT_LOWERCASE) && (opt.fmt & FMT_SFV)));
	up_flag = (uppercase ? ~0x20 : 0xFF);

	rsh_str_ensure_size(out, 1024); /* allocate big enough buffer */

	if (opt.sum_flags & OPT_ED2K_LINK) {
		rsh_str_append_n(out, "%l", 2);
		out->str[1] &= up_flag;
		return;
	}

	if (opt.sum_flags == 0) return;

	if (opt.fmt == FMT_BSD) {
		fmt = "\003(%p) = \001\\n";
	} else if (opt.fmt == FMT_MAGNET) {
		rsh_str_append(out, "magnet:?xl=%s&dn=%{urlname}");
		fmt = "&xt=urn:\002:\001";
		force_base32_mask = (RHASH_SHA1 | RHASH_BTIH);
		tail = "\\n";
	} else if (opt.fmt == FMT_SIMPLE && 0 == (opt.sum_flags & (opt.sum_flags - 1))) {
		fmt = "\001  %p\\n";
	} else {
		rsh_str_append_n(out, "%p", 2);
		fmt = (opt.fmt == FMT_SFV ? " \001" : "  \001");
		tail = "\\n";
	}

	/* loop by hashes */
	for (bit = 1 << index; bit <= opt.sum_flags; bit = bit << 1, index++) {
		const char *p;
		print_hash_info *info;

		if ((bit & opt.sum_flags) == 0) continue;
		p = fmt;
		info = &hash_info_table[index];

		/* ensure the output buffer have enough space */
		rsh_str_ensure_size(out, out->len + 256);

		for (;;) {
			int i;
			while (*p >= 0x20) out->str[out->len++] = *(p++);
			if (*p == 0) break;
			switch ((int)*(p++)) {
				case 1:
					out->str[out->len++] = '%';
					if ( (bit & force_base32_mask) != 0 ) {
						out->str[out->len++] = 'b';
					}
					if (info->short_char) out->str[out->len++] = info->short_char & up_flag;
					else {
						char *letter;
						out->str[out->len++] = '{';
						letter = out->str + out->len;
						rsh_str_append(out, info->short_name);
						*letter &= up_flag;
						out->str[out->len++] = '}';
					}
					break;
				case 2:
					rsh_str_append(out, rhash_get_magnet_name(bit));
					break;
				case 3:
					rsh_str_append(out, info->name);
					i = (int)strlen(info->name);
					for (i = (i < 5 ? 6 - i : 1); i > 0; i--) out->str[out->len++] = ' ';
					break;
			}
		}
	}
	if (tail) {
		rsh_str_append(out, tail);
	}
	out->str[out->len] = '\0';
}

/*=========================================================================
* SFV format output functions
*=========================================================================*/

/**
* Format file information into SFV line and print it to the specified stream.
*
* @param out the stream to print the file information to
* @param file the file info to print
* @return 0 on success, -1 on fail with error code stored in errno
*/
int print_sfv_header_line(FILE* out, file_t* file, const char* printpath)
{
	char buf[24];

	/* skip stdin stream */
	if ((file->mode & FILE_IFSTDIN) != 0) return 0;

	/* skip file if it can't be opened with exclusive sharing rights */
	if (file_is_write_locked(file)) return 0;

	if (!printpath) printpath = file->path;
	if (printpath[0] == '.' && IS_PATH_SEPARATOR(printpath[1])) printpath += 2;

	sprintI64(buf, file->size, 12);
	rsh_fprintf(out, "; %s  ", buf);
	print_time64(out, file->mtime);
	rsh_fprintf(out, " %s\n", printpath);
	return 0;
}

/**
* Print an SFV header banner. The banner consist of 3 comment lines,
* with the program description and current time.
*
* @param out a stream to print to
*/
void print_sfv_banner(FILE* out)
{
	time_t cur_time = time(NULL);
	struct tm *t = localtime(&cur_time);
	if (t) {
		rsh_fprintf(out, _("; Generated by %s v%s on %4u-%02u-%02u at %02u:%02u.%02u\n"),
			PROGRAM_NAME, get_version_string(),
			(1900 + t->tm_year), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		rsh_fprintf(out, _("; Written by Kravchenko Aleksey (Akademgorodok) - http://rhash.sf.net/\n;\n"));
	}
}
