/* torrent.c - create BitTorrent files and calculate BitTorrent  InfoHash (BTIH).
 *
 * Copyright: 2010 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>  /* time() */

#include "byte_order.h"
#include "algorithms.h"
#include "torrent.h"

#ifdef USE_OPENSSL
#define SHA1_INIT(ctx) ((pinit_t)ctx->sha_init)(&ctx->sha1_context)
#define SHA1_UPDATE(ctx, msg, size) ((pupdate_t)ctx->sha_update)(&ctx->sha1_context, (msg), (size))
#define SHA1_FINAL(ctx, result) ((pfinal_t)ctx->sha_final)(&ctx->sha1_context, (result))
#else
#define SHA1_INIT(ctx) rhash_sha1_init(&ctx->sha1_context)
#define SHA1_UPDATE(ctx, msg, size) rhash_sha1_update(&ctx->sha1_context, (msg), (size))
#define SHA1_FINAL(ctx, result) rhash_sha1_final(&ctx->sha1_context, (result))
#endif

/** size of a SHA1 hash in bytes */
#define BT_HASH_SIZE 20
/** number of SHA1 hashes to store together in one block */
#define BT_BLOCK_SIZE 256

/**
 * Initialize torrent context before calculating hash.
 *
 * @param ctx context to initialize
 */
void rhash_torrent_init(torrent_ctx* ctx)
{
	memset(ctx, 0, sizeof(torrent_ctx));
	ctx->piece_length = 65536;

#ifdef USE_OPENSSL
	{
		/* get the methods of the selected SHA1 algorithm */
		rhash_hash_info *sha1_info = &rhash_info_table[3];
		assert(sha1_info->info.hash_id == RHASH_SHA1);
		assert(sha1_info->context_size <= (sizeof(sha1_ctx) + sizeof(unsigned long)));
		ctx->sha_init = sha1_info->init;
		ctx->sha_update = sha1_info->update;
		ctx->sha_final = sha1_info->final;
	}
#endif

	SHA1_INIT(ctx);
}

/**
 * Clean up torrent context by freeing all dynamically
 * allocated memory.
 *
 * @param ctx torrent algorithm context
 */
void rhash_torrent_cleanup(torrent_ctx *ctx)
{
	size_t i;
	assert(ctx != NULL);

	/* destroy array of hash blocks */
	for(i = 0; i < ctx->hash_blocks.size; i++) {
		free(ctx->hash_blocks.array[i]);
	}

	/* destroy array of file paths */
	for(i = 0; i < ctx->files.size; i++) {
		free(ctx->files.array[i]);
	}

	free(ctx->program_name);
	free(ctx->announce);
	ctx->announce = ctx->program_name = 0;
	free(ctx->torrent_str);
}

static void rhash_make_torrent(torrent_ctx *ctx);

/**
 * Add an item to vector.
 *
 * @param vect vector to add item to
 * @param item the item to add
 * @return non-zero on success, zero on fail
 */
static int bt_vector_add_ptr(torrent_vect* vect, void* item)
{
	/* check if vect contains enough space for next item */
	if(vect->size >= vect->allocated) {
		size_t size = (vect->allocated == 0 ? 128 : vect->allocated * 2);
		void *new_array = realloc(vect->array, size * sizeof(void*));
		if(new_array == NULL) return 0; /* failed: no memory */
		vect->array = (void**)new_array;
		vect->allocated = size;
	}
	/* add new item to the vector */
	vect->array[vect->size] = item;
	vect->size++;
	return 1;
}

/**
 * Store a SHA1 hash of a processed file piece.
 *
 * @param ctx torrent algorithm context
 * @return non-zero on success, zero on fail
 */
static int bt_store_piece_sha1(torrent_ctx *ctx)
{
	unsigned char* block;
	unsigned char* hash;

	if((ctx->piece_count % BT_BLOCK_SIZE) == 0) {
		block = (unsigned char*)malloc(BT_HASH_SIZE * BT_BLOCK_SIZE);
		if(block == NULL || !bt_vector_add_ptr(&ctx->hash_blocks, block)) {
			if(block) free(block);
			return 0;
		}
	} else {
		block = (unsigned char*)(ctx->hash_blocks.array[ctx->piece_count / BT_BLOCK_SIZE]);
	}

	hash = &block[BT_HASH_SIZE * (ctx->piece_count % BT_BLOCK_SIZE)];
	SHA1_FINAL(ctx, hash); /* write the hash */
	ctx->piece_count++;
	return 1;
}

