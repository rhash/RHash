/** @file rhash_timing.h timer and benchmarking functions */
#ifndef RHASH_TIMING_H
#define RHASH_TIMING_H

/****************************************************************************
* Warning: all functions in this file are deprecated and will be removed
* from the library in the future. They are still exported for backward
* compatibility.
****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RHASH_API
/**
 * Modifier for LibRHash functions.
 */
# define RHASH_API
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
/**
 * Platform-dependent time delta.
 */
typedef unsigned long long timedelta_t;

#else
# include <sys/time.h> /* for timeval */
/**
 * Platform-dependent time delta.
 */
typedef struct timeval timedelta_t;
#endif


/**
 * Start a timer.
 *
 * @deprecated This function shall be removed soon, since
 * it is not related to the hashing library main functionality.
 *
 * @param timer timer to start
 */
RHASH_API void rhash_timer_start(timedelta_t* timer);

/**
 * Stop given timer.
 *
 * @deprecated This function shall be removed soon, since
 * it is not related to the hashing library main functionality.
 *
 * @param timer the timer to stop
 * @return number of seconds timed
 */
RHASH_API double rhash_timer_stop(timedelta_t* timer);


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
 * @deprecated This function will be removed from the librhash API,
 * since it is not related to the hashing library main functionality.
 *
 * @param hash_id hash algorithm identifier
 * @param flags benchmark flags
 * @param output the stream to print results
 */
RHASH_API void rhash_run_benchmark(unsigned hash_id, unsigned flags,
				   FILE* output);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_TIMING_H */
