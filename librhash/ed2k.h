/* ed2k.h */
#ifndef ED2K_H
#define ED2K_H
#include "md4.h"

#ifdef __cplusplus
extern "C" {
#endif

/* algorithm context */
typedef struct ed2k_ctx
{
	md4_ctx md4_context_inner; /* md4 context to hash file blocks */
	md4_ctx md4_context;       /* md4 context to hash block hashes */
	int not_emule;             /* flag: 0 for emule ed2k algorithm */
	int hashset_collect;       /* flag: 1 if chunk hashes shall be saved, -1 on error */
	unsigned char* hashset;    /* buffer to store chunk hashes */
	size_t hashset_size;       /* number of stored chunk hashes */
	size_t hashset_capacity;   /* number of allocated chunk hashes */
} ed2k_ctx;

/* hash functions */

void rhash_ed2k_init(ed2k_ctx* ctx);
void rhash_ed2k_update(ed2k_ctx* ctx, const unsigned char* msg, size_t size);
void rhash_ed2k_final(ed2k_ctx* ctx, unsigned char result[16]);

/* Clean up context by freeing allocated memory.
 * Shall be called when aborting hash calculations. */
void rhash_ed2k_cleanup(ed2k_ctx* ctx);

int rhash_ed2k_hashset_realloc(ed2k_ctx* ctx, size_t chunk_count);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ED2K_H */
