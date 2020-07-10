/* hash_print.h - functions to print message digests */
#ifndef HASH_PRINT_H
#define HASH_PRINT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An element of a list specifying an output format.
 */
typedef struct print_item
{
	struct print_item* next;
	unsigned flags;
	unsigned hash_id;
	unsigned width;
	const char* data;
} print_item;

/**
 * Name and other info of a hash function
 */
typedef struct print_hash_info
{
	const char* name;
	const char* bsd_name;
	char short_name[20];
	char short_char;
} print_hash_info;

extern print_hash_info hash_info_table[];

struct file_info;
struct file_t;
struct strbuf_t;

/* initialization of static data */
void init_hash_info_table(void);
void init_printf_format(struct strbuf_t* out);

/* formatted output of message digests and file information */
print_item* parse_print_string(const char* format, unsigned* sum_mask);
int print_line(FILE* out, unsigned out_mode, print_item* list, struct file_info* info);
void free_print_list(print_item* list);

/* SFV format functions */
int print_sfv_banner(FILE* out);
int print_sfv_header_line(FILE* out, unsigned out_mode, struct file_t* file);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HASH_PRINT_H */
