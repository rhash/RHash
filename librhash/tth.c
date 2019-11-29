/* tth.c - calculate TTH (Tiger Tree Hash) function.
 *
 * Copyright (c) 2007, Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include "byte_order.h"
#include "tth.h"

/**
 * Initialize context before calculaing hash.
 *
 * @param ctx context to initialize
 */
void rhash_tth_init(tth_ctx* ctx)
{
	rhash_tiger_init(&ctx->tiger);
	ctx->tiger.message[ ctx->tiger.length++ ] = 0x00;
	ctx->block_count = 0;
}

/**
 * The core transformation.
 *
 * @param ctx algorithm state
 */
static void rhash_tth_process_block(tth_ctx* ctx)
{
	uint64_t it;
	unsigned pos = 0;
	unsigned char msg[24];

	for (it = 1; it & ctx->block_count; it <<= 1) {
		rhash_tiger_final(&ctx->tiger, msg);
		rhash_tiger_init(&ctx->tiger);
		ctx->tiger.message[ctx->tiger.length++] = 0x01;
		rhash_tiger_update(&ctx->tiger, (unsigned char*)(ctx->stack + pos), 24);
		/* note: we can cut this step, if the previous rhash_tiger_final saves directly to ctx->tiger.message+25; */
		rhash_tiger_update(&ctx->tiger, msg, 24);
		pos += 3;
	}
	rhash_tiger_final(&ctx->tiger, (unsigned char*)(ctx->stack + pos));
	ctx->block_count++;
}

/**
 * Calculate message hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param msg message chunk
 * @param size length of the message chunk
 */
void rhash_tth_update(tth_ctx* ctx, const unsigned char* msg, size_t size)
{
	size_t rest = 1025 - (size_t)ctx->tiger.length;
	for (;;) {
		if (size < rest) rest = size;
		rhash_tiger_update(&ctx->tiger, msg, rest);
		msg += rest;
		size -= rest;
		if (ctx->tiger.length < 1025) {
			return;
		}

		/* process block hash */
		rhash_tth_process_block(ctx);

		/* init block hash */
		rhash_tiger_init(&ctx->tiger);
		ctx->tiger.message[ ctx->tiger.length++ ] = 0x00;
		rest = 1024;
	}
}

/**
 * Store calculated hash into the given array.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param result calculated hash in binary form
 */
void rhash_tth_final(tth_ctx* ctx, unsigned char result[24])
{
	uint64_t it = 1;
	unsigned pos = 0;
	unsigned char msg[24];
	const unsigned char* last_message;

	/* process the bytes left in the context buffer */
	if (ctx->tiger.length > 1 || ctx->block_count == 0) {
		rhash_tth_process_block(ctx);
	}

	for (; it < ctx->block_count && (it & ctx->block_count) == 0; it <<= 1) pos += 3;
	last_message = (unsigned char*)(ctx->stack + pos);

	for (it <<= 1; it <= ctx->block_count; it <<= 1) {
		/* merge TTH sums in the tree */
		pos += 3;
		if (it & ctx->block_count) {
			rhash_tiger_init(&ctx->tiger);
			ctx->tiger.message[ ctx->tiger.length++ ] = 0x01;
			rhash_tiger_update(&ctx->tiger, (unsigned char*)(ctx->stack + pos), 24);
			rhash_tiger_update(&ctx->tiger, last_message, 24);

			rhash_tiger_final(&ctx->tiger, msg);
			last_message = msg;
		}
	}

	/* save result hash */
	memcpy(ctx->tiger.hash, last_message, tiger_hash_length);
	if (result) memcpy(result, last_message, tiger_hash_length);
}
