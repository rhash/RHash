/** @file test_utils.h timer and benchmarking functions */
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Benchmarking flag: don't print intermediate benchmarking info.
 */
#define RHASH_BENCHMARK_QUIET 1

/**
 * Benchmarking flag: measure the CPU "clocks per byte" speed.
 */
#define RHASH_BENCHMARK_CPB 2

/**
 * Benchmarking flag: print benchmark result in tab-delimed format.
 */
#define RHASH_BENCHMARK_RAW 4

/**
 * Benchmark a hash algorithm.
 *
 * @param hash_id hash algorithm identifier
 * @param flags benchmark flags
 * @param output the stream to print results
 */
void test_run_benchmark(unsigned hash_id, unsigned flags,
				   FILE* output);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* TEST_UTILS_H */
