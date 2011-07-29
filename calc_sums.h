/* calc_sums.h */
#ifndef CALC_SUMS_H
#define CALC_SUMS_H

#include <stdint.h>
#include "librhash/rhash.h"
#include "common_func.h"

#ifdef __cplusplus
extern "C" {
#endif

/* binary result of calculations */
typedef struct rhash_sums_t
{
	unsigned flags;
	union {
		unsigned char digest[4];
		unsigned be;
	} crc32;
	unsigned char md4_digest[16];
	unsigned char md5_digest[16];
	unsigned char ed2k_digest[16];
	unsigned char sha1_digest[20];
	unsigned char tiger_digest[24];
	unsigned char tth_digest[24];
	unsigned char aich_digest[20];
	unsigned char whirlpool_digest[64];
	unsigned char ripemd160_digest[20];
	unsigned char gost_digest[32];
	unsigned char gost_cryptopro_digest[32];
	unsigned char snefru256_digest[32];
	unsigned char snefru128_digest[16];
	unsigned char has160_digest[20];
	unsigned char btih_digest[20];
	unsigned char sha224_digest[28];
	unsigned char sha256_digest[32];
	unsigned char sha384_digest[48];
	unsigned char sha512_digest[64];
	unsigned char edonr256_digest[32];
	unsigned char edonr512_digest[64];
} rhash_sums_t;

#include <sys/types.h> /* struct stat */
#include <sys/stat.h> /* stat() */

/* information about currently processed file */
struct file_info {
	char* full_path;
	const char* print_path;
	char* utf8_print_path;
	uint64_t    size;     /* the size of the file */
	double time; /* file processing time in seconds */
	struct infohash_ctx *infohash;
	rhash rctx;  /* state of hash algorithms */
	int error;  /* -1 for i/o error, -2 for wrong sum, 0 on success */
	char* allocated_ptr;

	/* note: rsh_stat_struct size depends on _FILE_OFFSET_BITS */
	struct rsh_stat_struct stat_buf; /* file attributes */

	unsigned wrong_sums;  /* sum comparison results */
	struct rhash_sums_t sums; /* sums of the file */
	struct rhash_sums_t *orig_sums; /* sums from a crc file */
};

void file_info_destroy(struct file_info*); /* free allocated memory */
const char* file_info_get_utf8_print_path(struct file_info*);
unsigned char* rhash_get_digest_ptr(struct rhash_sums_t *sums, unsigned hash_id);

int calculate_and_print_sums(FILE* out, const char *print_path, const char *full_path, struct rsh_stat_struct* stat_buf);
int check_hash_file(const char* crc_file_path, int chdir);
int rename_file_to_embed_crc32(struct file_info *info);
void print_sfv_banner(FILE* out);
int print_sfv_header_line(FILE* out, const char* printpath, const char* fullpath);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CALC_SUMS_H */
