/* hash_check.c - functions to parse and verify a hash file contianing message digests */

#include "hash_check.h"
#include "calc_sums.h"
#include "common_func.h"
#include "hash_print.h"
#include "output.h"
#include "parse_cmdline.h"
#include "rhash_main.h"
#include "librhash/rhash.h"
#include <assert.h>
#include <ctype.h>  /* isspace */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* size of the buffer to receive a hash file line */
#define LINE_BUFFER_SIZE 4096

/* message digest conversion macros and functions */
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
 * Decode an URL-encoded string into the specified buffer.
 *
 * @param buffer buffer to decode string into
 * @param buffer_size buffer size
 * @param src URL-encoded string
 * @param src_length length of the string to decode
 * @return length of the decoded string
 */
static size_t urldecode(char* buffer, size_t buffer_size, const char* src, size_t src_length)
{
	char* dst = buffer;
	char* dst_back = dst + buffer_size - 1;
	const char* src_end = src + src_length;
	assert(src_length < buffer_size);
	for (; src < src_end && dst < dst_back; dst++) {
		*dst = *(src++); /* copy non-escaped characters */
		if (*dst == '%') {
			if (*src == '%') {
				src++; /* interpret '%%' as single '%' */
			} else if (IS_HEX(*src)) {
				/* decode character from the %<hex-digit><hex-digit> form */
				int ch = HEX_TO_DIGIT(*src);
				src++;
				if (IS_HEX(*src)) {
					ch = (ch << 4) | HEX_TO_DIGIT(*src);
					src++;
				}
				*dst = (char)ch;
			}
		}
	}
	assert(dst <= dst_back);
	*dst = '\0'; /* terminate decoded string */
	return (dst - buffer);
}

/**
 * Decode a string with escaped characters into the specified buffer.
 *
 * @param buffer buffer to decode string into
 * @param buffer_size buffer size
 * @param src string with escaped characters
 * @param src_length length of the source string
 * @return length of the decoded string
 */
static size_t unescape_characters(char* buffer, size_t buffer_size, const char* src, size_t src_length)
{
	char* dst = buffer;
	char* dst_back = dst + buffer_size - 1;
	const char* src_end = src + src_length;
	assert(src_length < buffer_size);
	for (; src < src_end && dst < dst_back; dst++) {
		*dst = *(src++); /* copy non-escaped characters */
		if (*dst == '\\') {
			if (*src == '\\') {
				src++; /* interpret '\\' as a single '\' */
			} else if (*src == 'n') {
				*dst = '\n';
				src++;
			}
		}
	}
	assert(dst <= dst_back);
	*dst = '\0'; /* terminate decoded string */
	return (dst - buffer);
}

/* convert a hash function bit-flag to the index of the bit */
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
 * Calculate a bit-mask of hash-ids by a length of message digest in bytes.
 *
 * @param digest_size length of a binary message digest in bytes
 * @return mask of hash-ids with given hash length, 0 on fail
 */
static unsigned hash_bitmask_by_digest_size(int digest_size)
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

/**
 * Bit flags corresponding to possible formats of a message digest.
 */
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
 * @param p_len pointer to a number to store length of detected message digest
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
		for (; p > end && p[-1] == '='; eq_num++, p--)
			char_type = FmtBase64;
		for (; p > end && (next_type &= test_hash_char(p[-1])); len++, p--)
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
 * Check if a message digest of the specified bit length is supported by the program.
 *
 * @param length the bit length of a binary message digest value
 * @return 1 if a message digest of the specified bit length is supported, 0 otherwise
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
 * Test the given substring to be a hexadecimal or base32
 * message digest of one of the supported hash functions.
 *
 * @param ptr the pointer to start scanning from
 * @param end pointer to scan to
 * @param p_len pointer to a number to store length of detected message digest
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

enum HashNameMatchModes {
	ExactMatch,
	PrefixMatch
};

/**
 * Detect a hash-function id by its BSD name.
 *
 * @param name an uppercase string, a possible name of a hash-function
 * @param length length of the name string
 * @param match_mode whether the name parameter must match the name of a
 *                   known hash algorithm exactly, or trailing chars are
 *                   allowed.
 * @return id of hash function if found, zero otherwise
 */
static unsigned bsd_hash_name_to_id(const char* name, unsigned length, enum HashNameMatchModes match_mode)
{
#define code2mask_size (19 * 2)
	static unsigned code2mask[code2mask_size] = {
		FOURC2U('A', 'I', 'C', 'H'), RHASH_AICH,
		FOURC2U('B', 'L', 'A', 'K'), (RHASH_BLAKE2S | RHASH_BLAKE2B),
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
	char fourth_char;
	if (length < 3) return 0;
	fourth_char = (name[3] != '-' ? name[3] : name[4]);
	code = FOURC2U(name[0], name[1], name[2], fourth_char);
	/* quick fix to detect "RMD160" as RIPEMD160 */
	if (code == FOURC2U('R', 'M', 'D', '1'))
		return (length == 6 && name[4] == '6' && name[5] == '0' ? RHASH_RIPEMD160 : 0);
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
		if ((hash_id & hash_mask) == 0)
			continue;
		assert(length > 4);
		assert(strlen(hash_info_table[i].name) >= 4);
		/* check the name tail, starting from &name[3] to detect names like "SHA-1" or "SHA256" */
		for (a = hash_info_table[i].name + 3, b = name + 3; *a; a++, b++)
		{
			if (*a == *b)
				continue;
			else if (*a == '-')
				b--;
			else if (*b == '-')
				a--;
			else
				break;
		}
		if (!*a && (match_mode == PrefixMatch || !*b))
			return hash_id;
	}
	return 0;
}

