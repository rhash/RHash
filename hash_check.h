/* hash_check.h - functions to parse a file with hash sums to verify it */
#ifndef HASH_CHECK_H
#define HASH_CHECK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* bit flags for hash_check.flags */
#define HC_HAS_FILESIZE 1
#define HC_HAS_EMBCRC32 2
#define HC_WRONG_FILESIZE 4
#define HC_WRONG_EMBCRC32 8
#define HC_WRONG_HASHES 16
#define HC_FAILED(flags) ((flags) & (HC_WRONG_FILESIZE | HC_WRONG_EMBCRC32 | HC_WRONG_HASHES))

#define HC_MAX_HASHES 32

/**
 * Parsed hash value.
 */
typedef struct hash_value
{
	unsigned hash_id; /* the id of hash, if it was detected */
	unsigned short offset;
	unsigned char length;
	unsigned char format;
} hash_value;

struct rhash_context;

/**
 * Parsed file info, like the path, size and file hash values.
 */
typedef struct hash_check
{
	char *file_path; /* parsed file path */
	uint64_t file_size; /* parsed file size, e.g. from magnet link */
	unsigned hash_mask; /* the mask of hash ids to verify against */
	unsigned flags; /* bit flags */
	unsigned embedded_crc32;  /* CRC32 embedded into filename */
	char *data; /* the buffer with the current hash file line */
	unsigned found_hash_ids; /* bit mask for matched hash ids */
	unsigned wrong_hashes;   /* bit mask for mismatched hashes */
	int hashes_num; /* number of parsed hashes */
	hash_value hashes[HC_MAX_HASHES];
} hash_check;

int hash_check_parse_line(char* line, hash_check* hashes, int check_eol);
int hash_check_verify(hash_check* hashes, struct rhash_context* ctx);

void rhash_base32_to_byte(const char* str, unsigned char* bin, int len);
void rhash_hex_to_byte(const char* str, unsigned char* bin, int len);
unsigned get_crc32(struct rhash_context* ctx);

/* note: IS_HEX() is defined on ASCII-8 while isxdigit() only when isascii()==true */
#define IS_HEX(c) ((c) <= '9' ? (c) >= '0' : (unsigned)(((c) - 'A') & ~0x20) <= ('F' - 'A' + 0U))
#define IS_BASE32(c) (((c) <= '7' ? ('2' <= (c)) : (unsigned)(((c) - 'A') & ~0x20) <= ('Z' - 'A' + 0U)))

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HASH_CHECK_H */