/**
 * A filepath and filesize information.
 */
typedef struct file_n_size_info
{
	uint64_t size;
	char path[1];
} file_n_size_info;

/**
 * Add a file info into the batch of files of given torrent.
 *
 * @param ctx torrent algorithm context
 * @param path file path
 * @param filesize file size
 * @return non-zero on success, zero on fail
 */
int rhash_torrent_add_file(torrent_ctx *ctx, const char* path, uint64_t filesize)
{
	size_t len = strlen(path);
	file_n_size_info* info = (file_n_size_info*)malloc(sizeof(uint64_t) + len + 1);
	if(info == NULL) {
		ctx->error = 1;
		return 0;
	}

	info->size = filesize;
	memcpy(info->path, path, len + 1);
	if(!bt_vector_add_ptr(&ctx->files, info)) return 0;

	/* recalculate piece length (but only if hashing not started yet) */
	if(ctx->piece_count == 0 && ctx->index == 0) {
		/* note: in case of batch of files should use a total batch size */
		ctx->piece_length = rhash_torrent_default_piece_length(filesize);
	}
	return 1;
}

/**
 * Calculate message hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param ctx the algorithm context containing current hashing state
 * @param msg message chunk
 * @param size length of the message chunk
 */
void rhash_torrent_update(torrent_ctx *ctx, const void* msg, size_t size)
{
	const unsigned char* pmsg = (const unsigned char*)msg;
	size_t rest = (size_t)(ctx->piece_length - ctx->index);
	assert(ctx->index < ctx->piece_length);

	while(size > 0) {
		size_t left = (size < rest ? size : rest);
		SHA1_UPDATE(ctx, pmsg, left);
		if(size < rest) {
			ctx->index += left;
			break;
		}
		bt_store_piece_sha1(ctx);
		SHA1_INIT(ctx);
		ctx->index = 0;

		pmsg += rest;
		size -= rest;
		rest = ctx->piece_length;
	}
}

/**
 * Finalize hashing and optionally store calculated hash into the given array.
 * If the result parameter is NULL, the hash is not stored, but it is
 * accessible by rhash_torrent_get_btih().
 *
 * @param ctx the algorithm context containing current hashing state
 * @param result pointer to the array store message hash into
 */
void rhash_torrent_final(torrent_ctx *ctx, unsigned char result[20])
{
	if(ctx->index > 0) {
		bt_store_piece_sha1(ctx); /* flush buffered data */
	}

	rhash_make_torrent(ctx);
	if(result) memcpy(result, ctx->btih, btih_hash_size);
}

/* BitTorrent functions */

/**
 * Grow, if needed, the torrent_str buffer to ensure it contains
 * at least (length + 1) characters.
 *
 * @param ctx the torrent algorithm context
 * @param length length of the string, the allocated buffer must contain
 * @return 1 on success, 0 on error
 */
static int bt_str_ensure_length(torrent_ctx* ctx, size_t length)
{
	char* new_str;
	if(length >= ctx->torrent_allocated && !ctx->error) {
		length++; /* allocate one character more */
		if(length < 64) length = 64;
		else length = (length + 255) & ~255;
		new_str = (char*)realloc(ctx->torrent_str, length);
		if(new_str == NULL) {
			ctx->error = 1;
			ctx->torrent_allocated = 0;
			return 0;
		}
		ctx->torrent_str = new_str;
		ctx->torrent_allocated = length;
	}
	return 1;
}

/**
 * Print 64-bit number with trailing '\0' to a string buffer.
 *
 * @param dst output buffer
 * @param number the number to print
 * @return length of the printed number (without trailing '\0')
 */
static int rhash_sprintI64(char *dst, uint64_t number)
{
	/* The biggest number has 20 digits: 2^64 = 18 446 744 073 709 551 616 */
	char buf[24];
	size_t len;
	char *p = buf + 23;
	*p = '\0'; /* last symbol should be '\0' */
	if(number == 0) {
		*(--p) = '0';
	} else {
		for(; p >= buf && number != 0; number /= 10) {
			*(--p) = '0' + (char)(number % 10);
		}
	}
	len = buf + 23 - p;
	memcpy(dst, p, len + 1);
	return (int)len;
}