/**
 * Detect ASCII-7 white spaces (0x09=\t, 0x0a=\n, 0x0b=\v, 0x0c=\f, 0x0d=\r, 0x20=' ').
 * Note that standard C function isspace() is locale specific and
 * detects Unicode spaces, like U+00A0.
 *
 * @param ch character to check
 * @return non-zero if ch is space, zero otherwise
 */
static int rhash_isspace(char ch)
{
	unsigned code = (unsigned)(ch - 9);
	return (code < 0x18 && ((1u << code) & 0x80001f));
}

/**
 * Extended hash parser data.
 */
struct hash_parser_ext {
	struct hash_parser hp;
	file_t* hash_file;
	file_t parent_dir;
	FILE* fd;
	uint64_t expected_hash_mask;
	unsigned is_sfv;
	unsigned line_number;
	char buffer[LINE_BUFFER_SIZE];
};

/**
 * Information about found token.
 */
struct hash_token
{
	char* begin; /* start of the buffer to parse */
	char* end;   /* end of the buffer to parse */
	file_t* p_parsed_path;
	struct hash_parser_ext* parser;
	struct hash_value* p_hashes;
	unsigned expected_hash_id;
	int hash_type;
};

/**
 * Bit-flags for the fstat_path_token().
 */
enum FstatPathBitFlags {
	PathUrlDecode = 1,
	PathUnescape = 2
};

static int fstat_path_token(struct hash_token* token, char* path, size_t path_length, int is_url);

/**
 * Bit-flags for the match_hash_tokens().
 */
enum MhtOptionsBitFlags {
	MhtFstatPath = 1,
	MhtAllowOneHash = 2,
	MhtAllowEscapedPath = 4
};

/**
 * Constants returned by match_hash_tokens() and some other functions.
 */
enum MhtResults {
	ResFailed = 0,
	ResAllMatched = 1,
	ResPathNotExists = 2,
	ResOneHashDetected = 3
};

/**
 * Parse the buffer pointed by token->begin, into tokens specified by format string.
 * The format string can contain the following special characters:
 * '\1' - BSD hash function name, '\2' - any hash, '\3' - specified hash,
 * '\4' - an URL-encoded file name, '\5' - a file size,
 * '\6' - a required space or line end.
 * A space ' ' means 0 or more space characters.
 * '$' - parse the rest of the buffer and the format string backward.
 * Other (non-special) symbols mean themselves.
 * The function updates token->begin and token->end pointers on success.
 *
 * @param token the structure to store parsed tokens info
 * @param format the format string
 * @param bit_flags MhtFstatPath flag to run fstat on parsed path,
 *              MhtAllowOneHash to allow line containing one message digest without a file path
 * @return one of the MhtResults constants
 */
