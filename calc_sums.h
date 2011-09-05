/* calc_sums.h */
#ifndef CALC_SUMS_H
#define CALC_SUMS_H

#include <stdint.h>
#include "librhash/rhash.h"
#include "common_func.h"
#include "hash_check.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h> /* struct stat */
#include <sys/stat.h> /* stat() */

/**
 * Information about a file to calculate/verify hashes for.
 */
struct file_info {
	char* full_path; /* file path (in system encoding) */
	const char* print_path; /* the path part to print */
	char* utf8_print_path; /* file path in UTF8 */
	uint64_t size; /* the size of the file */
	double time; /* file processing time in seconds */
	struct infohash_ctx *infohash;
	rhash rctx;  /* state of hash algorithms */
	int error;  /* -1 for i/o error, -2 for wrong sum, 0 on success */
	char* allocated_ptr;

	/* note: rsh_stat_struct size depends on _FILE_OFFSET_BITS */
	struct rsh_stat_struct stat_buf; /* file attributes */

	unsigned sums_flags; /* mask of ids of calculated hash functions */
	struct hash_check hc; /* hash values parsed from a hash file */
};

void file_info_destroy(struct file_info*); /* free allocated memory */
const char* file_info_get_utf8_print_path(struct file_info*);

int calculate_and_print_sums(FILE* out, const char *print_path, const char *full_path, struct rsh_stat_struct* stat_buf);
int check_hash_file(const char* hash_file_path, int chdir);
int rename_file_to_embed_crc32(struct file_info *info);
void print_sfv_banner(FILE* out);
int print_sfv_header_line(FILE* out, const char* printpath, const char* fullpath);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CALC_SUMS_H */
