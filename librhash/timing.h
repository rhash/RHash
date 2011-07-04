/* timing.h */
#ifndef TIMING_H
#define TIMING_H

#ifndef _WIN32
#include <sys/time.h> /* for timeval */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RHASH_API
/* modifier for LibRHash functions */
# define RHASH_API
#endif

/* timedelta_t and timing interface */
#ifdef _WIN32
typedef unsigned long long timedelta_t;
#else
typedef struct timeval timedelta_t;
#endif

/* timer functions */
RHASH_API void rhash_timer_start(timedelta_t* timer);
RHASH_API double rhash_timer_stop(timedelta_t* timer);

/* flags for running a benchmark */
#define RHASH_BENCHMARK_QUIET 1
#define RHASH_BENCHMARK_CPB 2
#define RHASH_BENCHMARK_RAW 4

RHASH_API void rhash_run_benchmark(unsigned hash_id, unsigned flags, FILE* output);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* TIMING_H */
