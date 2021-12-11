/* calc_sums.h */
#ifndef CALC_SUMS_H
#define CALC_SUMS_H

#include "common_func.h"
#include "file.h"
#include "hash_check.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Information about a file to calculate/verify message digests for.
 */
struct file_info {
	uint64_t size;          /* the size of the hashed file */
	uint64_t msg_offset;    /* rctx->msg_size before hashing this file */
	uint64_t time;          /* file processing time in milliseconds */
	file_t* file;           /* the file being processed */
	struct rhash_context* rctx; /* state of hash algorithms */
	struct hash_parser* hp;  /* parsed line of a hash file */
	unsigned sums_flags;    /* mask of ids of calculated hash functions */
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
 * Benchmark a hash algorithm.
 *
 * @param hash_id hash algorithm identifier
 * @param flags benchmark flags, can contain BENCHMARK_CPB, BENCHMARK_RAW
 */
void run_benchmark(unsigned hash_id, unsigned flags);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CALC_SUMS_H */
