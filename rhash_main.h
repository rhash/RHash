/* rhash_main.h */
#ifndef RHASH_MAIN_H
#define RHASH_MAIN_H

#include "file.h"

#ifdef __cplusplus
extern "C" {
#endif

enum StopFlags {
	InterruptedFlag = 1,
	FatalErrorFlag = 2
};

/**
 * Runtime data.
 */
struct rhash_t
{
	FILE* out;
	FILE* log;
	file_t out_file;
	file_t log_file;
	file_t upd_file;
	file_t config_file;

	char*  printf_str;
	struct print_item* print_list;
	struct strbuf_t* template_text;
	struct update_ctx* update_context;
	struct rhash_context* rctx;
	unsigned stop_flags;
	int non_fatal_error;

	/* missed, ok and processed files statistics */
	unsigned processed;
	unsigned ok;
	unsigned miss;
	uint64_t total_size;
	uint64_t batch_size;
};

/** static variable, holding most of the runtime data */
extern struct rhash_t rhash_data;

void rhash_destroy(struct rhash_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_MAIN_H */
