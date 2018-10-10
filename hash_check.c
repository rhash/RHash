/* hash_check.c - verification of hashes of files */

#include <assert.h>
#include <ctype.h>  /* isspace */
#include <string.h>

#include "hash_check.h"
#include "hash_print.h"
#include "common_func.h"
#include "output.h"
#include "parse_cmdline.h"
#include "librhash/rhash.h"

/* hash conversion macros and functions */
#define HEX2DIGIT(c) ((c) <= '9' ? (c) & 0xF : ((c) - 'a' + 10) & 0xF)
#define BASE32_TO_DIGIT(c) ((c) < 'A' ? (c) - '2' + 26 : ((c) & ~0x20) - 'A')
#define BASE32_LENGTH(bytes) (((bytes) * 8 + 4) / 5)

/**
 * Convert a hexadecimal string to a string of bytes.
 *
 * @param str string to parse
 * @param bin result
 * @param len string length
 */
void rhash_hex_to_byte(const char* str, unsigned char* bin, int len)
{
	/* parse the highest hexadecimal digit */
	if ((len & 1) != 0) {
		*(bin++) = HEX2DIGIT(*(str++));
		len--;
	}

	/* parse the rest - an even-sized hexadecimal string */
	for (; len >= 2; len -= 2, str += 2) {
		*(bin++) = (HEX2DIGIT(str[0]) << 4) | HEX2DIGIT(str[1]);
	}
}

/**
 * Decode an URL-encoded string in the specified buffer.
 *
 * @param buffer the 0-terminated URL-encoded string
 */
static void urldecode(char *buffer)
{
	char *wpos = buffer; /* set writing position */
	for (; *buffer; wpos++) {
		*wpos = *(buffer++); /* copy non-escaped characters */
		if (*wpos == '%') {
			if (*buffer == '%') {
				buffer++; /* interpret '%%' as single '%' */
			} else if (IS_HEX(*buffer)) {
				/* decode character from the %<hex-digit><hex-digit> form */
				int ch = HEX2DIGIT(*buffer);
				buffer++;
				if (IS_HEX(*buffer)) {
					ch = (ch << 4) | HEX2DIGIT(*buffer);
					buffer++;
				}
				*wpos = (char)ch;
			}
		}
	}
	*wpos = '\0'; /* terminate decoded string */
}

#ifndef _WIN32
/**
 * Convert a windows file path to a UNIX one, replacing '\\' by '/'.
 *
 * @param path the path to convert
 * @return converted path
 */
static void process_backslashes(char* path)
{
  for (;*path;path++) {
    if (*path == '\\') *path = '/';
  }
}
#else /* _WIN32 */
#define process_backslashes(path)
#endif /* _WIN32 */


/* convert a hash flag to index */
#if __GNUC__ >= 4 || (__GNUC__ ==3 && __GNUC_MINOR__ >= 4) /* GCC < 3.4 */
# define get_ctz(x) __builtin_ctz(x)
#else

/**
 * Returns index of the trailing bit of a 32-bit number.
 * This is a plain C equivalent for GCC __builtin_ctz() bit scan.
 *
 * @param x the number to process
 * @return zero-based index of the trailing bit
 */