/**
 * B-encode given integer.
 *
 * @param ctx the torrent algorithm context
 * @param number the integer to output
 */
static void bt_bencode_int(torrent_ctx* ctx, uint64_t number)
{
	char* p;
	/* add up to 20 digits and 2 letters */
	if(!bt_str_ensure_length(ctx, ctx->torrent_length + 22)) return;
	p = ctx->torrent_str + ctx->torrent_length;
	*(p++) = 'i';
	p += rhash_sprintI64(p, number);
	*(p++) = 'e';
	*p = '\0'; /* terminate string with \0 */

	ctx->torrent_length = (p - ctx->torrent_str);
}

/**
 * B-encode a string.
 *
 * @param ctx the torrent algorithm context
 * @param str the string to encode
 */
static void bt_bencode_str(torrent_ctx* ctx, const char* str)
{
	size_t len = strlen(str);
	int num_len;
	char* p;
	if(!bt_str_ensure_length(ctx, ctx->torrent_length + len + 21)) return;

	p = ctx->torrent_str + ctx->torrent_length;
	p += (num_len = rhash_sprintI64(p, len));
	ctx->torrent_length += len + num_len + 1;

	*(p++) = ':';
	memcpy(p, str, len + 1); /* copy with trailing '\0' */
}

/**
 * B-encode array of SHA1 hashes of file pieces.
 *
 * @param ctx pointer to the torrent structure containing SHA1 hashes
 */
static void bt_bencode_pieces(torrent_ctx* ctx)
{
	int pieces_length = ctx->piece_count * BT_HASH_SIZE;
	int num_len;
	int size, i;
	char* p;

	if(!bt_str_ensure_length(ctx, ctx->torrent_length + pieces_length + 21))
		return;

	p = ctx->torrent_str + ctx->torrent_length;
	p += (num_len = rhash_sprintI64(p, pieces_length));
	ctx->torrent_length += pieces_length + num_len + 1;

	*(p++) = ':';
	p[pieces_length] = '\0'; /* terminate with \0 just in case */

	for(size = ctx->piece_count, i = 0; size > 0;
		size -= BT_BLOCK_SIZE, i++)
	{
		memcpy(p, ctx->hash_blocks.array[i],
			(size < BT_BLOCK_SIZE ? size : BT_BLOCK_SIZE) * BT_HASH_SIZE);
		p += BT_BLOCK_SIZE * BT_HASH_SIZE;
	}
}

/**
 * Append a null-terminated string to the string string buffer.
 *
 * @param ctx the torrent algorithm context
 * @param text the null-terminated string to append
 */
static void bt_str_append(torrent_ctx *ctx, const char* text)
{
	size_t length = strlen(text);

	if(!bt_str_ensure_length(ctx, ctx->torrent_length + length)) return;
	memcpy(ctx->torrent_str + ctx->torrent_length, text, length);
	ctx->torrent_length += length;
	ctx->torrent_str[ctx->torrent_length] = '\0';
}

/**
 * Calculate default torrent piece length, using uTorrent algorithm.
 * Algorithm:
 *  length = 64K for total_size < 64M,
 *  length = 4M for total_size >= 2G,
 *  length = top_bit(total_size) / 512 otherwise.
 *
 * @param total_size total hashed batch size of torrent file
 * @return piece length used by torrent file
 */
size_t rhash_torrent_default_piece_length(uint64_t total_size)
{
	uint64_t hi_bit;
	if(total_size < 67108864) return 65536;
	if(total_size >= I64(2147483648) ) return 4194304;
	for(hi_bit = 67108864 << 1; hi_bit <= total_size; hi_bit <<= 1);
	return (size_t)(hi_bit >> 10);
}

/**
 * Generate torrent file content
 * @see http://wiki.theory.org/BitTorrentSpecification
 *
 * @param ctx the torrent algorithm context
 */
