/* crc_printf.h - functions to print hash sums */
#ifndef CRC_PRINT_H
#define CRC_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct print_item {
	struct print_item *next;
	unsigned flags;
	unsigned hash_id;
	unsigned width;
	const char *data;
} print_item;

typedef struct print_hash_info
{
	char short_name[16];
	char short_char;
	const char *name;
	const char *urn;
} print_hash_info;

extern print_hash_info hash_info_table[];

struct file_info;

print_item* parse_print_string(const char* format, unsigned *sum_mask);
void print_line(FILE* out, print_item* list, struct file_info *info);
void free_print_list(print_item* list);

void init_hash_info_table(void);
void init_printf_format(struct strbuf_t* out);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CRC_PRINT_H */
