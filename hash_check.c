/* hash_check.c - verification of hashes of files */

#include "hash_check.h"
#include "common_func.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "librhash/rhash.h"
#include <assert.h>
#include <ctype.h>  /* isspace */
#include <string.h>

/* hash conversion macros and functions */
#define HEX_TO_DIGIT(c) ((c) <= '9' ? (c) & 0xF : ((c) - 'a' + 10) & 0xF)
#define BASE32_TO_DIGIT(c) ((c) < 'A' ? (c) - '2' + 26 : ((c) & ~0x20) - 'A')
#define BASE32_LENGTH(bits) (((bits) + 4) / 5)
#define BASE64_LENGTH(bits)  (((bits) + 5) / 6)
#define BASE32_BIT_SIZE(length) (((length) * 5) & ~7)
#define BASE64_BIT_SIZE(length) (((length) * 6) & ~7)

/* pack a character sequence into a single unsigned integer */
#define THREEC2U(c1, c2, c3) (((unsigned)(c1) << 16) | \
	((unsigned)(c2) << 8) | (unsigned)(c3))
#define FOURC2U(c1, c2, c3, c4) (((unsigned)(c1) << 24) | \
	((unsigned)(c2) << 16) | ((unsigned)(c3) << 8) | (unsigned)(c4))

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
		*(bin++) = HEX_TO_DIGIT(*(str++));
		len--;
	}

	/* parse the rest - an even-sized hexadecimal string */
	for (; len >= 2; len -= 2, str += 2) {
		*(bin++) = (HEX_TO_DIGIT(str[0]) << 4) | HEX_TO_DIGIT(str[1]);
	}
}

/**
 * Decode an URL-encoded string in the specified buffer.
 *
 * @param buffer the 0-terminated URL-encoded string
 */
