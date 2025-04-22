#include "blake3.h"
#include "byte_order.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

const size_t words_per_stack_entry = 8;
const size_t blake3_chunk_size = 1024;

/**
 * The initial value (IV) of BLAKE3 is the same as SHA-256 IV.
 * This IV is set to the initial chaining value of BLAKE3,
 * when no key is used. The first four words IV[0..3] are also
 * used by the compression function as the local initial state.
 */
static const uint32_t blake3_IV[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

/**
 * Initialize context before calculating hash.
 *
 * @param ctx context to initialize
 */
void rhash_blake3_init(struct blake3_ctx *ctx)
{
#if defined(NO_IMPORT_EXPORT)
	ctx->length = 0;
	ctx->stack_depth = 0;
	ctx->final_flags = 0;
#else
	const size_t zero_size = offsetof(blake3_ctx, root) + sizeof(ctx->root);
	memset(ctx, 0, zero_size);
#endif
	memcpy(ctx->stack, blake3_IV, sizeof(blake3_IV));
}

/* Compression Function Flags */
#define CHUNK_START 0x01u
#define CHUNK_END   0x02u
#define PARENT      0x04u
#define ROOT        0x08u

/**
 * Array of 7 permutations applied during Rounds 0-6 of
 * the BLAKE3 compression function.
 *
 * Round 0 uses identity permutation and Round 1 uses the
 * following base permutation of the 16 indices (0 to 15):
 *     {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8}
 *
 * Subsequent permutations (Rounds 2-6) are derived by applying
 * the base permutation iteratively.
 */
static const uint8_t permutations[7][16] = {
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, /* Round 0 (identity) */
	{2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8}, /* Round 1 */
	{3, 4, 10, 12, 13, 2, 7, 14, 6, 5, 9, 0, 11, 15, 8, 1}, /* Round 2 */
	{10, 7, 12, 9, 14, 3, 13, 15, 4, 0, 11, 2, 5, 8, 1, 6}, /* Round 3 */
	{12, 13, 9, 11, 15, 10, 14, 8, 7, 2, 5, 3, 0, 1, 6, 4}, /* Round 4 */
	{9, 14, 11, 5, 8, 12, 15, 1, 13, 3, 0, 10, 2, 6, 4, 7}, /* Round 5 */
	{11, 15, 5, 0, 1, 9, 8, 6, 14, 10, 2, 12, 3, 4, 7, 13}, /* Round 6 */
};

/**
 * Quarter-Round Function G, which processes four 32-bit words
 * of the internal state `v[0..15]` at a time.
 * A full round of shuffling in BLAKE3 consists of several such
 * `G` operations applied to different parts of the state.
 *
 * The `G` function mixes two input words from the message array `msg[]`
 * into four words of the working array `v[0..15]`. The parameters
 * `round` and `i` are used to index the appropriate words from `msg[]`.
 *
 * @param round the current round index.
 * @param i the index used to select words from `msg[]`.
 * @param a, b, c, d the four words from `v[]` to be processed.
 */
#define G(round, i, a, b, c, d) \
	a = a + b + msg[permutations[round][i * 2]]; \
	d = ROTR32(d ^ a, 16); \
	c = c + d; \
	b = ROTR32(b ^ c, 12); \
	a = a + b + msg[permutations[round][i * 2 + 1]]; \
	d = ROTR32(d ^ a, 8); \
	c = c + d; \
	b = ROTR32(b ^ c, 7);

#define ROUND(round) \
	G(round, 0, v[0], v[4], v[8],  v[12]); \
	G(round, 1, v[1], v[5], v[9],  v[13]); \
	G(round, 2, v[2], v[6], v[10], v[14]); \
	G(round, 3, v[3], v[7], v[11], v[15]); \
	G(round, 4, v[0], v[5], v[10], v[15]); \
	G(round, 5, v[1], v[6], v[11], v[12]); \
	G(round, 6, v[2], v[7], v[8],  v[13]); \
	G(round, 7, v[3], v[4], v[9],  v[14]);

/**
 * BLAKE3 compression function is used to process chunks,
 * compute parent nodes within its tree structure,
 * and generate output bytes from the root node.
 *
 * @param output compression result
 * @param msg message block to be processed
 * @param hash hash chaining value
 * @param counter 64-bit counter
 * @param length the number of application data bytes in the msg block,
 *        must be at least 1 and at most 64. It is equal to
 *        64 minus the number of padding bytes, which are set to 0x00.
 * @param flags bit flags used to compress the current block
 */
static void compress(uint32_t *output,
		const uint32_t msg[16], const uint32_t hash[8],
		uint64_t counter, uint32_t length, uint32_t flags)
{
	uint32_t v[16] = {
		hash[0], hash[1], hash[2], hash[3],
		hash[4], hash[5], hash[6], hash[7],
		blake3_IV[0], blake3_IV[1], blake3_IV[2], blake3_IV[3],
		(uint32_t)counter, (uint32_t)(counter >> 32), length, flags,
	};

	ROUND(0);
	ROUND(1);
	ROUND(2);
	ROUND(3);
	ROUND(4);
	ROUND(5);
	ROUND(6);

	if (flags & ROOT) {
		output[8]  = v[8]  ^ hash[0];
		output[9]  = v[9]  ^ hash[1];
		output[10] = v[10] ^ hash[2];
		output[11] = v[11] ^ hash[3];
		output[12] = v[12] ^ hash[4];
		output[13] = v[13] ^ hash[5];
		output[14] = v[14] ^ hash[6];
		output[15] = v[15] ^ hash[7];
	}
	output[0] = v[0] ^ v[8];
	output[1] = v[1] ^ v[9];
	output[2] = v[2] ^ v[10];
	output[3] = v[3] ^ v[11];
	output[4] = v[4] ^ v[12];
	output[5] = v[5] ^ v[13];
	output[6] = v[6] ^ v[14];
	output[7] = v[7] ^ v[15];
}

/* Process a block of 64 bytes */
static void process_block(struct blake3_ctx *ctx, const uint32_t msg[16])
{
	uint32_t *cur_hash = ctx->stack + ctx->stack_depth * words_per_stack_entry;
	uint64_t tail_index = ctx->length - 1;
	uint64_t chunk_index = tail_index >> 10;
	uint32_t block_index_bits = (uint32_t)tail_index & 0x3c0;
	uint32_t flags = 0;
	switch (block_index_bits) {
	case 0:
		flags = CHUNK_START; /* the first block of a chunk */
		break;
	case 15 << 6:
		flags = CHUNK_END; /* the last block of a chunk */
		break;
	}
	compress(cur_hash, msg, cur_hash, chunk_index, blake3_block_size, flags);

	if ((flags & CHUNK_END) != 0) {
		/* process Merkle tree */
		uint64_t path = chunk_index + 1;
		for (; (path & 1) == 0; path >>= 1) {
			cur_hash -= words_per_stack_entry;
			compress(cur_hash, cur_hash, blake3_IV, 0, blake3_block_size, PARENT);
		}
		cur_hash += words_per_stack_entry;
		memcpy(cur_hash, blake3_IV, sizeof(blake3_IV));
	}
	ctx->stack_depth = (uint32_t)((cur_hash - ctx->stack) / words_per_stack_entry);
}

/**
 * Calculate message hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param msg message chunk
 * @param size length of the message chunk
 */
void rhash_blake3_update(blake3_ctx *ctx, const unsigned char* msg, size_t size)
{
	unsigned index = ctx->length ? (((unsigned)ctx->length - 1) & 63) + 1 : 0;

	/* fill partial block */
	if (index) {
		size_t left = blake3_block_size - index;
		if (size < left)
			left = size;
		le32_copy(ctx->message, index, msg, left);
		ctx->length += left;
		size -= left;
		if (!size)
			return;
		/* process partial block */
		process_block(ctx, ctx->message);
		msg += left;
	}
	while (size > blake3_block_size) {
		uint32_t* aligned_message_block;
		if (IS_LITTLE_ENDIAN && IS_ALIGNED_32(msg)) {
			/* the most common case is processing a 32-bit aligned message
			on a little-endian CPU without copying it */
			aligned_message_block = (uint32_t*)msg;
		} else {
			le32_copy(ctx->message, 0, msg, blake3_block_size);
			aligned_message_block = ctx->message;
		}

		ctx->length += blake3_block_size;
		process_block(ctx, aligned_message_block);
		msg += blake3_block_size;
		size -= blake3_block_size;
	}
	if (size) {
		/* save leftovers */
		le32_copy(ctx->message, 0, msg, size);
		ctx->length += size;
	}
}

/**
 * Process the last block.
 *
 * @param ctx the algorithm context containing current hashing state
 */
static void process_last_block(blake3_ctx *ctx)
{
	uint64_t tail_index = ctx->length ? ctx->length - 1 : 0;
	uint32_t index = ctx->length ? ((uint32_t)tail_index & 63) + 1 : 0;
	uint32_t flags = tail_index & 0x3c0 ? CHUNK_END : CHUNK_START | CHUNK_END;
	uint32_t *message;
	uint32_t *cur_hash = ctx->stack + ctx->stack_depth * words_per_stack_entry;

	/* pad the last block with zeros */
	le32_memset(ctx->message, index, 0, blake3_block_size - index);

	if (ctx->stack_depth == 0) {
		/* use the padded message to get root hash */
		message = ctx->message;
	} else {
		/* compress the last chunk */
		uint64_t chunk_index = tail_index >> 10;
		compress(cur_hash, ctx->message, cur_hash, chunk_index, index, flags);

		/* calculate the top parent hash of the tree */
		flags = PARENT;
		while ((cur_hash -= words_per_stack_entry) != ctx->stack)
			compress(cur_hash, cur_hash, blake3_IV, 0, blake3_block_size, flags);
		index = blake3_block_size;
		ctx->stack_depth = 0;
		message = ctx->root.root_message;
		cur_hash = (uint32_t *)blake3_IV;
	}
	flags |= ROOT;
	ctx->final_flags = flags;
	compress(ctx->root.hash, message, cur_hash, 0, index, flags);
}

/**
 * Store calculated 256-bit hash into the given array.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param result buffer to receive the calculated hash or NULL
 */
void rhash_blake3_final(blake3_ctx *ctx, unsigned char* result)
{
	if (!ctx->final_flags)
		process_last_block(ctx);
	if (result)
		le32_copy(result, 0, ctx->root.hash, blake3_hash_size);
}

#if !defined(NO_IMPORT_EXPORT)
/**
 * Load a 32-bit unsigned integer from memory in memory order.
 * Performs no bounds checking - caller must ensure memory is accessible.
 *
 * @param data pointer to the memory region containing the 32-bit value.
 *             Must have at least 4 bytes of readable memory.
 * @return the 32-bit value constructed from the memory bytes according
 *         to system endianness.
 */
static uint32_t load_uint32(const uint8_t *data)
{
#if IS_LITTLE_ENDIAN
	return (uint32_t)data[0] | (uint32_t)data[1] << 8 |
		(uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
#else
	return (uint32_t)data[3] | (uint32_t)data[2] << 8 |
		(uint32_t)data[1] << 16 | (uint32_t)data[0] << 24;
#endif
}

/**
 * Calculate the minimum stack size in bytes based on the stack depth,
 * ensuring the stack contains blake3_ctx.root structure.
 * The size calculation accounts for:
 * - Each stack entry being 8 uint32_t values (32 bytes)
 * - A minimum stack size of 32 uint32_t values (128 bytes)
 *
 * @param stack_depth the desired depth of the stack (number of entries)
 * @return size_t the calculated stack size in bytes
 */
static size_t get_stack_size(uint32_t stack_depth)
{
	const size_t bytes_per_entry = sizeof(uint32_t) * words_per_stack_entry;
	const size_t min_stack_bytes = sizeof(uint32_t) * 32;
	size_t size = stack_depth * bytes_per_entry;
	return size < min_stack_bytes ? min_stack_bytes : size;
}

/**
 * Export tth context to a memory region, or calculate the
 * size required for context export.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param out pointer to the memory region or NULL
 * @param size size of memory region
 * @return the size of the exported data on success, 0 on fail.
 */
size_t rhash_blake3_export(const blake3_ctx* ctx, void* out, size_t size)
{
	size_t export_size = offsetof(blake3_ctx, stack) + get_stack_size(ctx->stack_depth);
	if (out != NULL) {
		if (size < export_size)
			return 0;
		memcpy(out, ctx, export_size);
	}
	return export_size;
}

/**
 * Import tth context from a memory region.
 *
 * @param ctx pointer to the algorithm context
 * @param in pointer to the data to import
 * @param size size of data to import
 * @return the size of the imported data on success, 0 on fail.
 */
size_t rhash_blake3_import(blake3_ctx* ctx, const void* in, size_t size)
{
	uint32_t stack_depth = load_uint32(
		(const uint8_t*)in + offsetof(blake3_ctx, stack_depth));
	size_t import_size =
		offsetof(blake3_ctx, stack) + get_stack_size(stack_depth);
	if (size < import_size)
		return 0;
	memcpy(ctx, in, import_size);
	assert(ctx->stack_depth == stack_depth);
	return import_size;
}
#endif /* !defined(NO_IMPORT_EXPORT) */
