/* hash_check.h - functions to parse and verify a hash file contianing message digests */
#ifndef HASH_CHECK_H
#define HASH_CHECK_H

#include "file.h"
#include "file_set.h"

#ifdef __cplusplus
extern "C" {
#endif

/* bit flags for hash_parser */
enum HpBitFlags {
	HpHasFileSize = 0x01,
	HpHasEmbeddedCrc32 = 0x02,
	HpWrongFileSize = 0x10,
	HpWrongEmbeddedCrc32 = 0x20,
	HpWrongHashes = 0x40,
	HpIsBinaryFile = 0x100
};

#define HP_FAILED(flags) ((flags) & (HpWrongFileSize | HpWrongEmbeddedCrc32 | HpWrongHashes))
#define HP_MAX_HASHES 32

/**
 * Parsed message digest.
 */
struct hash_value
{
	uint32_t hash_id; /* the id of hash, if it was detected */
	uint16_t offset;
	unsigned char length;
	unsigned char format;
};

/**
 * Parsed file info, like the path, size and file message digests.
 */
struct hash_parser {
	file_t parsed_path; /* parsed file path */
	uint64_t file_size; /* parsed file size, e.g. from magnet link */
	int parsed_path_errno;
	unsigned bit_flags;
	uint64_t found_hash_ids; /* bit mask for matched hash ids */
	uint64_t wrong_hashes;   /* bit mask for mismatched message digests */
	char* line_begin;
	unsigned embedded_crc32;  /* CRC32 embedded into filename */
	unsigned hash_mask; /* the mask of hash ids to verify against */
	int hashes_num; /* number of parsed message digests */
	struct hash_value hashes[HP_MAX_HASHES];
};

enum HashFileBits {
	HashFileExist = 0x01,
	HashFileIsEmpty = 0x02,
	HashFileHasBom = 0x04,
	HashFileHasWrongHashes = 0x08,
	HashFileHasMissedFiles = 0x10,
	HashFileHasUnparsedLines = 0x20
};

int load_updated_hash_file(struct file_t* hash_file, file_set* set);
int check_hash_file(struct file_t* hash_file, int chdir);
int check_embedded_crc32(struct file_t* file);

void rhash_base32_to_byte(const char* str, unsigned char* bin, int len);
void rhash_hex_to_byte(const char* str, unsigned char* bin, int len);

struct rhash_context;
unsigned get_crc32(struct rhash_context* ctx);

/* note: IS_HEX() is defined on ASCII-8 while isxdigit() only on ASCII-7 */
#define IS_HEX(c) ((c) <= '9' ? (c) >= '0' : (unsigned)(((c) - 'A') & ~0x20) <= ('F' - 'A' + 0U))

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HASH_CHECK_H */
