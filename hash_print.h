/* hash_print.h - functions to print hash sums */
#ifndef HASH_PRINT_H
#define HASH_PRINT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An element of a list specifying an output format.
 */
typedef struct print_item {
	struct print_item *next;
	unsigned flags;
	unsigned hash_id;
	unsigned width;
	const char *data;
} print_item;

/**
 * Name and other info of a hash function
 */
typedef struct print_hash_info
{
	char short_name[16];
	char short_char;
	const char *name;
} print_hash_info;

extern print_hash_info hash_info_table[];

struct file_info;
struct file_t;
struct strbuf_t;

/* initialization of static data */
void init_hash_info_table(void);
void init_printf_format(struct strbuf_t* out);

/* formatted output of hash sums and file information */
print_item* parse_print_string(const char* format, unsigned *sum_mask);
void print_line(FILE* out, print_item* list, struct file_info *info);
void free_print_list(print_item* list);

/* SFV format functions */
void print_sfv_banner(FILE* out);
int print_sfv_header_line(FILE* out, struct file_t* file, const char* printpath);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HASH_PRINT_H */
