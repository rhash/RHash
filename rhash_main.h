/* rhash_main.h */
#ifndef RHASH_MAIN_H
#define RHASH_MAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rhash_t
{
	FILE *out;
	FILE *log;
	FILE *upd_fd; /* descriptor of a crc file to update */
	int  saved_console_codepage; /* saved codepage */
	unsigned saved_cursor_size;

	char*  printf;
	struct print_item *print_list;
	struct strbuf_t *template_text;
	/*struct file_info  *cur_file;*/

	/* missed, ok and processed files statistics */
	unsigned processed;
	unsigned ok;
	unsigned miss;
	uint64_t total_size;

	int error_flag;     /* non-zero if any error occurred */
};

extern struct rhash_t rhash_data;

void rhash_destroy(struct rhash_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_MAIN_H */
