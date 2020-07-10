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
	double time;            /* file processing time in seconds */
	file_t* file;           /* the file being processed */
	struct rhash_context* rctx; /* state of hash algorithms */
	int processing_result;  /* -1/-2 for i/o error, 0 on success, 1 on a hash mismatch */
	unsigned sums_flags; /* mask of ids of calculated hash functions */
	struct hash_check hc; /* message digests parsed from a hash file */
};

int save_torrent_to(file_t* torrent_file, struct rhash_context* rctx);
int calculate_and_print_sums(FILE* out, file_t* out_file, file_t* file);
int check_hash_file(file_t* file, int chdir);
int rename_file_by_embeding_crc32(struct file_info* info);

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
