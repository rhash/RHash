/* blake2s.h */
#ifndef BLAKE2S_H
#define BLAKE2S_H
#include "ustd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define blake2s_block_size 64
#define blake2s_hash_size  32

typedef struct blake2s_ctx
{
	uint32_t hash[8];
	uint32_t message[16];
	uint64_t length;
} blake2s_ctx;

void rhash_blake2s_init(blake2s_ctx* ctx);
void rhash_blake2s_update(blake2s_ctx* ctx, const unsigned char* msg, size_t size);
void rhash_blake2s_final(blake2s_ctx* ctx, unsigned char* result);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* BLAKE2S_H */
