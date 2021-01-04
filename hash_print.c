/* hash_print.c - output message digests using printf-like format */

#include "hash_print.h"
#include "calc_sums.h"
#include "output.h"
#include "parse_cmdline.h"
#include "win_utils.h"
#include "librhash/rhash.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <fcntl.h>
# include <io.h>
#endif /* _WIN32 */

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
	PRINT_FLAG_URLENCODE = 0x4000000,
	PRINT_FLAG_PAD_WITH_ZERO = 0x8000000,
	PRINT_FLAGS_ALL = PRINT_FLAG_UPPERCASE | PRINT_FLAG_PAD_WITH_ZERO | PRINT_FLAG_RAW
		| PRINT_FLAG_HEX | PRINT_FLAG_BASE32 | PRINT_FLAG_BASE64 | PRINT_FLAG_URLENCODE,
	PRINT_STR = 0x10000000,
	PRINT_ZERO,
	PRINT_NEWLINE,
	PRINT_FILEPATH,
	PRINT_BASENAME,
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
static print_item* new_print_item(unsigned flags, unsigned hash_id, const char* data)
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
static char parse_escaped_char(const char** pformat)
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
				(*pformat)++;
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
print_item* parse_print_string(const char* format, unsigned* sum_mask)
{
	char* buf;
	char* p;
	print_item* list = NULL;
	print_item* item = NULL;
	print_item** tail;

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
 * @param flags pointer to unsigned variable to receive print flags
 * @return directive id on success, 0 on fail
 */
static unsigned printf_name_to_id(const char* name, size_t length, unsigned* flags)
{
	char buf[20];
	size_t i;
	print_hash_info* info = hash_info_table;
	unsigned bit;

	if (length > (sizeof(buf) - 1)) return 0;
	for (i = 0; i < length; i++) buf[i] = tolower(name[i]);

	/* check for legacy '%{urlname}' directive for compatibility */
	if (length == 7 && memcmp(buf, "urlname", 7) == 0) {
		*flags = PRINT_BASENAME | PRINT_FLAG_URLENCODE;
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
 * Parse a token following a percent sign in a printf-like format string.
 *
 * @param str a pointer to the string, containing the token to parse
 * @return print_item with parsed information
 */
print_item* parse_percent_item(const char** str)
{
	const char* format = *str;
	const char* p = NULL;
	unsigned hash_id = 0;
	unsigned modifier_flags = 0;
	int id_found = 0;
	int width = 0;
	print_item* item = NULL;

	static const char* short_hash = "CMHTGWRAE";
	static const char* short_other = "Llpfs";
	static const unsigned hash_ids[] = {
		RHASH_CRC32, RHASH_MD5, RHASH_SHA1, RHASH_TTH, RHASH_GOST12_256,
		RHASH_WHIRLPOOL, RHASH_RIPEMD160, RHASH_AICH, RHASH_ED2K
	};
	static const unsigned other_flags[] = {
		(PRINT_ED2K_LINK | PRINT_FLAG_UPPERCASE), PRINT_ED2K_LINK,
		PRINT_FILEPATH, PRINT_BASENAME, PRINT_SIZE
	};
	/* detect the padding by zeros */
	if (*format == '0') {
		modifier_flags = PRINT_FLAG_PAD_WITH_ZERO;
		format++;
	} else if ((*format & ~0x20) == 'U') {
		modifier_flags = (*format & 0x20 ? PRINT_FLAG_URLENCODE : PRINT_FLAG_URLENCODE | PRINT_FLAG_UPPERCASE);
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

	/* if a complicated token encountered */
	if (*format == '{') {
		/* parse the token of the kind "%{some-token}" */
		const char* p = format + 1;
		for (; isalnum((unsigned char)*p) || (*p == '-'); p++);
		if (*p == '}') {
			unsigned flags = 0;
			hash_id = printf_name_to_id(format + 1, p - (format + 1), &flags);
			if (hash_id || (flags & PRINT_FLAG_URLENCODE) || flags == PRINT_MTIME) {
				/* set uppercase flag if the first letter of printf-entity is uppercase */
				modifier_flags |= flags | (format[1] & 0x20 ? 0 : PRINT_FLAG_UPPERCASE);
				format = p;
				id_found = 1;
			}
		}
	}

	/* if still not found a token denoting a hash function */
	if (!id_found) {
		const char upper = *format & ~0x20;
		/* if the string terminated just after the '%' character */
		if (*format == '\0')
			return NULL;
		/* look for a known token */
		if (upper && (p = strchr(short_hash, upper))) {
			assert( (p - short_hash) < (int)(sizeof(hash_ids) / sizeof(unsigned)) );
			hash_id = hash_ids[p - short_hash];
			modifier_flags |= (*format & 0x20 ? 0 : PRINT_FLAG_UPPERCASE);
		}
		else if ((p = strchr(short_other, *format))) {
			assert( (p - short_other) < (int)(sizeof(other_flags) / sizeof(unsigned)) );
			modifier_flags |= other_flags[p - short_other];
			if ((modifier_flags & ~PRINT_FLAGS_ALL) == PRINT_ED2K_LINK)
				hash_id = RHASH_ED2K | RHASH_AICH;
		} else if ((modifier_flags & PRINT_FLAG_URLENCODE) != 0) {
			/* handle legacy token: '%u' -> '%uf' */
			modifier_flags |= PRINT_BASENAME;
			format--;
		} else {
			return 0; /* no valid token found */
		}
	}
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
 * @param sums the file message digests
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int fprint_ed2k_url(FILE* out, struct file_info* info, int print_type)
{
	const char* filename = file_get_print_path(info->file, FPathUtf8 | FPathBaseName | FPathNotNull);
	int upper_case = (print_type & PRINT_FLAG_UPPERCASE ? RHPR_UPPERCASE : 0);
	char buf[104];
	char* dst = buf;
	assert(info->sums_flags & RHASH_ED2K);
	assert(info->rctx);
	if (rsh_fprintf(out, "ed2k://|file|") < 0 || fprint_urlencoded(out, filename, upper_case) < 0)
		return -1;
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
	return PRINTF_RES(rsh_fprintf(out, "%s", buf));
}

/**
 * Output aligned uint64_t number to specified output stream.
 *
 * @param out the stream to output to
 * @param filesize the 64-bit integer to output, usually a file size
 * @param width minimal width of integer to output
 * @param flag =1 if the integer shall be prepended by zeros
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int fprintI64(FILE* out, uint64_t u64, int width, int zero_pad)
{
	char* buf = (char*)rsh_malloc(width > 40 ? width + 1 : 41);
	int len = int_len(u64);
	int res;
	sprintI64(buf, u64, width);
	if (len < width && zero_pad) {
		memset(buf, '0', width - len);
	}
	res = PRINTF_RES(rsh_fprintf(out, "%s", buf));
	free(buf);
	return res;
}

/**
 * Print time formatted as 'YYYY-MM-DD hh:mm:ss' to a file stream.
 *
 * @param out the stream to print the time to
 * @param time the time to print
 * @param sfv_format if =1, then change time format to 'hh:mm.ss YYYY-MM-DD'
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int print_time64(FILE* out, uint64_t time64, int sfv_format)
{
	time_t time = (time_t)time64;
	struct tm* t = localtime(&time);
	char* format = (sfv_format ? "%02u:%02u.%02u %4u-%02u-%02u" :
		"%4u-%02u-%02u %02u:%02u:%02u");
	int date_index = (sfv_format ? 3 : 0);
	unsigned d[6];
	if (!!t) {
		d[date_index + 0] = t->tm_year + 1900;
		d[date_index + 1] = t->tm_mon + 1;
		d[date_index + 2] = t->tm_mday;
		d[3 - date_index] = t->tm_hour;
		d[4 - date_index] = t->tm_min;
		d[5 - date_index] = t->tm_sec;
	} else {
		/* if got a strange day, then print the date '1900-01-00 00:00:00' */
		memset(d, 0, sizeof(d));
		d[date_index + 0] = 1900;
		d[date_index + 1] = 1;
	}
	return PRINTF_RES(rsh_fprintf(out, format, d[0], d[1], d[2], d[3], d[4], d[5]));
}

/**
 * Print formatted file information to the given output stream.
 *
 * @param out the stream to print information to
 * @param out_mode output file mode
 * @param list the format according to which information shall be printed
 * @param info the file information
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_line(FILE* out, unsigned out_mode, print_item* list, struct file_info* info)
{
	char buffer[130];
	int res = 0;
	unsigned out_flags = (out_mode & FileContentIsUtf8 ? OutForceUtf8 : 0);
#ifdef _WIN32
	/* switch to binary mode to correctly output binary message digests */
	int out_fd = _fileno(out);
	int old_mode = (out_fd > 0 && !isatty(out_fd) ? _setmode(out_fd, _O_BINARY) : -1);
#endif

	for (; list && res == 0; list = list->next) {
		int print_type = list->flags & ~(PRINT_FLAGS_ALL);
		size_t len;

		/* output a hash function digest */
		if (!print_type) {
			unsigned hash_id = list->hash_id;
			int print_flags = (list->flags & PRINT_FLAG_UPPERCASE ? RHPR_UPPERCASE : 0)
				| (list->flags & PRINT_FLAG_RAW ? RHPR_RAW : 0)
				| (list->flags & PRINT_FLAG_BASE32 ? RHPR_BASE32 : 0)
				| (list->flags & PRINT_FLAG_BASE64 ? RHPR_BASE64 : 0)
				| (list->flags & PRINT_FLAG_HEX ? RHPR_HEX : 0)
				| (list->flags & PRINT_FLAG_URLENCODE ? RHPR_URLENCODE : 0);
			if ((hash_id == RHASH_GOST94 || hash_id == RHASH_GOST94_CRYPTOPRO) && (opt.flags & OPT_GOST_REVERSE))
				print_flags |= RHPR_REVERSE;
			assert(hash_id != 0);
			len = rhash_print(buffer, info->rctx, hash_id, print_flags);
			assert(len < sizeof(buffer));
			/* output the hash, continue on success */
			if (rsh_fwrite(buffer, 1, len, out) == len || errno == 0)
				continue;
			res = -1;
			break; /* exit on error */
		}

		/* output other special items: filepath, URL-encoded filename etc. */
		switch (print_type) {
			case PRINT_STR:
				res = PRINTF_RES(rsh_fprintf(out, "%s", list->data));
				break;
			case PRINT_ZERO: /* the '\0' character */
				res = PRINTF_RES(rsh_fprintf(out, "%c", 0));
				break;
#ifdef _WIN32
			case PRINT_NEWLINE:
				res = PRINTF_RES(rsh_fprintf(out, "%s", "\r\n"));
				break;
#endif
			case PRINT_FILEPATH:
			case PRINT_BASENAME: /* the filename without directory */
				if ((list->flags & PRINT_FLAG_URLENCODE) != 0) {
					unsigned fflags = FPathUtf8 | FPathNotNull | (print_type == PRINT_BASENAME ? FPathBaseName : 0);
					fprint_urlencoded(out, file_get_print_path(info->file, fflags),
						(list->flags & PRINT_FLAG_UPPERCASE));
				} else {
					unsigned pflags = (print_type == PRINT_BASENAME ? OutBaseName : 0);
					res = PRINTF_RES(fprintf_file_t(out, NULL, info->file, pflags | out_flags));
				}
				break;
			case PRINT_MTIME: /* the last-modified tine of the filename */
				res = print_time64(out, info->file->mtime, 0);
				break;
			case PRINT_SIZE: /* file size */
				res = fprintI64(out, info->size, list->width, (list->flags & PRINT_FLAG_PAD_WITH_ZERO));
				break;
			case PRINT_ED2K_LINK:
				res = fprint_ed2k_url(out, info, list->flags);
				break;
		}
	}
	if (res == 0 && fflush(out) < 0)
		res = -1;
#ifdef _WIN32
	if (old_mode >= 0)
		_setmode(out_fd, old_mode);
#endif
	return res;
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
 * Initialization of internal data
 *=========================================================================*/

#define VNUM(v, index) (((unsigned)v >> (24 - index * 8)) & 0xff)

/**
 * Get text representation of librhash version.
 */
static const char* get_librhash_version(void)
{
	static char buf[20];
	rhash_uptr_t v = rhash_get_version();
	if (v == RHASH_ERROR) {
		/* test for version-specific librhash features */
		int algorithm_count = rhash_count();
		if (rhash_transmit(14, NULL, 0, 0) != RHASH_ERROR)
			v = 0x01040000;
		else if (rhash_print(NULL, NULL, RHASH_CRC32, (RHPR_RAW | RHPR_URLENCODE)) != 4)
			v = 0x01030900;
		else if (algorithm_count == 29)
			v = 0x01030800;
		else if (algorithm_count == 27)
			v = 0x01030700;
		else if (rhash_get_openssl_supported_mask() != RHASH_ERROR)
			v = 0x01030600;
		else if (algorithm_count == 26)
			return "1.3.[0-5]";
		else
			return "1.2.*";
	}
	sprintf(buf, "%d.%d.%d", VNUM(v, 0), VNUM(v, 1), VNUM(v, 2));
	return buf;
}

/**
 * Initialize information about message digests, stored in the
 * hash_info_table global variable.
 */
void init_hash_info_table(void)
{
	unsigned bit;
	const unsigned fullmask = RHASH_ALL_HASHES | OPT_ED2K_LINK;
	const unsigned custom_bsd_name = RHASH_RIPEMD160 | RHASH_BLAKE2S | RHASH_BLAKE2B |
		RHASH_SHA224 | RHASH_SHA256 | RHASH_SHA384 | RHASH_SHA512;
	const unsigned short_opt_mask = RHASH_CRC32 | RHASH_MD5 | RHASH_SHA1 | RHASH_TTH | RHASH_ED2K |
		RHASH_AICH | RHASH_WHIRLPOOL | RHASH_RIPEMD160 | RHASH_GOST12_256 | OPT_ED2K_LINK;
	const char* short_opt = "cmhteawrgl";
	print_hash_info* info = hash_info_table;

	/* prevent crash on incompatible librhash */
	if (rhash_count() < RHASH_HASH_COUNT) {
		rsh_fprintf(stderr, "fatal error: incompatible librhash version is loaded: %s\n", get_librhash_version());
		rsh_exit(2);
	} else if (RHASH_HASH_COUNT != rhash_count())
		log_warning("inconsistent librhash version is loaded: %s\n", get_librhash_version());

	memset(hash_info_table, 0, sizeof(hash_info_table));
	for (bit = 1; bit && bit <= fullmask; bit = bit << 1) {
		const char* p;
		char* e;
		char* d;

		if (!(bit & fullmask))
			continue;

		info->short_char = ((bit & short_opt_mask) != 0 && *short_opt ?
			*(short_opt++) : 0);

		info->name = (bit & RHASH_ALL_HASHES ? rhash_get_name(bit) : "ED2K-LINK");
		assert(strlen(info->name) < 19);
		p = info->name;
		d = info->short_name;
		e = info->short_name + 19; /* buffer overflow protection */

		if (memcmp(info->name, "SHA", 3) == 0 || memcmp(info->name, "GOST", 4) == 0) {
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
		if ((bit & custom_bsd_name) != 0) {
			switch (bit) {
				case RHASH_RIPEMD160:
					info->bsd_name = "RMD160";
					break;
				case RHASH_SHA224:
					info->bsd_name = "SHA224";
					break;
				case RHASH_SHA256:
					info->bsd_name = "SHA256";
					break;
				case RHASH_SHA384:
					info->bsd_name = "SHA384";
					break;
				case RHASH_SHA512:
					info->bsd_name = "SHA512";
					break;
				case RHASH_BLAKE2S:
					info->bsd_name = "BLAKE2s";
					break;
				case RHASH_BLAKE2B:
					info->bsd_name = "BLAKE2b";
					break;
			}
		} else
			info->bsd_name = info->name;
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
	const char* fmt;
	const char* tail = 0;
	unsigned bit, index = 0;
	int uppercase;
	unsigned need_modifier = 0;
	char up_flag;
	char fmt_modifier = 'b';

	if (!opt.fmt) {
		/* print SFV header for CRC32 or if no hash functions options has been specified */
		opt.fmt = (opt.sum_flags == RHASH_CRC32 || !opt.sum_flags ? FMT_SFV : FMT_SIMPLE);
	}
	uppercase = ((opt.flags & OPT_UPPERCASE) ||
		(!(opt.flags & OPT_LOWERCASE) && (opt.fmt & FMT_SFV)));
	up_flag = (uppercase ? ~0x20 : 0xFF);

	rsh_str_ensure_size(out, 1024); /* allocate big enough buffer */

	if ((opt.sum_flags & OPT_ED2K_LINK) != 0) {
		rsh_str_append_n(out, "%l\\n", 4);
		out->str[1] &= up_flag;
		return;
	}
	if (opt.sum_flags == 0)
		return;

	if (opt.fmt == FMT_BSD) {
		fmt = "\003(%p) = \001\\n";
	} else if (opt.fmt == FMT_MAGNET) {
		rsh_str_append(out, "magnet:?xl=%s&dn=");
		rsh_str_append(out, (uppercase ? "%Uf" : "%uf"));
		fmt = "&xt=urn:\002:\001";
		need_modifier = RHASH_SHA1;
		tail = "\\n";
	} else if (opt.fmt == FMT_SIMPLE && 0 == (opt.sum_flags & (opt.sum_flags - 1))) {
		fmt = "\001  %p\\n";
	} else {
		rsh_str_append_n(out, "%p", 2);
		fmt = (opt.fmt == FMT_SFV ? " \001" : "  \001");
		tail = "\\n";
	}
	if ((opt.flags & OPT_FMT_MODIFIERS) != 0)
	{
		need_modifier = 0xffffffff;
		fmt_modifier = (opt.flags & OPT_HEX ? 'x' : opt.flags & OPT_BASE32 ? 'b' : 'B');
	}

	/* loop by message digests */
	for (bit = 1 << index; bit && bit <= opt.sum_flags; bit = bit << 1, index++) {
		const char* p;
		print_hash_info* info;

		if ((bit & opt.sum_flags) == 0)
			continue;
		p = fmt;
		info = &hash_info_table[index];

		/* ensure the output buffer have enough space */
		rsh_str_ensure_size(out, out->len + 256);

		for (;;) {
			int i;
			while (*p >= 0x20)
				out->str[out->len++] = *(p++);
			if (*p == 0)
				break;
			switch ((int)*(p++)) {
				case 1:
					out->str[out->len++] = '%';
					if ( (bit & need_modifier) != 0 )
						out->str[out->len++] = fmt_modifier;
					if (info->short_char)
						out->str[out->len++] = info->short_char & up_flag;
					else {
						char* letter;
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
					rsh_str_append(out, info->bsd_name);
					/* add some spaces after the hash BSD name */
					i = (int)strlen(info->bsd_name);
					for (i = (i < 5 ? 6 - i : 1); i > 0; i--)
						out->str[out->len++] = ' ';
					break;
			}
		}
	}
	if (tail)
		rsh_str_append(out, tail);
	out->str[out->len] = '\0';
}

/*=========================================================================
 * SFV format output functions
 *=========================================================================*/

/**
 * Format file information into SFV line and print it to the specified stream.
 *
 * @param out the stream to print the file information to
 * @param out_mode output file mode
 * @param file the file info to print
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_sfv_header_line(FILE* out, unsigned out_mode, file_t* file)
{
	char buf[24];
	unsigned out_flags = (out_mode & FileContentIsUtf8 ? OutForceUtf8 : 0);

	/* skip stdin stream and message-texts passed by command-line */
	if (FILE_ISSPECIAL(file))
		return 0;
	/* silently skip unreadable files, the access error will be reported later */
	if (!file_is_readable(file))
		return 0;
	sprintI64(buf, file->size, 12);
	if (rsh_fprintf(out, "; %s  ", buf) < 0)
		return -1;
	print_time64(out, file->mtime, 1);
	return PRINTF_RES(fprintf_file_t(out, " %s\n", file, out_flags));
}

/**
 * Print an SFV header banner. The banner consist of 3 comment lines,
 * with the program description and current time.
 *
 * @param out a stream to print to
 * @return 0 on success, -1 on fail with error code stored in errno
 */
int print_sfv_banner(FILE* out)
{
	time_t cur_time = time(NULL);
	struct tm* t = localtime(&cur_time);
	if (!t)
		return 0;
	if (rsh_fprintf(out,
			_("; Generated by %s v%s on %4u-%02u-%02u at %02u:%02u.%02u\n"),
			PROGRAM_NAME, get_version_string(),
			(1900 + t->tm_year), t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec) < 0)
		return -1;
	return PRINTF_RES(rsh_fprintf(out,
			_("; Written by Kravchenko Aleksey (Akademgorodok) - http://rhash.sf.net/\n;\n")));
}