static int match_hash_tokens(struct hash_token* token, const char* format, unsigned bit_flags)
{
	int backward = 0;
	char buf[20];
	const char* fend = strchr(format, '\0');
	char* begin = token->begin;
	char* end = token->end;
	char* url_path = 0;
	size_t url_length;
	struct hash_parser* hp = &token->parser->hp;
	struct hash_value hv;
	int unsaved_hashval = 0;
	int unescape_flag = 0;
	memset(&hv, 0, sizeof(hv));

	if ((bit_flags & MhtAllowEscapedPath) && (opt.flags & OPT_NO_PATH_ESCAPING) == 0 &&
			begin[0] == '\\' && !(begin[1] == '\\' && begin[2] == '?' && begin[3] == '\\')) {
		begin++;
		unescape_flag = PathUnescape;
	}

	while (format < fend) {
		const char* search_str;
		int len = 0;
		uint64_t file_size;

		if (backward) {
			for (; fend[-1] >= '-' && format < fend; fend--, len++);
			if (len == 0)
				fend--;
			search_str = fend;
		} else {
			search_str = format;
			for (; *format >= '-' && format < fend; format++, len++);
			if (len == 0)
				format++;
		}
		if (len > 0) {
			if ((end - begin) < len)
				return ResFailed;
			if (0 != memcmp(search_str, (backward ? end - len : begin), len))
				return ResFailed;
			if (backward)
				end -= len;
			else
				begin += len;
			continue;
		}
		assert(len == 0);

		/* find substring specified by single character */
		switch (*search_str) {
		case '\1': /* parse BSD hash function name */
			/* the name should contain alphanumeric or '-' symbols, but */
			/* actually the loop shall stop at characters [:& \(\t] */
			for (; (begin[len] <= '9' ? begin[len] >= '0' || begin[len] == '-' : begin[len] >= 'A'); len++) {
				if (len >= 20)
					return ResFailed; /* limit name length */
				buf[len] = toupper(begin[len]);
			}
			buf[len] = '\0';
			token->expected_hash_id = bsd_hash_name_to_id(buf, len, ExactMatch);
			if (!token->expected_hash_id)
				return ResFailed;
			token->hash_type = FmtAll;
			begin += len;
			break;
		case '\2':
		case '\3':
			if (backward) {
				hv.format = test_hash_string(&end, begin, &len);
				hv.offset = (unsigned short)(end - hp->line_begin);
			} else {
				hv.offset = (unsigned short)(begin - hp->line_begin);
				hv.format = test_hash_string(&begin, end, &len) & token->hash_type;
			}
			if (!hv.format)
				return ResFailed;
			if (*search_str == '\3') {
				/* verify message digest type */
				int bit_length = rhash_get_digest_size(token->expected_hash_id) * 8;
				hv.format &= token->hash_type;
				if ((len * 4) != bit_length)
					hv.format &= ~FmtHex;
				if (len != BASE32_LENGTH(bit_length))
					hv.format &= ~FmtBase32;
				if (len != BASE64_LENGTH(bit_length))
					hv.format &= ~FmtBase64;
				if (!hv.format)
					return ResFailed;
				hv.hash_id = token->expected_hash_id;
			} else hv.hash_id = 0;
			hv.length = (unsigned char)len;
			unsaved_hashval = 1;
			break;
		case '\4': /* get URL-encoded name */
			url_path = begin;
			url_length = strcspn(begin, "?&|");
			if (url_length == 0)
				return ResFailed; /* empty name */
			begin += url_length;
			break;
		case '\5': /* retrieve file size */
			assert(!backward);
			file_size = 0L;
			for (; '0' <= *begin && *begin <= '9'; begin++, len++) {
				file_size = file_size * 10 + (*begin - '0');
			}
			if (len == 0)
				return ResFailed;
			else {
				hp->file_size = file_size;
				hp->bit_flags |= HpHasFileSize;
			}
			break;
		case '\6':
		case ' ':
			if (backward)
				for (; begin < end && rhash_isspace(end[-1]); end--, len++);
			else
				for (; rhash_isspace(*begin) && begin < end; begin++, len++);
			/* check if space is mandatory */
			if (*search_str != ' ' && len == 0) {
				/* for '\6' verify (len > 0 || (MhtAllowOneHash is set && begin == end)) */
				if (!(bit_flags & MhtAllowOneHash) || begin < end)
					return ResFailed;
			}
			break;
		case '$':
			backward = 1; /* switch to parsing string backward */
			break;
		default:
			if ((backward ? *(--end) : *(begin++)) != *search_str)
				return ResFailed;
		}
	}

	if (unsaved_hashval && hp->hashes_num < HP_MAX_HASHES) {
		token->p_hashes[hp->hashes_num++] = hv;
	}
	token->begin = begin;
	token->end = end;
	if ((bit_flags & MhtAllowOneHash) != 0 && hp->hashes_num == 1 && begin == end)
	{
		struct hash_parser_ext* const parser = token->parser;
		file_t* parsed_path = &parser->hp.parsed_path;

		if (file_modify_path(parsed_path, parser->hash_file, NULL, FModifyRemoveExtension) < 0) {
			/* note: trailing whitespaces were removed from line by hash_parser_parse_line() */
			return ResFailed;
		}
		if ((bit_flags & MhtFstatPath) != 0 && file_stat(parsed_path, 0) < 0)
			hp->parsed_path_errno = errno;
		return ResOneHashDetected;
	}

	if ((bit_flags & MhtFstatPath) != 0) {
		int fstat_res;
		if (url_path != 0) {
			fstat_res = fstat_path_token(token, url_path, url_length, PathUrlDecode);
		} else {
			size_t path_length;
			if (begin[0] == '*' && unescape_flag == 0)
				begin++;
			path_length = end - begin;
			fstat_res = fstat_path_token(token, begin, path_length, unescape_flag);
		}
		if (fstat_res < 0)
			return ResPathNotExists;
	}
	return ResAllMatched;
}

/**
 * Fstat given file path. Optionally prepend it, if needed, by parent directory.
 * Depending on bit_flags, the path is url-decoded or decoded using escape sequences.
 * Fstat result is stored into token->p_parsed_path.
 *
 * @param token current tokens parsing context
 * @param str pointer to the path start
 * @param str_length length of the path
 * @param bit_flags PathUrlDecode or PathUnescape to decode path
 * @return 0 on success, -1 on fstat fail
 */
static int fstat_path_token(struct hash_token* token, char* str, size_t str_length, int bit_flags)
{
	static char buffer[LINE_BUFFER_SIZE];
	file_t* parent_dir = &token->parser->parent_dir;
	unsigned init_flags = (FILE_IS_IN_UTF8(token->parser->hash_file) ?
		FileInitRunFstat | FileInitUtf8PrintPath : FileInitRunFstat);
	char* path = (bit_flags == 0 ? str : buffer);
	size_t path_length = (bit_flags == 0 ? str_length : (bit_flags & PathUrlDecode ?
		urldecode(buffer, LINE_BUFFER_SIZE, str, str_length) :
		unescape_characters(buffer, LINE_BUFFER_SIZE, str, str_length)));
	int res;
	int is_absolute = IS_PATH_SEPARATOR(path[0]);
	char saved_char = path[path_length];
	path[path_length] = '\0';

	IF_WINDOWS(is_absolute = is_absolute || (path[0] && path[1] == ':'));
	if (is_absolute || !parent_dir->real_path)
		parent_dir = NULL;
	if ((bit_flags & PathUnescape) != 0)
		init_flags |= FileInitUseRealPathAsIs;

	res = file_init_by_print_path(token->p_parsed_path, parent_dir, path, init_flags);
	if (res < 0 && token->p_parsed_path == &token->parser->hp.parsed_path)
		token->parser->hp.parsed_path_errno = errno;
	path[path_length] = saved_char;
	return res;
}

/**
 * Cleanup token context and fill hash_mask of the parser.
 *
 * @param token token parsing context
 * @return ResPathNotExists if path has not been found, ResAllMatched otherwise
 */
