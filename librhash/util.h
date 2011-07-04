/* util.h */
#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* clever malloc with error detection */
#define rsh_malloc(size) rhash_malloc(size, __FILE__, __LINE__)
#define rsh_strdup(str)  rhash_strdup(str,  __FILE__, __LINE__)
#define rsh_realloc(mem, size) rhash_realloc(mem, size, __FILE__, __LINE__)
void* rhash_malloc(size_t size, const char* srcfile, int srcline);    
char* rhash_strdup(const char* str, const char* srcfile, int srcline);
void* rhash_realloc(void* mem, size_t size, const char* srcfile, int srcline);

extern void (*rsh_exit)(int code);
extern void (*rsh_report_error)(const char* srcfile, int srcline, const char* format, ...);

/* vector functions */
typedef struct vector_t
{
	void **array;
	size_t size;
	size_t allocated;
	void (*destructor)(void*);
} vector_t;

struct vector_t* rsh_vector_new(void (*destructor)(void*));
struct vector_t* rsh_vector_new_simple(void);
void rsh_vector_free(struct vector_t* vect);
void rsh_vector_destroy(struct vector_t* vect);
void rsh_vector_add_ptr(struct vector_t* vect, void *item);
/*void rsh_vector_sort(struct vector_t* vect, int (*compare)(const void *rec1, const void *rec2));*/
void rsh_vector_item_add_empty(struct vector_t* vect, size_t item_size);
#define rsh_vector_add_uint32(vect, item) { \
	rsh_vector_item_add_empty(vect, item_size); \
	((unsigned*)(vect)->array)[(vect)->size - 1] = item; \
}
#define rsh_vector_add_item(vect, item, item_size) { \
	rsh_vector_item_add_empty(vect, item_size); \
	memcpy(((char*)(vect)->array) + item_size * ((vect)->size - 1), item, item_size); \
}

/* a vector pattern implementation, allocating elements by blocks */
typedef struct blocks_vector_t
{
	size_t size;
	vector_t blocks;
} blocks_vector_t;

void rsh_blocks_vector_init(struct blocks_vector_t*);
/* #define rsh_blocks_vector_init(bvector) memset(bvector, 0, sizeof(*bvector)); */
void rsh_blocks_vector_destroy(struct blocks_vector_t* vect);
#define rsh_blocks_vector_get_item(bvector, index, blocksize, item_type) \
	(&((item_type*)((bvector)->blocks.array[(index) / (blocksize)]))[(index) % (blocksize)])
#define rsh_blocks_vector_get_ptr(bvector, index, blocksize, item_size) \
	(&((unsigned char*)((bvector)->blocks.array[(index) / (blocksize)]))[(item_size) * ((index) % (blocksize))])
#define rsh_blocks_vector_add(bvector, item, blocksize, item_size) { \
	if(((bvector)->size % (blocksize)) == 0) \
		rsh_vector_add_ptr(&((bvector)->blocks), rsh_malloc((item_size) * (blocksize))); \
	memcpy(rsh_blocks_vector_get_ptr((bvector), (bvector)->size, (blocksize), (item_size)), (item), (item_size)); \
	(bvector)->size++; \
}
#define rsh_blocks_vector_add_ptr(bvector, ptr, blocksize) { \
	if(((bvector)->size % (blocksize)) == 0) \
		rsh_vector_add_ptr(&((bvector)->blocks), rsh_malloc(sizeof(void*) * (blocksize))); \
	((void***)(bvector)->blocks.array)[(bvector)->size / (blocksize)][(bvector)->size % (blocksize)] = (void*)ptr; \
	(bvector)->size++; \
}
#define rsh_blocks_vector_add_empty(bvector, blocksize, item_size) { \
	if( (((bvector)->size++) % (blocksize)) == 0) \
		rsh_vector_add_ptr(&((bvector)->blocks), rsh_malloc((item_size) * (blocksize))); \
}

/* string buffer functions */
typedef struct strbuf_t
{
	char* str;
	size_t allocated;
	size_t len;
} strbuf_t;

strbuf_t* rsh_str_new(void);
void rsh_str_free(strbuf_t* buf);
void rsh_str_ensure_size(strbuf_t *str, size_t new_size);
void rsh_str_append_n(strbuf_t *str, const char* text, size_t len);
void rsh_str_append(strbuf_t *str, const char* text);

#define rsh_str_ensure_length(str, len) \
	if((size_t)(len) >= (size_t)(str)->allocated) rsh_str_ensure_size((str), (len) + 1);
#define rsh_wstr_ensure_length(str, len) \
	if((size_t)((len) + 2) > (size_t)(str)->allocated) rsh_str_ensure_size((str), (len) + 2);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* UTIL_H */