static unsigned get_ctz(unsigned x)
{
	/* see http://graphics.stanford.edu/~seander/bithacks.html */
	static unsigned char bit_pos[32] =  {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	return bit_pos[((uint32_t)((x & -(int)x) * 0x077CB531U)) >> 27];
}
#endif /* (GCC >= 4.3) */

/**
 * Encode a hash function digest size into a small number in [0,...,7].
 * The digest size must be in the set { 4, 16, 20, 24, 28, 32, 48, 64 }.
 *
 * @param digest_size digest size (aka hash length) in bytes
 * @return code for digest size on success, 32 on error
 */
static int code_digest_size(int digest_size)
{
	static int size_codes[17] = {
		-1, 0,-1, -1, 1, 2, 3, 4, 5, -1,
		-1, -1, 6, -1, -1, -1, 7
	};
	return (digest_size <= 64 ? size_codes[digest_size / 4] : -1);
}

/**
 * Calculate an OR-ed mask of hash-ids by a length of hash in bytes.
 *
 * @param digest_size length of a hash in bytes.
 * @return mask of hash-ids with given hash length, 0 on fail.
 */
static unsigned hash_check_mask_by_digest_size(int digest_size)
{
	static unsigned mask[10] = { 0,0,0,0,0,0,0,0,0,0 };
	int code;
	if (mask[9] == 0) {
		unsigned hid;
		for (hid = 1; hid <= RHASH_ALL_HASHES; hid <<= 1) {
			code = code_digest_size(rhash_get_digest_size(hid));
			assert(0 <= code && code <= 7);
			if (code >= 0) mask[code] |= hid;
		}
		mask[9] = 1;
	}
	code = code_digest_size(digest_size);
	return (code >= 0 ? mask[code] : 0);
}

#define HV_BIN 0
#define HV_HEX 1
#define HV_B32 2

/**
 * Test if a character is a hexadecimal/base32 digit.
 *
 * @param c the character to test
 * @return result of the test, a combination of flags HV_HEX and HV_B32
 */
static int test_hash_char(char c)
{
	return (IS_HEX(c) ? HV_HEX : 0) | (IS_BASE32(c) ? HV_B32 : 0);
}

/**
 * Detect if given string contains a hexadecimal or base32 hash.
 *
 * @param ptr the pointer to start scanning from
 * @param end pointer to scan to
 * @param p_len pointer to a number to store length of detected hash string
 * @return type of detected hash as combination of HV_HEX and HV_B32 flags
 */
static int detect_hash_type(char **ptr, char *end, int *p_len)
{
	int len = 0;
	int char_type = 0, next_type = (HV_HEX | HV_B32);

	if (*ptr < end) {
		/* search forward (but no more then 129 symbols) */
		if ((end - *ptr) >= 129) end = *ptr + 129;
		for (; (next_type &= test_hash_char(**ptr)) && *ptr <= end; len++, (*ptr)++) {
			char_type = next_type;
		}
	} else {
		/* search backward (but no more then 129 symbols) */
		if ((*ptr-end) >= 129) end = *ptr - 129;
		for (; (next_type &= test_hash_char((*ptr)[-1])) && *ptr >= end; len++, (*ptr)--) {
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
static unsigned char test_hash_string(char **ptr, char *end, int *p_len)
{
	int len = 0;
	int char_type = detect_hash_type(ptr, end, &len);
	unsigned char hash_type = 0;

	if ((char_type & HV_HEX) && (len & 7) == 0 && len <= 256) {
		int pow = get_ctz(len >> 3);
		int code = ((len >> (pow + 4)) << 3) | pow;
		if (code < 32 && ((1 << code) & 0x101061d)) hash_type |= HV_HEX;
	}
	if ((char_type & HV_B32) && (len == 32 || len == 39)) {
		hash_type |= HV_B32;
	}
	if (hash_type != 0) {
		*p_len = len;
	}
	return hash_type;
}

/**
 * Detect ASCII-7 white spaces (not including Unicode whitespaces).
 * Note that isspace() is locale specific and detect Unicode spaces,
 * like U+00A0.
 */
static int rhash_isspace(char ch)
{
	return (((unsigned char)ch) <= 0x7F && isspace((unsigned char)ch));
}

/**
 * Information about found token
 */
typedef struct hc_search
{
	char* begin; /* start of the buffer to search */
	char* end;   /* end of the buffer to search */
	hash_check* hc;
	unsigned expected_hash_id;
	int hash_type;
	char* url_name;
	size_t url_length;
} hc_search;

/**
 * Parse the buffer pointed by search->begin, into tokens specified by format
 * string. The format string can contain the following special characters:
 * '\1' - hash function name, '\2' - any hash, '\3' - specified hash,
 * '\4' - an URL-encoded file name, '\5' - a file size,
 * '\6' - a required-space, '\7' - a space or string end.
 * A space ' ' means 0 or more space characters.
 * '$' - parse the rest of the buffer and the format string backward.
 * Other (non-special) symbols mean themselves.
 * The function updates search->begin and search->end pointers on success.
 *
 * @param search the structure to store parsed tokens info
 * @param format the format string
 * @return 1 on success, 0 if couldn't find specified token(s)
 */
static int hash_check_find_str(hc_search *search, const char* format)
{
	int backward = 0;
	char buf[20];
	const char *fend = strchr(format, '\0');
	char* begin = search->begin;
	char* end = search->end;
	hash_check* hc = search->hc;
	hash_value hv;
	int unsaved_hashval = 0;
	memset(&hv, 0, sizeof(hash_value));

	while (format < fend) {
		const char *search_str;
		int i, len = 0;
		uint64_t file_size;

		if (backward) {
			for (; fend[-1] >= '-' && format < fend; fend--, len++);
			if (len == 0) --fend;
			search_str = fend;
		} else {
			search_str = format;
			for (; *format >= '-' && format < fend; format++, len++);
			if (len == 0) format++;
		}
		if (len > 0) {
			if ((end - begin) < len) return 0;
			if (0 != memcmp(search_str, (backward ? end - len : begin), len)) return 0;
			if (backward) end -= len;
			else begin += len;
			continue;
		}
		assert(len == 0);

		/* find substring specified by single character */
		switch (*search_str) {
		case '\1': /* parse hash function name */
			/* the name should contain alphanumeric or '-' symbols, but */
			/* actually the loop shall stop at characters [:& \(\t] */
			for (; (begin[len] <= '9' ? begin[len] >= '0' || begin[len]=='-' : begin[len] >= 'A'); len++) {
				if (len >= 20) return 0; /* limit name length */
				buf[len] = toupper(begin[len]);
			}
			begin += len;
			if (len == 0) return 0; /* not alpha-numeric sequence */
			buf[len] = '\0';
			search->expected_hash_id = 0;

			 /* find hash_id by a hash function name */
			for (i = 0; i < RHASH_HASH_COUNT; i++) {
				if (strcmp(buf, hash_info_table[i].name) == 0) {
					search->expected_hash_id = 1 << i;
					search->hash_type = (HV_HEX | HV_B32);
					break;
				}
			}
			break;
		case '\2':
		case '\3':
			if (backward) {
				hv.format = test_hash_string(&end, begin, &len);
				hv.offset = (unsigned short)(end - hc->data);
			} else {
				hv.offset = (unsigned short)(begin - hc->data);
				hv.format = test_hash_string(&begin, end, &len);
			}
			if (!hv.format) return 0;
			if (*search_str == '\3') {
				/* verify hash type */
				int dlen = rhash_get_digest_size(search->expected_hash_id);
				hv.format &= search->hash_type;
				if ((!(hv.format & HV_HEX) || len != (dlen * 2)) &&
					(!(hv.format & HV_B32) || len != BASE32_LENGTH(dlen)))
					return 0;
				hv.hash_id = search->expected_hash_id;
			} else hv.hash_id = 0;
			hv.length = (unsigned char)len;
			unsaved_hashval = 1;
			break;
		case '\4': /* get URL-encoded name */
			search->url_name = begin;
			search->url_length = strcspn(begin, "?&|");
			if (search->url_length == 0) return 0; /* empty name */
			begin += search->url_length;
			break;
		case '\5': /* retrieve file size */
			assert(!backward);
			file_size = 0L;
			for (; '0' <= *begin && *begin <= '9'; begin++, len++) {
				file_size = file_size * 10 + (*begin - '0');
			}
			if (len == 0) return 0;
			else {
				hc->file_size = file_size;
				hc->flags |= HC_HAS_FILESIZE;
			}
			break;
		case '\6':
		case '\7':
		case ' ':
			if (backward) for (; begin < end && rhash_isspace(end[-1]); end--, len++);
			else for (; rhash_isspace(*begin) && begin < end; begin++, len++);
			/* check if space is mandatory */
			if (*search_str != ' ' && len == 0) {
				/* for '\6' check (len > 0) */
				/* for '\7' check (len > 0 || begin == end) */
				if (*search_str == '\6' || begin < end) return 0;
			}
			break;
		case '$':
			backward = 1; /* switch to parsing string backward */
			break;
		default:
			if ((backward ? *(--end) : *(begin++)) != *search_str) return 0;
		}
	}

	if (unsaved_hashval && hc->hashes_num < HC_MAX_HASHES) {
		hc->hashes[hc->hashes_num++] = hv;
	}
	search->begin = begin;
	search->end = end;

	return 1;
}

/* macros used by hash_check_parse_line() */
#define THREEC2U(c1, c2, c3) (((unsigned)(c1) << 16) | \
	((unsigned)(c2) << 8) | (unsigned)(c3))
#define FOURC2U(c1, c2, c3, c4) (((unsigned)(c1) << 24) | \
	((unsigned)(c2) << 16) | ((unsigned)(c3) << 8) | (unsigned)(c4))

/**
 * Parse a line of a hash-file. This function accepts five formats.
 * <ul>
 * <li/> magnet links
 * <li/> EDonkey/EMule ed2k links
 * <li/> BSD format: HASH_FUNCTION ( filepath ) = FILE_HASH
 * <li/> filepath FILE_HASH1 FILE_HASH2...
 * <li/> FILE_HASH1 FILE_HASH2... filepath
 * </ul>
 * For a magnet/ed2k links file size is also parsed.
 *
 * @param line the line to parse
 * @param hashes structure to store parsed hashes, file path and file size
 * @return 1 on success, 0 if couldn't parse the line
 */
int hash_check_parse_line(char* line, hash_check* hashes, int check_eol)
{
	hc_search hs;
	char* le = strchr(line, '\0'); /* set pointer to the end of line */
	char* url_name = NULL;
	size_t url_length = 0;
	int single_hash = 0;
	int reversed = 0;
	int bad = 0;
	int i, j;

	/* return if EOL not found at the end of the line */
	if ( line[0]=='\0' || (le[-1] != '\n' && check_eol) ) return 0;

	/* note: not using str_tim because 'le' is re-used below */

	/* remove trailing white spaces */
	while (rhash_isspace(le[-1]) && le > line) *(--le) = 0;
	/* skip white spaces at the start of the line */
	while (rhash_isspace(*line)) line++;

	memset(&hs, 0, sizeof(hs));
	hs.begin = line;
	hs.end = le;
	hs.hc = hashes;

	memset(hashes, 0, sizeof(hash_check));
	hashes->data = line;
	hashes->file_size = (uint64_t)-1;

	if (strncmp(line, "magnet:?", 8) == 0) {
		hs.begin += 8;

		/* loop by magnet link parameters */
		while (1) {
			char* next = strchr(hs.begin, '&');
			char* param_end = (next ? next++ : hs.end);
			char* hf_end;

			if ((hs.begin += 3) < param_end) {
				switch (THREEC2U(hs.begin[-3], hs.begin[-2], hs.begin[-1])) {
				case THREEC2U('d', 'n', '='): /* URL-encoded file path */
					url_name = hs.begin;
					url_length = param_end - hs.begin;
					break;
				case THREEC2U('x', 'l', '='): /* file size */
					if (!hash_check_find_str(&hs, "\5")) bad = 1;
					if (hs.begin != param_end) bad = 1;
					break;
				case THREEC2U('x', 't', '='): /* a file hash */
					/* find last ':' character (hash name can be complex like tree:tiger) */
					for (hf_end = param_end - 1; *hf_end != ':' && hf_end > hs.begin; hf_end--);

					/* test for the "urn:" string */
					if ((hs.begin += 4) >= hf_end) return 0;
					if (FOURC2U('u', 'r', 'n', ':') != FOURC2U(hs.begin[-4],
						hs.begin[-3], hs.begin[-2], hs.begin[-1])) return 0;

					/* find hash by its magnet link specific URN name  */
					for (i = 0; i < RHASH_HASH_COUNT; i++) {
						const char* urn = rhash_get_magnet_name(1 << i);
						size_t len = hf_end - hs.begin;
						if (strncmp(hs.begin, urn, len) == 0 &&
							urn[len] == '\0') break;
					}
					if (i >= RHASH_HASH_COUNT) {
						if (opt.flags & OPT_VERBOSE) {
							*hf_end = '\0';
							log_warning(_("unknown hash in magnet link: %s\n"), hs.begin);
						}
						return 0;
					}

					hs.begin = hf_end + 1;
					hs.expected_hash_id = 1 << i;
					hs.hash_type = (HV_HEX | HV_B32);
					if (!hash_check_find_str(&hs, "\3")) bad = 1;
					if (hs.begin != param_end) bad = 1;
					break;

					/* note: this switch () skips all unknown parameters */
				}
			}
			if (!bad && next) hs.begin = next;
			else break;
		}
		if (!url_name) bad = 1; /* file path parameter is mandatory */
	} else if (strncmp(line, "ed2k://|file|", 13) == 0) {
		hs.begin += 13;
		hs.expected_hash_id = RHASH_ED2K;
		hs.hash_type = HV_HEX;
		if (hash_check_find_str(&hs, "\4|\5|\3|")) {
			url_name = hs.url_name;
			url_length = hs.url_length;
		} else bad = 1;

		/* try to parse optional AICH hash */
		hs.expected_hash_id = RHASH_AICH;
		hs.hash_type = (HV_HEX | HV_B32); /* AICH is usually base32-encoded*/
		hash_check_find_str(&hs, "h=\3|");
	} else {
		if (hash_check_find_str(&hs, "\1 ( $ ) = \3")) {
			/* BSD-formatted line has been processed */
		} else if (hash_check_find_str(&hs, "$\6\2")) {
			while (hash_check_find_str(&hs, "$\6\2"));
			if (hashes->hashes_num > 1) reversed = 1;
		} else if (hash_check_find_str(&hs, "\2\7")) {
			if (hs.begin == hs.end) {
				/* the line contains no file path, only a single hash */
				single_hash = 1;
			} else {
				while (hash_check_find_str(&hs, "\2\6"));
				/* drop an asterisk before filename if present */
				if (*hs.begin == '*') hs.begin++;
			}
		} else bad = 1;

		if (hs.begin >= hs.end && !single_hash) bad = 1;
	}

	if (bad) {
		log_warning(_("can't parse line: %s\n"), line);
		return 0;
	}

	assert(hashes->file_path == 0);

	/* if !single_hash then we shall extract filepath from the line */
	if (url_name) {
		hashes->file_path = url_name;
		url_name[url_length] = '\0';
		urldecode(url_name); /* decode filename from URL */
		process_backslashes(url_name);
	} else if (!single_hash) {
		assert(hs.begin < hs.end);
		hashes->file_path = hs.begin;
		*hs.end = '\0';
		process_backslashes(hs.begin);
	}

	if (reversed) {
		/* change the order of hash values from reversed back to forward */
		for (i = 0, j = hashes->hashes_num - 1; i < j; i++, j--) {
			hash_value tmp = hashes->hashes[i];
			hashes->hashes[i] = hashes->hashes[j];
			hashes->hashes[j] = tmp;
		}
	}

	/* post-process parsed hashes */
	for (i = 0; i < hashes->hashes_num; i++) {
		hash_value *hv = &hashes->hashes[i];
		char *hash_str = hashes->data + hv->offset;

		if (hv->hash_id == 0) {
			/* calculate hash mask */
			unsigned mask = 0;
			if (hv->format & HV_HEX) {
				mask |= hash_check_mask_by_digest_size(hv->length >> 1);
			}
			if (hv->format & HV_B32) {
				assert(((hv->length * 5 / 8) & 3) == 0);
				mask |= hash_check_mask_by_digest_size(hv->length * 5 / 8);
			}
			assert(mask != 0);
			if ((mask & opt.sum_flags) != 0) mask &= opt.sum_flags;
			hv->hash_id = mask;
		}
		hashes->hash_mask |= hv->hash_id;

		/* change the hash string to be upper-case */
		for (j = 0; j < (int)hv->length; j++) {
			if (hash_str[j] >= 'a') hash_str[j] &= ~0x20;
		}
		hash_str[j] = '\0'; /* terminate the hash string */
	}

	return 1;
}

/**
 * Forward and reverse hex string compare. Compares two hexadecimal strings
 * using forward and reversed byte order. The function is used to compare
 * GOST hashes which can be reversed, because byte order of
 * an output string is not specified by GOST standard.
 * The function acts almost the same way as memcmp, but always returns
 * 1 for unmatched strings.
 *
 * @param mem1 the first byte string
 * @param mem2 the second byte string
 * @param size the length of byte strings to much
 * @return 0 if strings are matched, 1 otherwise.
 */
static int fr_hex_cmp(const void* mem1, const void* mem2, size_t size)
{
	const char *p1, *p2, *pe;
	if (memcmp(mem1, mem2, size) == 0) return 0;
	if ((size & 1) != 0) return 1; /* support only even size */

	p1 = (const char*)mem1, p2 = ((const char*)mem2) + size - 2;
	for (pe = ((const char*)mem1) + size / 2; p1 < pe; p1 += 2, p2 -= 2) {
		if (p1[0] != p2[0] || p1[1] != p2[1]) return 1;
	}
	return 0;
}

/**
 * Obtain CRC32 from rhash context. The function assumes that
 * context contains CRC32 and makes no checks for this.
 *
 * @param rhash context
 * @return crc32 hash sum
 */
unsigned get_crc32(struct rhash_context* ctx)
{
	unsigned char c[4];
	rhash_print((char*)c, ctx, RHASH_CRC32, RHPR_RAW);
	return ((unsigned)c[0] << 24) | ((unsigned)c[1] << 16) |
			((unsigned)c[2] << 8) | (unsigned)c[3];
}

/**
 * Verify calculated hashes against original values.
 * Also verify the file size and embedded CRC32 if present.
 * The HC_WRONG_* bits are set in the hashes->flags field on fail.
 *
 * @param hashes 'original' parsed hash values, to verify against
 * @param ctx the rhash context containing calculated hash values
 * @return 1 on success, 0 on fail
 */
int hash_check_verify(hash_check* hashes, struct rhash_context* ctx)
{
	unsigned unverified_mask;
	unsigned hid;
	unsigned printed;
	char hex[132], b32[104];
	int j;

	/* verify file size, if present */
	if ((hashes->flags & HC_HAS_FILESIZE) != 0 && hashes->file_size != ctx->msg_size)
		hashes->flags |= HC_WRONG_FILESIZE;

	/* verify embedded CRC32 hash sum, if present */
	if ((hashes->flags & HC_HAS_EMBCRC32) != 0 && get_crc32(ctx) != hashes->embedded_crc32)
		hashes->flags |= HC_WRONG_EMBCRC32;

	/* return if nothing else to verify */
	if (hashes->hashes_num == 0)
		return !HC_FAILED(hashes->flags);

	unverified_mask = (1 << hashes->hashes_num) - 1;

	for (hid = 1; hid <= RHASH_ALL_HASHES; hid <<= 1) {
		if ((hashes->hash_mask & hid) == 0) continue;
		printed = 0;

		for (j = 0; j < hashes->hashes_num; j++) {
			hash_value *hv = &hashes->hashes[j];
			char *hash_str, *hash_orig;
			int dgst_size;

			/* skip already verified hashes and hashes with different digest size */
			if (!(unverified_mask & (1 << j)) || !(hv->hash_id & hid)) continue;
			dgst_size = rhash_get_digest_size(hid);
			if (hv->length == (dgst_size * 2)) {
				assert(hv->format & HV_HEX);
				assert(hv->length <= 128);

				/* print hexadecimal value, if not printed yet */
				if ((printed & HV_HEX) == 0) {
					rhash_print(hex, ctx, hid, RHPR_HEX | RHPR_UPPERCASE);
					printed |= HV_HEX;
				}
				hash_str = hex;
			} else {
				assert(hv->format & HV_B32);
				assert(hv->length == BASE32_LENGTH(dgst_size));
				assert(hv->length <= 103);

				/* print base32 value, if not printed yet */
				if ((printed & HV_B32) == 0) {
					rhash_print(b32, ctx, hid, RHPR_BASE32 | RHPR_UPPERCASE);
					printed |= HV_B32;
				}
				hash_str = b32;
			}
			hash_orig = hashes->data + hv->offset;

			if ((hid & (RHASH_GOST | RHASH_GOST_CRYPTOPRO)) != 0) {
				if (fr_hex_cmp(hash_orig, hash_str, hv->length) != 0) continue;
			} else {
				if (memcmp(hash_orig, hash_str, hv->length) != 0) continue;
			}

			unverified_mask &= ~(1 << j); /* the j-th hash verified */
			hashes->found_hash_ids |= hid;

			/* end loop if all hashes were successfully verified */
			if (unverified_mask == 0) goto hc_verify_exit;
		}
	}

hc_verify_exit: /* we use label/goto to jump out of two nested loops  */
	hashes->wrong_hashes = unverified_mask;
	if (unverified_mask != 0) hashes->flags |= HC_WRONG_HASHES;
	return !HC_FAILED(hashes->flags);
}