static void urldecode(char* buffer)
{
	char* wpos = buffer; /* set writing position */
	for (; *buffer; wpos++) {
		*wpos = *(buffer++); /* copy non-escaped characters */
		if (*wpos == '%') {
			if (*buffer == '%') {
				buffer++; /* interpret '%%' as single '%' */
			} else if (IS_HEX(*buffer)) {
				/* decode character from the %<hex-digit><hex-digit> form */
				int ch = HEX_TO_DIGIT(*buffer);
				buffer++;
				if (IS_HEX(*buffer)) {
					ch = (ch << 4) | HEX_TO_DIGIT(*buffer);
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
	for (; *path; path++) {
		if (*path == '\\')
			*path = '/';
	}
}
#else /* _WIN32 */
#define process_backslashes(path)
#endif /* _WIN32 */

/* convert a hash flag to index */
#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) /* GCC >= 3.4 */
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
		unsigned hash_id;
		for (hash_id = 1; hash_id <= RHASH_ALL_HASHES; hash_id <<= 1) {
			code = code_digest_size(rhash_get_digest_size(hash_id));
			assert(0 <= code && code <= 7);
			if (code >= 0)
				mask[code] |= hash_id;
		}
		mask[9] = 1;
	}
	code = code_digest_size(digest_size);
	return (code >= 0 ? mask[code] : 0);
}

enum FmtBitFlags {
	FmtHex   = 1,
	FmtBase32LoweCase = 2,
	FmtBase32UpperCase = 4,
	FmtBase64   = 8,
	FmtBase32 = (FmtBase32LoweCase | FmtBase32UpperCase),
	FmtAll   = (FmtHex | FmtBase32 | FmtBase64)
};

/**
 * Test if a character is a hexadecimal/base32 digit.
 *
 * @param c the character to test
 * @return a combination of Fmt* bits
 */
static int test_hash_char(char c)
{
	static unsigned char char_bits[80] = {
		8, 0, 0, 0, 8, 9, 9, 15, 15, 15, 15, 15, 15, 9, 9, 0, 0, 0, 0, 0,
		0, 0, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
		12, 12, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0, 0, 0, 11, 11, 11, 11, 11, 11,
		10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
	};
	c -= '+';
	return ((unsigned)c <= 80 ? char_bits[(unsigned)c] : 0);
}

/**
 * Detect if given string contains a hexadecimal or base32 hash.
 *
 * @param ptr the pointer to start scanning from
 * @param end pointer to scan to
 * @param p_len pointer to a number to store length of detected hash string
 * @return type of detected hash as combination of Fmt* flags
 */
static int detect_hash_type(char** ptr, char* end, int* p_len)
{
	char* p = *ptr;
	size_t len = 0;
	size_t eq_num = 0;
	int char_type = 0;
	int next_type = FmtAll;

	if (p < end) {
		/* search forward (but no more then 129 symbols) */
		if ((end - p) >= 129) end = p + 129;
		for (; p <= end && (next_type &= test_hash_char(*p)); len++, p++)
			char_type = next_type;
		if ((char_type & FmtBase64) && *p == '=')
		{
			char_type = FmtBase64;
			for (; *p == '=' && p <= end; eq_num++, p++);
		}
	} else {
		/* search backward (but no more then 129 symbols) */
		if ((p - end) >= 129) end = p - 129;
		for (; p >= end && p[-1] == '='; eq_num++, p--)
			char_type = FmtBase64;
		for (; p >= end && (next_type &= test_hash_char(p[-1])); len++, p--)
			char_type = next_type;
	}
	if ((char_type & FmtBase64) != 0)
	{
		size_t hash_len = (len * 6) & ~7;
		if (eq_num > 3 || ((len + eq_num) & 3) || len != (hash_len + 5) / 6)
			char_type &= ~FmtBase64;
	}
	*ptr = p;
	*p_len = (int)len;
	return char_type;
}

/**
 * Check if a hash with of the specified bit length is supported by the program.
 *
 * @param length the bit length of a binary string
 * @return 1 if a hash of the specified bit length is supported, 0 otherwise
 */
static int is_acceptable_bit_length(int length)
{
	if ((length & 31) == 0 && length <= 512)
	{
		int pow = get_ctz(length >> 5);
		int code = ((length >> (pow + 6)) << 3) | pow;
		return (code < 32 && ((1 << code) & 0x101061d));
	}
	return 0;
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
static unsigned char test_hash_string(char** ptr, char* end, int* p_len)
{
	int len = 0;
	int char_type = detect_hash_type(ptr, end, &len);
	unsigned char hash_type = 0;

	if ((char_type & FmtHex) && is_acceptable_bit_length(len * 4))
		hash_type |= FmtHex;
	if ((char_type & FmtBase32) && is_acceptable_bit_length(BASE32_BIT_SIZE(len)))
		hash_type |= FmtBase32;
	if ((char_type & FmtBase64) && is_acceptable_bit_length(BASE64_BIT_SIZE(len)))
		hash_type |= FmtBase64;
	if (hash_type != 0)
		*p_len = len;
	return hash_type;
}

/**
 * Detect a hash-function by name.
 *
 * @param name an uppercase string, a possible name of a hash-function
 * @param length length of the name string
 * @return id of hash function if found, zero otherwise
 */
static unsigned detect_hash_id(const char* name, unsigned length)
{
#define code2mask_size (18 * 2)
	static unsigned code2mask[code2mask_size] = {
		FOURC2U('A', 'I', 'C', 'H'), RHASH_AICH,
		FOURC2U('B', 'T', 'I', 'H'), RHASH_BTIH,
		FOURC2U('C', 'R', 'C', '3'), (RHASH_CRC32 | RHASH_CRC32C),
		FOURC2U('E', 'D', '2', 'K'), RHASH_ED2K,
		FOURC2U('E', 'D', 'O', 'N'), (RHASH_EDONR256 | RHASH_EDONR512),
		FOURC2U('G', 'O', 'S', 'T'),
			(RHASH_GOST12_256 | RHASH_GOST12_512 | RHASH_GOST94 | RHASH_GOST94_CRYPTOPRO),
		FOURC2U('H', 'A', 'S', '1'), RHASH_HAS160,
		FOURC2U('M', 'D', '4', 0),   RHASH_MD4,
		FOURC2U('M', 'D', '5', 0),   RHASH_MD5,
		FOURC2U('R', 'I', 'P', 'E'), RHASH_RIPEMD160,
		FOURC2U('S', 'H', 'A', '1'), RHASH_SHA1,
		FOURC2U('S', 'H', 'A', '2'), (RHASH_SHA224 | RHASH_SHA256),
		FOURC2U('S', 'H', 'A', '3'),
			(RHASH_SHA384 | RHASH_SHA3_224 | RHASH_SHA3_256 | RHASH_SHA3_384 | RHASH_SHA3_512),
		FOURC2U('S', 'H', 'A', '5'), RHASH_SHA512,
		FOURC2U('S', 'N', 'E', 'F'), (RHASH_SNEFRU128 | RHASH_SNEFRU256),
		FOURC2U('T', 'I', 'G', 'E'), RHASH_TIGER,
		FOURC2U('T', 'T', 'H', 0),   RHASH_TTH,
		FOURC2U('W', 'H', 'I', 'R'), RHASH_WHIRLPOOL
	};
	unsigned code, i, hash_mask, hash_id;
	char ch;
	if (length < 3) return 0;
	ch = (name[3] != '-' ? name[3] : name[4]);
	code = FOURC2U(name[0], name[1], name[2], ch);
	for (i = 0; code2mask[i] != code; i += 2)
		if (i >= (code2mask_size - 2)) return 0;
	hash_mask = code2mask[i + 1];
	i = get_ctz(hash_mask);
	if (length <= 4)
	{
		assert((hash_mask & (hash_mask - 1)) == 0);
		return (length == strlen(hash_info_table[i].name) ? hash_mask : 0);
	}
	/* look for the hash_id in the hash_mask */
	for (hash_id = 1 << i; hash_id && hash_id <= hash_mask; i++, hash_id <<= 1)
	{
		const char* a;
		const char* b;
		if ((hash_id & hash_mask) == 0) continue;
		assert(length > 4 && strlen(hash_info_table[i].name) > 4);
		/* check the name tail, start from name[3] to detect names like "SHA-1" or "SHA256" */
		for (a = hash_info_table[i].name + 3, b = name + 3; *a; a++, b++)
		{
			if (*a == *b) continue;
			if (*a == '-')
				b--;
			else if (*b == '-')
				a--;
			else
				break;
		}
		if (!*a && !*b) return hash_id;
	}
	return 0;
}

/**
 * Detect ASCII-7 white spaces (not including Unicode whitespaces).
 * Note that isspace() is locale specific and detect Unicode spaces,
 * like U+00A0.
 *
 * @param ch character to check
 * @return non-zero if ch is space, zero otherwise
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
static int hash_check_find_str(hc_search* search, const char* format)
{
	int backward = 0;
	char buf[20];
	const char* fend = strchr(format, '\0');
	char* begin = search->begin;
	char* end = search->end;
	hash_check* hc = search->hc;
	hash_value hv;
	int unsaved_hashval = 0;
	memset(&hv, 0, sizeof(hash_value));

	while (format < fend) {
		const char* search_str;
		int len = 0;
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
			for (; (begin[len] <= '9' ? begin[len] >= '0' || begin[len] == '-' : begin[len] >= 'A'); len++) {
				if (len >= 20) return 0; /* limit name length */
				buf[len] = toupper(begin[len]);
			}
			buf[len] = '\0';
			search->expected_hash_id = detect_hash_id(buf, len);
			if (!search->expected_hash_id) return 0;
			search->hash_type = FmtAll;
			begin += len;
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
				int bit_length = rhash_get_digest_size(search->expected_hash_id) * 8;
				hv.format &= search->hash_type;
				if ((len * 4) != bit_length)
					hv.format &= ~FmtHex;
				if (len != BASE32_LENGTH(bit_length))
					hv.format &= ~FmtBase32;
				if (len != BASE64_LENGTH(bit_length))
					hv.format &= ~FmtBase64;
				if (!hv.format)
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
			if ((backward ? *(--end) : *(begin++)) != *search_str)
				return 0;
		}
	}

	if (unsaved_hashval && hc->hashes_num < HC_MAX_HASHES) {
		hc->hashes[hc->hashes_num++] = hv;
	}
	search->begin = begin;
	search->end = end;

	return 1;
}

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
	if (line[0] == '\0' || (le[-1] != '\n' && check_eol)) return 0;

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
					if (FOURC2U(hs.begin[-4], hs.begin[-3], hs.begin[-2], hs.begin[-1]) !=
							FOURC2U('u', 'r', 'n', ':'))
						return 0;
					/* find hash by its magnet link specific URN name  */
					for (i = 0; i < RHASH_HASH_COUNT; i++) {
						const char* urn = rhash_get_magnet_name(1 << i);
						size_t len = hf_end - hs.begin;
						if (strncmp(hs.begin, urn, len) == 0 && urn[len] == '\0')
							break;
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
					hs.hash_type = (FmtHex | FmtBase32);
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
		hs.hash_type = FmtHex;
		if (hash_check_find_str(&hs, "\4|\5|\3|")) {
			url_name = hs.url_name;
			url_length = hs.url_length;
		} else bad = 1;

		/* try to parse optional AICH hash */
		hs.expected_hash_id = RHASH_AICH;
		hs.hash_type = (FmtHex | FmtBase32); /* AICH is usually base32-encoded*/
		hash_check_find_str(&hs, "h=\3|");
	} else {
		if (hash_check_find_str(&hs, "\1 ( $ ) = \3")) {
			/* BSD-formatted line has been processed */
		} else if (hash_check_find_str(&hs, "$\6\2")) {
			while (hash_check_find_str(&hs, "$\6\2"));
			if (hashes->hashes_num > 1)
				reversed = 1;
		} else if (hash_check_find_str(&hs, "\2\7")) {
			if (hs.begin == hs.end) {
				/* the line contains no file path, only a single hash */
				single_hash = 1;
			} else {
				while (hash_check_find_str(&hs, "\2\6"));
				/* drop an asterisk before filename if present */
				if (*hs.begin == '*')
					hs.begin++;
			}
		} else bad = 1;

		if (hs.begin >= hs.end && !single_hash)
			bad = 1;
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
		hash_value* hv = &hashes->hashes[i];
		char* hash_str = hashes->data + hv->offset;
		hash_str[hv->length] = '\0'; /* terminate the hash string */

		if (hv->hash_id == 0) {
			/* calculate hash mask */
			unsigned mask = 0;
			if (hv->format & FmtHex) {
				mask |= hash_check_mask_by_digest_size(hv->length >> 1);
			}
			if (hv->format & FmtBase32) {
				assert(((hv->length * 5 / 8) & 3) == 0);
				mask |= hash_check_mask_by_digest_size(BASE32_BIT_SIZE(hv->length) / 8);
			}
			if (hv->format & FmtBase64) {
				mask |= hash_check_mask_by_digest_size(BASE64_BIT_SIZE(hv->length) / 8);
			}
			assert(mask != 0);
			if ((mask & opt.sum_flags) != 0)
				mask &= opt.sum_flags;
			hv->hash_id = mask;
		}
		hashes->hash_mask |= hv->hash_id;
	}
	return 1;
}

