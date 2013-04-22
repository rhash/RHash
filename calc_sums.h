/* calc_sums.h */
#ifndef CALC_SUMS_H
#define CALC_SUMS_H

#include <stdint.h>
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
	/* the file path (in system encoding). It can be changed
	 * if a crc sum is embedded into the filename. */
	char* full_path;
	const char* print_path; /* the part of the path for printing */
	char* utf8_print_path;  /* file path in UTF8 */
	uint64_t size;          /* the size of the hashed file */
	double time;            /* file processing time in seconds */
	struct file_t* file;    /* the file being processed */
	struct rhash_context* rctx; /* state of hash algorithms */
	int error;  /* -1 for i/o error, -2 for wrong sum, 0 on success */
	char* allocated_ptr;

	unsigned sums_flags; /* mask of ids of calculated hash functions */
	struct hash_check hc; /* hash values parsed from a hash file */
};

void file_info_destroy(struct file_info*); /* free allocated memory */
const char* file_info_get_utf8_print_path(struct file_info*);

void save_torrent_to(const char* path, struct rhash_context* rctx);
int calculate_and_print_sums(FILE* out, file_t* file, const char *print_path);
int check_hash_file(file_t* file, int chdir);
int rename_file_by_embeding_crc32(struct file_info *info);
void print_sfv_banner(FILE* out);
int print_sfv_header_line(FILE* out, file_t* file, const char* printpath);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CALC_SUMS_H */