static int finalize_parsed_data(struct hash_token* token)
{
	int i;
	struct hash_parser* const parser = &token->parser->hp;
	if (token->p_parsed_path != &parser->parsed_path) {
		file_cleanup(&parser->parsed_path);
		parser->parsed_path = *token->p_parsed_path;
	}
	if (FILE_ISBAD(&parser->parsed_path) || !parser->parsed_path.real_path)
		return ResPathNotExists;

	if (token->p_hashes != parser->hashes) {
		assert(parser->hashes_num == 1);
		parser->hashes[0] = token->p_hashes[0];
	}

	/* post-process parsed message digests */
	for (i = 0; i < parser->hashes_num; i++) {
		struct hash_value* hv = &parser->hashes[i];
		char* hash_str = parser->line_begin + hv->offset;
		hash_str[hv->length] = '\0'; /* terminate the message digest */

		if (hv->hash_id == 0) {
			/* calculate bit-mask of hash function ids */
			unsigned mask = 0;
			if (hv->format & FmtHex) {
				mask |= hash_bitmask_by_digest_size(hv->length >> 1);
			}
			if (hv->format & FmtBase32) {
				assert(((hv->length * 5 / 8) & 3) == 0);
				mask |= hash_bitmask_by_digest_size(BASE32_BIT_SIZE(hv->length) / 8);
			}
			if (hv->format & FmtBase64) {
				mask |= hash_bitmask_by_digest_size(BASE64_BIT_SIZE(hv->length) / 8);
			}
			assert(mask != 0);
			if ((mask & token->parser->expected_hash_mask) != 0)
				mask &= token->parser->expected_hash_mask;
			hv->hash_id = mask;
		}
		parser->hash_mask |= hv->hash_id;
	}
	return ResAllMatched;
}

 /**
  * Parse the rest of magnet link.
  *
 * @param token tokens parsing context
  * @return ResAllMatched on success, ResFailed on a bad magnet link
  */
static int parse_magnet_url(struct hash_token* token)
{
	char* url_path = 0;
	size_t url_length;

	/* loop by magnet link parameters */
	while (1) {
		char* next = strchr(token->begin, '&');
		char* param_end = (next ? next++ : token->end);
		char* hf_end;

		if ((token->begin += 3) < param_end) {
			switch (THREEC2U(token->begin[-3], token->begin[-2], token->begin[-1])) {
			case THREEC2U('d', 'n', '='): /* URL-encoded file path */
				url_path = token->begin;
				url_length = param_end - token->begin;
				break;
			case THREEC2U('x', 'l', '='): /* file size */
				if (!match_hash_tokens(token, "\5", 0))
					return ResFailed;
				if (token->begin != param_end)
					return ResFailed;
				break;
			case THREEC2U('x', 't', '='): /* a file hash */
				{
					int i;
					/* find last ':' character (hash name can be complex like tree:tiger) */
					for (hf_end = param_end - 1; *hf_end != ':' && hf_end > token->begin; hf_end--);

					/* test for the "urn:" string */
					if ((token->begin += 4) >= hf_end)
						return ResFailed;
					if (FOURC2U(token->begin[-4], token->begin[-3], token->begin[-2], token->begin[-1]) !=
							FOURC2U('u', 'r', 'n', ':'))
						return ResFailed;
					/* find hash by its magnet link specific URN name  */
					for (i = 0; i < RHASH_HASH_COUNT; i++) {
						const char* urn = rhash_get_magnet_name(1 << i);
						size_t len = hf_end - token->begin;
						if (strncmp(token->begin, urn, len) == 0 && urn[len] == '\0')
							break;
					}
					if (i >= RHASH_HASH_COUNT) {
						if (opt.flags & OPT_VERBOSE) {
							*hf_end = '\0';
							log_warning(_("unknown hash in magnet link: %s\n"), token->begin);
						}
						return ResFailed;
					}
					token->begin = hf_end + 1;
					token->expected_hash_id = 1 << i;
					token->hash_type = (FmtHex | FmtBase32);
					if (!match_hash_tokens(token, "\3", 0))
						return ResFailed;
					if (token->begin != param_end)
						return ResFailed;
					break;
				}
				/* note: this switch () skips all unknown parameters */
			}
		}
		if (next)
			token->begin = next;
		else
			break;
	}
	if (!url_path || token->parser->hp.hashes_num == 0)
		return ResFailed;
	fstat_path_token(token, url_path, url_length, PathUrlDecode);
	return finalize_parsed_data(token);
}

 /**
  * Parse the rest of ed2k link.
  *
  * @param token the structure to store parsed tokens info
  * @return ResAllMatched on success, ResFailed on a bad ed2k link
  */