enum {
	CompareHashCaseSensitive = 1,
	CompareHashReversed = 2
};

/**
 * Compare two hash strings. For base64 encoding, the case-sensitive comparasion is done.
 * For hexadecimal or base32 encodings, the case-insensitive match occurs.
 * For the GOST94 hash, the additional reversed case-insensitive match is done.
 *
 * @param calculated_hash the calculated hash, for the hex/base32 the hash is an upper-case string.
 * @param expected a hash value from a hash file to match against
 * @param length length of the hash strings
 * @param comparision_mode 0, CompareHashCaseSensitive or CompareHashReversed comparision mode
 */
static int is_hash_string_equal(const char* calculated_hash, const char* expected, size_t length, int comparision_mode)
{
	if (comparision_mode == CompareHashCaseSensitive)
		return (memcmp(calculated_hash, expected, length) == 0);
	{
		/* case-insensitive comparision of a hexadecimal or a base32 hash */
		size_t i = 0;
		for (; i < length && (calculated_hash[i] == (expected[i] >= 'a' ? expected[i] & ~0x20 : expected[i])); i++);
		if (i == length)
			return 1;
	}
	if (comparision_mode == CompareHashReversed) {
		/* case-insensitive comparision of reversed gost hash */
		size_t i = 0, last = length - 1;
		for (; i < length && (calculated_hash[last - (i ^ 1)] == (expected[i] >= 'a' ? expected[i] & ~0x20 : expected[i])); i++);
		return (i == length);
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
 * @return 1 on successfull verification, 0 on hash sums mismatch
 */
int do_hash_sums_match(hash_check* hashes, struct rhash_context* ctx)
{
	unsigned unverified_mask;
	unsigned hash_id;
	unsigned printed;
	char hex[132], base32[104], base64[88];
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

	for (hash_id = 1; hash_id <= RHASH_ALL_HASHES && unverified_mask; hash_id <<= 1) {
		if ((hashes->hash_mask & hash_id) == 0)
			continue;
		printed = 0;

		for (j = 0; j < hashes->hashes_num; j++) {
			hash_value* hv = &hashes->hashes[j];
			char* calculated_hash;
			char* expected_hash;
			int bit_length;
			int comparision_mode;

			/* skip already verified hashes and hashes with different digest size */
			if (!(unverified_mask & (1 << j)) || !(hv->hash_id & hash_id))
				continue;
			comparision_mode = 0;
			bit_length = rhash_get_digest_size(hash_id) * 8;
			if ((hv->length * 4) == bit_length) {
				assert(hv->format & FmtHex);
				assert(hv->length <= 128);

				/* print hexadecimal value, if not printed yet */
				if ((printed & FmtHex) == 0) {
					rhash_print(hex, ctx, hash_id, RHPR_HEX | RHPR_UPPERCASE);
					printed |= FmtHex;
				}
				calculated_hash = hex;
				if ((hash_id & (RHASH_GOST94 | RHASH_GOST94_CRYPTOPRO)) != 0)
					comparision_mode = CompareHashReversed;
			} else if (hv->length == BASE32_LENGTH(bit_length)) {
				assert(hv->format & FmtBase32);
				assert(hv->length <= 103);

				/* print base32 value, if not printed yet */
				if ((printed & FmtBase32) == 0) {
					rhash_print(base32, ctx, hash_id, RHPR_BASE32 | RHPR_UPPERCASE);
					printed |= FmtBase32;
				}
				calculated_hash = base32;
			} else {
				assert(hv->format & FmtBase64);
				assert(hv->length == BASE64_LENGTH(bit_length));
				assert(hv->length <= 86);

				/* print base32 value, if not printed yet */
				if ((printed & FmtBase64) == 0) {
					rhash_print(base64, ctx, hash_id, RHPR_BASE64);
					printed |= FmtBase64;
				}
				calculated_hash = base64;
				comparision_mode = CompareHashCaseSensitive;
			}
			expected_hash = hashes->data + hv->offset;
			if (!is_hash_string_equal(calculated_hash, expected_hash, hv->length, comparision_mode))
				continue;

			unverified_mask &= ~(1 << j); /* the j-th hash verified */
			hashes->found_hash_ids |= hash_id;

			/* end the loop if all hashes were successfully verified */
			if (unverified_mask == 0)
				break;
		}
	}

	hashes->wrong_hashes = unverified_mask;
	if (unverified_mask != 0)
		hashes->flags |= HC_WRONG_HASHES;
	return !HC_FAILED(hashes->flags);
}