static void rhash_make_torrent(torrent_ctx *ctx)
{
	uint64_t total_size = 0;
	size_t info_start_pos;

	assert(ctx->torrent_str == NULL);
	assert(ctx->files.size <= 1);

	if(ctx->piece_length == 0) {
		if(ctx->files.size == 1) {
			total_size = ((file_n_size_info*)ctx->files.array[0])->size;
		}
		ctx->piece_length = rhash_torrent_default_piece_length(total_size);
	}

	/* write torrent header to the ctx->torrent string bufer */
	if((ctx->options & BT_OPT_INFOHASH_ONLY) == 0) {
		bt_str_append(ctx, "d");
		if(ctx->announce) {
			bt_str_append(ctx, "8:announce");
			bt_bencode_str(ctx, ctx->announce);
		}

		if(ctx->program_name) {
			bt_str_append(ctx, "10:created by");
			bt_bencode_str(ctx, ctx->program_name);
		}

		bt_str_append(ctx, "13:creation date");
		bt_bencode_int(ctx, (uint64_t)time(NULL));
	}

	bt_str_append(ctx, "8:encoding5:UTF-8");

	bt_str_append(ctx, "4:infod"); /* start info dictionary */
	info_start_pos = ctx->torrent_length - 1;

	if(ctx->files.size == 1) {
		file_n_size_info* f = (file_n_size_info*)ctx->files.array[0];
		bt_str_append(ctx, "6:length");
		bt_bencode_int(ctx, f->size);

		/* note: for one file f->path must be a basename */
		bt_str_append(ctx, "4:name");
		bt_bencode_str(ctx, f->path);
	}
	bt_str_append(ctx, "12:piece length");
	bt_bencode_int(ctx, ctx->piece_length);

	bt_str_append(ctx, "6:pieces");
	bt_bencode_pieces(ctx);

	if(ctx->options & BT_OPT_PRIVATE) {
		bt_str_append(ctx, "7:privatei1e");
	}
	bt_str_append(ctx, "ee");

	/* calculate BTIH */
	SHA1_INIT(ctx);
	SHA1_UPDATE(ctx, (unsigned char*)ctx->torrent_str + info_start_pos,
		ctx->torrent_length - info_start_pos - 1);
	SHA1_FINAL(ctx, ctx->btih);
}

/* Getters/Setters */

/**
 * Get BTIH (BitTorrent Info Hash) value.
 *
 * @param ctx the torrent algorithm context
 * @return the 20-bytes long BTIH value
 */
unsigned char* rhash_torrent_get_btih(torrent_ctx *ctx)
{
	return ctx->btih;
}

/**
 * Set the torrent algorithm options.
 *
 * @param ctx the torrent algorithm context
 * @param options the options to set
 */
void rhash_torrent_set_options(torrent_ctx *ctx, unsigned options)
{
	ctx->options = options;
}

#if defined(__STRICT_ANSI__)
/* define strdup for gcc -ansi */
static char* bt_strdup(const char* str)
{
	size_t len = strlen(str);
	char* res = (char*)malloc(len + 1);
	if(res) memcpy(res, str, len + 1);
	return res;
}
#define strdup bt_strdup
#endif /* __STRICT_ANSI__ */

/**
 * Set optional name of the program generating the torrent
 * for storing into torrent file.
 *
 * @param ctx the torrent algorithm context
 * @param name the program name
 * @return non-zero on success, zero on error
 */
int rhash_torrent_set_program_name(torrent_ctx *ctx, const char* name)
{
	ctx->program_name = strdup(name);
	return (ctx->program_name != NULL);
}

/**
 * Set length of a file piece.
 *
 * @param ctx the torrent algorithm context
 * @param piece_length the piece length in bytes
 */
void rhash_torrent_set_piece_length(torrent_ctx *ctx, size_t piece_length)
{
	ctx->piece_length = piece_length;
}

/**
 * Set torrent announcement-URL for storing into torrent file.
 *
 * @param ctx the torrent algorithm context
 * @param announce_url the announcement-URL
 * @return non-zero on success, zero on error
 */
int rhash_torrent_set_announce(torrent_ctx *ctx, const char* announce_url)
{
	free(ctx->announce);
	ctx->announce = strdup(announce_url);
	return (ctx->announce != NULL);
}

/**
 * Get the content of generated torrent file.
 *
 * @param ctx the torrent algorithm context
 * @param pstr pointer to pointer receiving the buffer with file content
 * @return length of the torrent file content
 */
size_t rhash_torrent_get_text(torrent_ctx *ctx, char** pstr)
{
	assert(ctx->torrent_str);
	*pstr = ctx->torrent_str;
	return ctx->torrent_length;
}
