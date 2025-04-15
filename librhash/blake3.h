/* blake3.h */
#ifndef RHASH_BLAKE3_H
#define RHASH_BLAKE3_H
#include "ustd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define blake3_block_size 64
#define blake3_hash_size  32

typedef struct blake3_ctx {
	uint32_t message[16];    /* current input bytes */
	uint64_t length;         /* processed data length */
	uint32_t stack_depth;
	uint32_t final_flags;
	union {
		uint32_t stack[54 * 8];  /* chain value stack */
		struct {
			uint32_t root_message[16];
			uint32_t hash[16];
		} root;
	};
} blake3_ctx;

void rhash_blake3_init(blake3_ctx *);
void rhash_blake3_update(blake3_ctx *, const unsigned char* msg, size_t);
void rhash_blake3_final(blake3_ctx *ctx, unsigned char* result);

#if !defined(NO_IMPORT_EXPORT)
size_t rhash_blake3_export(const blake3_ctx* ctx, void* out, size_t size);
size_t rhash_blake3_import(blake3_ctx* ctx, const void* in, size_t size);
#endif /* !defined(NO_IMPORT_EXPORT) */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_BLAKE3_H */
