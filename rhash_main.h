/* rhash_main.h */
#ifndef RHASH_MAIN_H
#define RHASH_MAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Runtime data.
 */
struct rhash_t
{
	FILE *out;
	FILE *log;
	FILE *upd_fd; /* descriptor of a crc file to update */
#ifdef _WIN32
	wchar_t* program_dir;
	unsigned saved_cursor_size;
	unsigned output_flags;
#endif

	char*  printf_str;
	struct print_item *print_list;
	struct strbuf_t *template_text;
	struct rhash_context* rctx;
	int interrupted; /* non-zero if program was interrupted */

	/* missed, ok and processed files statistics */
	unsigned processed;
	unsigned ok;
	unsigned miss;
	uint64_t total_size;
	uint64_t batch_size;

	int error_flag;     /* non-zero if any error occurred */
};

/** static variable, holding most of the runtime data */
extern struct rhash_t rhash_data;

void rhash_destroy(struct rhash_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_MAIN_H */
