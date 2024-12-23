/* calc_sums.h */
#ifndef CALC_SUMS_H
#define CALC_SUMS_H

#include "common_func.h"
#include "file.h"
#include "hash_check.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hash function identifiers and bit masks */
#define bit64_to_hash_id(bit64) ((unsigned)RHASH_EXTENDED_BIT ^ get_ctz64(bit64))
#define hash_id_to_bit64(hash_id) ((hash_id) & RHASH_EXTENDED_BIT ? \
	(uint64_t)1 << (unsigned)((hash_id) & ~RHASH_EXTENDED_BIT): (uint64_t)(hash_id))
#define hash_id_to_extended(hash_id) ((hash_id) & RHASH_EXTENDED_BIT ? hash_id : \
	(unsigned)RHASH_EXTENDED_BIT ^ get_ctz(hash_id))

int hash_mask_to_hash_ids(uint64_t hash_mask, unsigned max_count,
	unsigned* hash_ids, unsigned* out_count);
int set_openssl_enabled_hash_mask(uint64_t hash_mask);
uint64_t get_openssl_supported_hash_mask(void);
uint64_t get_all_supported_hash_mask(void);

/* Hash function calculation */

/**
 * Information about a file to calculate/verify message digests for.
 */
struct file_info {
	uint64_t size;          /* the size of the hashed file */
	uint64_t msg_offset;    /* rctx->msg_size before hashing this file */
	uint64_t time;          /* file processing time in milliseconds */
	file_t* file;           /* the file being processed */
	struct rhash_context* rctx; /* state of hash algorithms */
	struct hash_parser* hp; /* parsed line of a hash file */
	uint64_t hash_mask;     /* mask of ids of calculated hash functions */
	int processing_result;  /* -1/-2 for i/o error, 0 on success, 1 on a hash mismatch */
};

int calc_sums(struct file_info* info);
int calculate_and_print_sums(FILE* out, file_t* out_file, file_t* file);
int find_embedded_crc32(file_t* file, unsigned* crc32);
int rename_file_by_embeding_crc32(struct file_info* info);
int save_torrent_to(file_t* torrent_file, struct rhash_context* rctx);

/* Benchmarking */

/** Benchmarking flag: measure the CPU "clocks per byte" speed */
#define BENCHMARK_CPB 1
/** Benchmarking flag: print benchmark result in tab-delimited format */
#define BENCHMARK_RAW 2

/**
 * Benchmark hash functions.
 *
 * @param hash_mask bit mask for hash functions to benchmark
 * @param flags benchmark flags, can contain BENCHMARK_CPB, BENCHMARK_RAW
 */
void run_benchmark(uint64_t hash_mask, unsigned flags);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CALC_SUMS_H */