static int parse_ed2k_link(struct hash_token* token)
{
	token->expected_hash_id = RHASH_ED2K;
	token->hash_type = FmtHex;
	if (!match_hash_tokens(token, "\4|\5|\3|", MhtFstatPath))
		return ResFailed;
	/* try to parse optional AICH hash */
	token->expected_hash_id = RHASH_AICH;
	token->hash_type = (FmtHex | FmtBase32); /* AICH is usually base32-encoded*/
	match_hash_tokens(token, "h=\3|", 0);
	return finalize_parsed_data(token);
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
 * @param parser structure to store parsed message digests, file path and file size
 * @param check_eol boolean flag meaning that '\n' at the end of the line is required
 * @return non-zero on success (ResAllMatched, ResOneHashDetected, ResPathNotExists),
 *         ResFailed if couldn't parse the line
 */
static int parse_hash_file_line(struct hash_parser_ext* parser, int check_eol)
{
	struct hash_token token;
	char* line = parser->hp.line_begin;
	char* line_end = strchr(line, '\0');
	int res;

	assert(line[0] != '\0');

	/* return if EOL not found at the end of the line */
	if (line_end[-1] != '\n' && check_eol)
		return ResFailed;
	/* trim line manually, without str_trim(), cause we'll need line_end later */
	while (rhash_isspace(line_end[-1]) && line_end > line)
		*(--line_end) = 0;
	/* skip white spaces at the start of the line */
	while (rhash_isspace(*line))
		line++;

	memset(&token, 0, sizeof(token));
	token.begin = line;
	token.end = line_end;
	token.parser = parser;
	token.p_parsed_path = &parser->hp.parsed_path;
	token.p_hashes = parser->hp.hashes;

	memset(&parser->hp, 0, sizeof(parser->hp));
	parser->hp.line_begin = line;
	parser->hp.file_size = (uint64_t)-1;

	/* check for a minimum acceptable message digest length */
	if ((line + 8) > line_end)
		return ResFailed;

	if (strncmp(token.begin, "ed2k://|file|", 13) == 0) {
		token.begin += 13;
		return parse_ed2k_link(&token);
	}
	if (strncmp(token.begin, "magnet:?", 8) == 0) {
		token.begin += 8;
		return parse_magnet_url(&token);
	}
	/* check for BSD-formatted line has been processed */
	res = match_hash_tokens(&token, "\1 ( $ ) = \3", MhtFstatPath | MhtAllowEscapedPath);
	if (res != ResFailed)
		return finalize_parsed_data(&token);
	token.hash_type = FmtAll;

	{
		const unsigned is_sfv_format = parser->is_sfv;
		unsigned valid_direction = 0;
		unsigned state;
		unsigned token_flags = (MhtFstatPath | MhtAllowEscapedPath | MhtAllowOneHash);

		struct file_t secondary_path;
		struct hash_token secondary_token;
		struct hash_token *cur_token = &token;
		struct hash_value secondary_hash[1];

		memset(&secondary_path, 0, sizeof(secondary_path));
		memcpy(&secondary_token, &token, sizeof(token));
		secondary_token.p_hashes = secondary_hash;

		/* parse one hash from each line end */
		for (state = 0; state < 2; state++)
		{
			const unsigned is_backward = (state ^ is_sfv_format);
			const char* token_format = (is_backward ? "$\6\2" : "\2\6");
			int res = match_hash_tokens(cur_token, token_format, token_flags);
			if (res == ResAllMatched || res == ResOneHashDetected)
			{
				return finalize_parsed_data(cur_token);
			}
			else if (res == ResPathNotExists)
			{
				assert(parser->hp.hashes_num == 1);
				valid_direction |= (1 << state); /* mark the current parsing direction as valid */
				if (token.p_parsed_path == &parser->hp.parsed_path)
					token.p_parsed_path = secondary_token.p_parsed_path = &secondary_path;
				cur_token = &secondary_token;
				parser->hp.hashes_num = 0;
			}
			token_flags &= ~MhtAllowOneHash;
		}
		token_flags = MhtFstatPath;

		/* parse the rest of hashes */
		for (state = 0; state < 2; state++)
		{
			if ((valid_direction & (1 << state)) != 0)
			{
				const unsigned is_backward = (state ^ is_sfv_format);
				const char* token_format = (is_backward ? "$\6\2" : "\2\6");

				/* restore parsing state and a parsed hash */
				parser->hp.hashes_num = 1;
				if (state == 1 && valid_direction == 3)
				{
					token.begin = secondary_token.begin;
					token.end = secondary_token.end;
					token.p_hashes[0] = secondary_token.p_hashes[0];
				}
				/* allow FmtBase64 only if the first hash can be interpreted only as Base64-encoded */
				token.hash_type = (token.hash_type & ~FmtBase64) |
					(token.p_hashes[0].format == FmtBase64 ? FmtBase64 : 0);
				for (;;)
				{
					int res = match_hash_tokens(&token, token_format, token_flags);
					assert(res != ResOneHashDetected);
					if (res == ResAllMatched)
						return finalize_parsed_data(&token);
					else if (res == ResFailed)
						break;
				}
			}
		}
		file_cleanup(&secondary_path);
		if (token.p_parsed_path != &parser->hp.parsed_path)
			return ResPathNotExists;
	}
	return ResFailed; /* could not parse line */
}

/**
 * Bit-flags for the hash.
 */
enum CompareHashBitFlags {
	CompareHashCaseSensitive = 1,
	CompareHashReversed = 2
};

/**
 * Compare two message digests. For base64 encoding, the case-sensitive comparasion is done.
 * For hexadecimal or base32 encodings, the case-insensitive match occurs.
 * For the GOST94 hash, the additional reversed case-insensitive match is done.
 *
 * @param calculated_hash the calculated message digest, for the hex/base32 the value must be in upper case
 * @param expected a message digest from a hash file to match against
 * @param length length of the message digests
 * @param comparision_mode 0, CompareHashCaseSensitive or CompareHashReversed comparision mode
 * @return 1 if message digests match, 0 otherwise
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
 * @return crc32 checksum
 */
unsigned get_crc32(struct rhash_context* ctx)
{
	unsigned char c[4];
	rhash_print((char*)c, ctx, RHASH_CRC32, RHPR_RAW);
	return ((unsigned)c[0] << 24) | ((unsigned)c[1] << 16) |
			((unsigned)c[2] << 8) | (unsigned)c[3];
}

/**
 * Verify calculated message digests against original values.
 * Also verify the file size and embedded CRC32 if present.
 * The HP_WRONG_* bits are set in the parser->flags field on fail.
 *
 * @param parser 'original' parsed message digests, to verify against
 * @param ctx the rhash context containing calculated message digests
 * @return 1 on successfull verification, 0 on message digests mismatch
 */
static int do_hash_sums_match(struct hash_parser* parser, struct rhash_context* ctx)
{
	unsigned unverified_mask;
	unsigned hash_id;
	unsigned printed;
	char hex[132], base32[104], base64[88];
	int j;

	/* verify file size, if present */
	if ((parser->bit_flags & HpHasFileSize) != 0 && parser->file_size != ctx->msg_size)
		parser->bit_flags |= HpWrongFileSize;

	/* verify embedded CRC32 checksum, if present */
	if ((parser->bit_flags & HpHasEmbeddedCrc32) != 0 && get_crc32(ctx) != parser->embedded_crc32)
		parser->bit_flags |= HpWrongEmbeddedCrc32;

	/* return if nothing else to verify */
	if (parser->hashes_num == 0)
		return !HP_FAILED(parser->bit_flags);

	unverified_mask = (1 << parser->hashes_num) - 1;

	for (hash_id = 1; hash_id <= RHASH_ALL_HASHES && unverified_mask; hash_id <<= 1) {
		if ((parser->hash_mask & hash_id) == 0)
			continue;
		printed = 0;

		for (j = 0; j < parser->hashes_num; j++) {
			struct hash_value* hv = &parser->hashes[j];
			char* calculated_hash;
			char* expected_hash;
			int bit_length;
			int comparision_mode;

			/* skip already verified message digests and message digests of different size */
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
			expected_hash = parser->line_begin + hv->offset;
			if (!is_hash_string_equal(calculated_hash, expected_hash, hv->length, comparision_mode))
				continue;

			unverified_mask &= ~(1 << j); /* mark the j-th message digest as verified */
			parser->found_hash_ids |= hash_id;

			/* end the loop if all message digests were successfully verified */
			if (unverified_mask == 0)
				break;
		}
	}

	parser->wrong_hashes = unverified_mask;
	if (unverified_mask != 0)
		parser->bit_flags |= HpWrongHashes;
	return !HP_FAILED(parser->bit_flags);
}

/**
 * Verify message digests of the file.
 * In a case of fail, the error will be logged.
 *
 * @param file the file, which hashes are verified
 * @param hp parsed hash file line
 * @return 0 on success, 1 on message digests mismatch,
 *     -1/-2 on input/output error
 */
static int verify_hashes(file_t* file, struct hash_parser* hp)
{
	struct file_info info;
	timedelta_t timer;
	int res = 0;

	if (FILE_ISBAD(file) && (opt.flags & OPT_IGNORE_MISSING) != 0)
		return -1;

	memset(&info, 0, sizeof(info));
	info.file = file;
	info.sums_flags = hp->hash_mask;
	info.hp = hp;

	/* initialize percents output */
	if (init_percents(&info) < 0) {
		log_error_file_t(&rhash_data.out_file);
		return -2;
	}

	rsh_timer_start(&timer);
	if (FILE_ISBAD(info.file)) {
		/* restore errno */
		errno = hp->parsed_path_errno;
		res = -1;
	} else {
		res = calc_sums(&info);
	}
	if (res < 0) {
		/* report file error */
		int output_res = finish_percents(&info, -1);
		return (output_res < 0 ? -2 : -1);
	}
	info.time = rsh_timer_stop(&timer);

	if (rhash_data.stop_flags) {
		report_interrupted();
		return 0;
	}
	if ((opt.flags & OPT_EMBED_CRC) &&
			find_embedded_crc32(info.file, &hp->embedded_crc32)) {
		hp->bit_flags |= HpHasEmbeddedCrc32;
		assert(hp->hash_mask & RHASH_CRC32);
	}
	if (!do_hash_sums_match(hp, info.rctx))
		res = 1;
	if (finish_percents(&info, res) < 0)
		res = -2;
	if ((opt.flags & OPT_SPEED) && info.sums_flags != 0)
		print_file_time_stats(&info);
	return res;
}

/**
 * Verify the file against the CRC32 checksum embedded into its filename.
 *
 * @param file the file to verify
 * @return 0 on success, -1 on input error, -2 on results output error
 */
int check_embedded_crc32(file_t* file)
{
	int res = 0;
	unsigned crc32;
	struct hash_parser hp;
	if (find_embedded_crc32(file, &crc32)) {
		/* initialize file_info structure */
		memset(&hp, 0, sizeof(hp));
		hp.hash_mask = RHASH_CRC32;
		hp.bit_flags = HpHasEmbeddedCrc32;
		hp.embedded_crc32 = crc32;

		res = verify_hashes(file, &hp);
		if (res >= -1 && fflush(rhash_data.out) < 0) {
			log_error_file_t(&rhash_data.out_file);
			res = -2;
		} else if (!rhash_data.stop_flags) {
			if (res == 0)
				rhash_data.ok++;
			else if (res == -1 && errno == ENOENT)
				rhash_data.miss++;
			rhash_data.processed++;
		}
	} else {
		/* TRANSLATORS: sample filename with embedded CRC32: file_[A1B2C3D4].mkv */
		log_warning_msg_file_t(_("file name doesn't contain a CRC32: %s\n"), file);
		return -1;
	}
	return res;
}

/**
 * Structure to store file extension and its length.
 */
struct file_ext {
	size_t length;
	char buffer[20];
};

/**
 * Extract file extension from given file.
 *
 * @param fe buffer to recieve file extension
 * @param file the file to process
 * @return 1 on success, 0 on fail
 */
static int extract_uppercase_file_ext(struct file_ext* fe, file_t* file)
{
	size_t length;
	char* buffer;
	const char* basename = file_get_print_path(file, FPathUtf8 | FPathBaseName);
	const char* ext;
	if (!basename)
		return 0;
	ext = strrchr(basename, '.');
	if (!ext)
		/* If there is no extension, then consider the whole filename
		 * as extension, so callers can do the right thing when
		 * encountering a file called e.g. "SHA256". */
		ext = basename;
	else
		ext++;
	buffer = fe->buffer;
	for (length = 0; '-' <= ext[length] && ext[length] <= 'z'; length++) {
		if (length >= sizeof(fe->buffer))
			return 0; /* limit hash name length */
		buffer[length] = toupper(ext[length]);
	}
	if (ext[length] != '\0')
		return 0;
	buffer[length] = '\0';
	fe->length = length;
	return 1;
}

/**
 * Detect SFV format and hash functions by the hash file extension.
 *
 * @param hash_file the file, which extension is checked
 * @param parser the parser to store hash_mask and sfv flag into
 */
static void set_flags_by_hash_file_extension(file_t* hash_file, struct hash_parser_ext* parser)
{
	struct file_ext ext;
	unsigned hash_mask;
	int is_sfv;
	if(HAS_OPTION(OPT_NO_DETECT_BY_EXT) || !extract_uppercase_file_ext(&ext, hash_file))
		return;
	hash_mask = (opt.sum_flags ? opt.sum_flags : bsd_hash_name_to_id(ext.buffer, ext.length, PrefixMatch));
	is_sfv = (ext.length == 3 && memcmp(ext.buffer, "SFV", 3) == 0);
	if (parser != NULL) {
		parser->expected_hash_mask = hash_mask;
		parser->is_sfv = is_sfv;
	}
	if (IS_MODE(MODE_UPDATE)) {
		opt.sum_flags = hash_mask;
		if (!opt.fmt)
			rhash_data.is_sfv = is_sfv;
	}
}

/**
 * Close and cleanup hash parser.
 *
 * @param parser the hash parser to close
 * @return 0 on success, -1 on fail with error code stored in errno
 */
static int hash_parser_close(struct hash_parser* hp)
{
	struct hash_parser_ext* parser = (struct hash_parser_ext*)hp;
	int res = 0;
	if (!parser)
		return 0;
	file_cleanup(&parser->parent_dir);
	file_cleanup(&parser->hp.parsed_path);
	if (parser->fd != stdin)
		res = fclose(parser->fd);
	free(parser);
	return res;
}

/**
 * Open a hash file and create a hash_parser for it.
 *
 * @param hash_file the hash file to open
 * @param chdir true if function should emulate chdir to the parent directory of the hash file
 * @return created hash_parser on success, NULL on fail with error code stored in errno
 */
static struct hash_parser* hash_parser_open(file_t* hash_file, int chdir)
{
	FILE* fd;
	struct hash_parser_ext* parser;

	/* open file or prepare file descriptor */
	if (FILE_ISSTDIN(hash_file))
		fd = stdin;
	else if ( !(fd = file_fopen(hash_file, FOpenRead | FOpenBin) ))
		return NULL;

	/* allocate and initialize parser */
	parser = (struct hash_parser_ext*)rsh_malloc(sizeof(struct hash_parser_ext));
	memset(parser, 0, sizeof(struct hash_parser_ext));
	parser->hash_file = hash_file;
	parser->fd = fd;
	parser->expected_hash_mask = opt.sum_flags;
	parser->is_sfv = rhash_data.is_sfv;

	/* extract the parent directory of the hash file, if required */
	if (chdir)
		file_modify_path(&parser->parent_dir, hash_file, NULL, FModifyGetParentDir);
	if((opt.flags & OPT_NO_DETECT_BY_EXT) == 0)
		set_flags_by_hash_file_extension(hash_file, parser);
	return &parser->hp;
}

/**
 * Constants returned by hash_parser_process_line() function.
 */
enum ProcessLineResults {
	ResReadError = -1,
	ResStopParsing = 0,
	ResSkipLine = 1,
	ResParsedLine = 2,
	ResFailedToParse = 3
};

/**
 * Parse one line of the openned hash file.
 *
 * @param hp parser processing the hash file
 * @return one of the ProcessLineResults constants
 */
static int hash_parser_process_line(struct hash_parser* hp)
{
	struct hash_parser_ext* parser = (struct hash_parser_ext*)hp;
	char *line = parser->buffer;

	if (!fgets(parser->buffer, sizeof(parser->buffer), parser->fd)) {
		if (ferror(parser->fd)) {
			log_error_file_t(parser->hash_file);
			return ResReadError;
		}
		return ResStopParsing;
	}

	parser->line_number++;
	if (parser->line_number > 1)
		file_cleanup(&hp->parsed_path);

	/* skip unicode BOM, if found */
	if (STARTS_WITH_UTF8_BOM(line)) {
		line += 3;
		if (parser->line_number == 1)
			parser->hash_file->mode |= FileContentIsUtf8;
	}
	if (is_binary_string(line)) {
		const char* message = (IS_MODE(MODE_UPDATE) ?
		/* TRANSLATORS: it's printed, when a non-text hash file is encountered in --update mode */
			_("skipping binary file") :
			_("file is binary"));
		log_msg("%s:%u: %s\n",
			file_get_print_path(parser->hash_file, FPathPrimaryEncoding | FPathNotNull),
			parser->line_number, message);
		hp->bit_flags |= HpIsBinaryFile;
		return ResReadError;
	}
	/* silently skip comments and empty lines */
	if (*line == '\0' || *line == '\r' || *line == '\n' || IS_COMMENT(*line))
		return ResSkipLine;

	hp->line_begin = line;
	if (!parse_hash_file_line(parser, !feof(parser->fd))) {
		log_msg(_("%s:%u: can't parse line \"%s\"\n"),
			file_get_print_path(parser->hash_file, FPathPrimaryEncoding | FPathNotNull),
			parser->line_number, parser->hp.line_begin);
		return ResFailedToParse;
	}
	errno = 0;
	return ResParsedLine;
}

/**
 * Parse content of the openned hash file.
 *
 * @param parser hash parser, controlling parsing process
 * @param files pointer to the vector to load parsed file paths into
 * @return HashFileBits bit mask on success, -1 on input error, -2 on results output error
 */
static int hash_parser_process_file(struct hash_parser *parser, file_set* files)
{
	timedelta_t timer;
	uint64_t time;
	int parsing_res;
	int result = (HashFileExist | HashFileIsEmpty);

	if (IS_MODE(MODE_CHECK) && print_verifying_header(((struct hash_parser_ext*)parser)->hash_file) < 0) {
		log_error_file_t(&rhash_data.out_file);
		return -2;
	}
	rsh_timer_start(&timer);

	/* initialize statistics */
	rhash_data.processed = rhash_data.ok = rhash_data.miss = 0;
	rhash_data.total_size = 0;

	while((parsing_res = hash_parser_process_line(parser)) > ResStopParsing) {
		result &= ~HashFileIsEmpty;
		/* skip comments and empty lines */
		if (parsing_res == ResSkipLine)
			continue;
		if (parsing_res == ResFailedToParse) {
			result |= HashFileHasUnparsedLines;
		} else {
			if (files)
			{
				/* put UTF8-encoded file path into the file set */
				const char* path = file_get_print_path(&parser->parsed_path, FPathUtf8);
				if (path)
					file_set_add_name(files, path);
			}
			if (IS_MODE(MODE_CHECK)) {
				/* verify message digests of the file */
				int res = verify_hashes(&parser->parsed_path, parser);

				if (res >= -1 && fflush(rhash_data.out) < 0) {
					log_error_file_t(&rhash_data.out_file);
					return -2;
				}
				if (rhash_data.stop_flags || res <= -2) {
					return res; /* stop on fatal error */
				}

				/* update statistics */
				if (res == 0)
					rhash_data.ok++;
				else
				{
					if (FILE_ISBAD(&parser->parsed_path) && (opt.flags & OPT_IGNORE_MISSING) != 0)
						continue;
					if (res == -1 && errno == ENOENT)
					{
						result |= HashFileHasMissedFiles;
						rhash_data.miss++;
					}
					else
						result |= HashFileHasWrongHashes;
				}
			}
		}
		rhash_data.processed++;
	}
	if (parsing_res == ResReadError)
		return -1;

	time = rsh_timer_stop(&timer);

	if (IS_MODE(MODE_CHECK)) {
		/* check for a hash file errors */
		if (result >= 0 && (result & (HashFileHasWrongHashes | HashFileHasMissedFiles | HashFileHasUnparsedLines)) != 0)
			rhash_data.non_fatal_error = 1;

		if (result >= -1 && (print_verifying_footer() < 0 || print_check_stats() < 0)) {
			log_error_file_t(&rhash_data.out_file);
			result = -2;
		}
		if (HAS_OPTION(OPT_SPEED) && !IS_MODE(MODE_UPDATE) && rhash_data.processed > 1)
			print_time_stats(time, rhash_data.total_size, 1);
	}

	return result;
}


/**
 * Open the given hash file and process its content.
 *
 * @param hash_file the hash file to process
 * @param files pointer to the vector to load parsed file paths into
 * @param chdir true if function should emulate chdir to the parent directory of the hash file
 * @return HashFileBits bit mask on success, -1 on input error, -2 on results output error
 */
static int process_hash_file(file_t* hash_file, file_set* files, int chdir)
{
	int result;
	struct hash_parser *parser = hash_parser_open(hash_file, chdir);
	if (!parser) {
		/* in the update mode, a non-existent hash file will be created later */
		if (IS_MODE(MODE_UPDATE) && errno == ENOENT) {
			set_flags_by_hash_file_extension(hash_file, 0);
			return HashFileIsEmpty;
		}
		log_error_file_t(hash_file);
		return -1;
	}
	result = hash_parser_process_file(parser, files);
	if (result >= 0 && (hash_file->mode & FileContentIsUtf8) != 0)
		result |= HashFileHasBom;
	hash_parser_close(parser);
	return result;
}

/**
 * Verify message digests of the files listed in the given hash file.
 * Lines beginning with ';' and '#' are ignored.
 * In a case of fail, obtained error will be logged.
 *
 * @param hash_file the hash file, containing message digests to verify
 * @param chdir true if function should emulate chdir to directory of filepath before checking it
 * @return HashFileBits bit mask on success, -1 on input error, -2 on results output error
 */
int check_hash_file(file_t* hash_file, int chdir)
{
	return process_hash_file(hash_file, 0, chdir);
}

/**
 * Load a set of files from the specified hash file.
 * In a case of fail, errors will be logged.
 *
 * @param hash_file the hash file, containing message digests to load
 * @param files pointer to the vector to load parsed file paths into
 * @return HashFileBits bit mask on success, -1 on input error, -2 on results output error
 */
int load_updated_hash_file(file_t* hash_file, file_set* files)
{
	return process_hash_file(hash_file, files, 0);
}
