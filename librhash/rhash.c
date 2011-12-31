/* rhash.c - implementation of LibRHash library calls
 *
 * Copyright: 2008 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 */
#include <string.h> /* memset() */
#include <stdlib.h> /* free() */
#include <stddef.h> /* ptrdiff_t */
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/* modifier for Windows DLL */
#if defined(_WIN32) && defined(RHASH_EXPORTS)
# define RHASH_API __declspec(dllexport)
#endif

#include "byte_order.h"
#include "algorithms.h"
#include "torrent.h"
#include "plug_openssl.h"
#include "util.h"
#include "hex.h"
#include "rhash.h" /* RHash library interface */

#define STATE_ACTIVE  0xb01dbabe
#define STATE_STOPED  0xdeadbeef
#define STATE_DELETED 0xdecea5ed
#define RCTX_AUTO_FINAL 0x1
#define RCTX_FINALIZED  0x2
#define RCTX_FINALIZED_MASK (RCTX_AUTO_FINAL | RCTX_FINALIZED)

/**
 * Initialize static data of rhash algorithms
 */
void rhash_library_init(void)
{
	rhash_init_algorithms(RHASH_ALL_HASHES);
#ifdef USE_OPENSSL
	rhash_plug_openssl();
#endif
}

/**
 * Returns the number of supported hash algorithms.
 *
 * @return the number of supported hash functions
 */
int RHASH_API rhash_count(void)
{
	return rhash_info_size;
}

/**
 * Information on a hash function and its context
 */
typedef struct rhash_vector_item
{
	struct rhash_hash_info* hash_info;
	void *context;
} rhash_vector_item;

/**
 * The rhash context containing contexts for several hash functions
 */
typedef struct rhash_context_ext
{
	struct rhash_context rc;
	unsigned hash_vector_size; /* number of contained hash sums */
	unsigned flags;
	unsigned state;
	void *callback, *callback_data;
	void *bt_ctx;
	rhash_vector_item vector[1]; /* contexts of contained hash sums */
} rhash_context_ext;

/* Lo-level rhash library functions */

/**
 * Allocate and initialize RHash context for calculating hash(es).
 * After initializing rhash_update()/rhash_final() functions should be used.
 * Then the context must be freed by calling rhash_free().
 *
 * @param hash_id union of bit flags, containing ids of hashes to calculate.
 * @return initialized rhash context
 */
RHASH_API rhash rhash_init(unsigned hash_id)
{
	unsigned tail_bit_index; /* index of hash_id trailing bit */
	unsigned num = 0;        /* number of hashes to compute */
	rhash_context_ext *rctx = NULL; /* allocated rhash context */
	size_t hash_size_sum = 0;   /* size of hash contexts to store in rctx */

	unsigned i, bit_index, id;
	struct rhash_hash_info* info;
	size_t aligned_size;
	char* phash_ctx;

	hash_id &= RHASH_ALL_HASHES;
	if(hash_id == 0) return NULL;

	tail_bit_index = rhash_ctz(hash_id); /* get trailing bit index */
	assert(tail_bit_index < RHASH_HASH_COUNT);

	id = 1 << tail_bit_index;

	if(hash_id == id) {
		/* handle the most common case of only one hash */
		num = 1;
		info = &rhash_info_table[tail_bit_index];
		hash_size_sum = info->context_size;
	} else {
		/* another case: hash_id contains several hashes */
		for(bit_index = tail_bit_index; id <= hash_id; bit_index++, id = id << 1) {
			assert(id != 0);
			assert(bit_index < RHASH_HASH_COUNT);
			info = &rhash_info_table[bit_index];
			if(hash_id & id) {
				/* align sizes by 8 bytes */
				aligned_size = (info->context_size + 7) & ~7;
				hash_size_sum += aligned_size;
				num++;
			}
		}
		assert(num > 1);
	}

	/* align the size of the rhash context common part */
	aligned_size = (offsetof(rhash_context_ext, vector[num]) + 7) & ~7;
	assert(aligned_size >= sizeof(rhash_context_ext));

	/* allocate rhash context with enough memory to store contexts of all used hashes */
	rctx = (rhash_context_ext*)malloc(aligned_size + hash_size_sum);
	if(rctx == NULL) return NULL;

	/* initialize common fields of the rhash context */
	memset(rctx, 0, sizeof(rhash_context_ext));
	rctx->rc.hash_id = hash_id;
	rctx->flags = RCTX_AUTO_FINAL; /* turn on auto-final by default */
	rctx->state = STATE_ACTIVE;
	rctx->hash_vector_size = num;

	/* aligned hash contexts follows rctx->vector[num] in the same memory block */
	phash_ctx = (char*)rctx + aligned_size;
	assert(phash_ctx >= (char*)&rctx->vector[num]);

	/* initialize context for every hash in a loop */
	for(bit_index = tail_bit_index, id = 1 << tail_bit_index, i = 0;
		id <= hash_id; bit_index++, id = id << 1)
	{
		/* check if a hash function with given id shall be included into rctx */
		if((hash_id & id) != 0) {
			info = &rhash_info_table[bit_index];
			assert(info->context_size > 0);
			assert(((phash_ctx - (char*)0) & 7) == 0); /* hash context is aligned */
			assert(info->init != NULL);

			rctx->vector[i].hash_info = info;
			rctx->vector[i].context = phash_ctx;

			/* BTIH initialization is complex, save pointer for later */
			if((id & RHASH_BTIH) != 0) rctx->bt_ctx = phash_ctx;
			phash_ctx += (info->context_size + 7) & ~7;

			/* initialize the i-th hash context */
			info->init(rctx->vector[i].context);
			i++;
		}
	}

	return &rctx->rc; /* return allocated and initialized rhash context */
}

/**
 * Free RHash context memory.
 *
 * @param ctx the context to free.
 */
void rhash_free(rhash ctx)
{
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	unsigned i;
	
	if(ctx == 0) return;
	assert(ectx->hash_vector_size <= RHASH_HASH_COUNT);
	ectx->state = STATE_DELETED; /* mark memory block as being removed */

	/* clean the hash functions, which require additional clean up */
	for(i = 0; i < ectx->hash_vector_size; i++) {
		struct rhash_hash_info* info = ectx->vector[i].hash_info;
		if(info->cleanup != 0) {
			info->cleanup(ectx->vector[i].context);
		}
	}

	free(ectx);
}

/**
 * Re-initialize RHash context to reuse it.
 * Useful to speed up processing of many small messages.
 *
 * @param ctx context to reinitialize
 */
RHASH_API void rhash_reset(rhash ctx)
{
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	unsigned i;
	assert(ectx->hash_vector_size > 0 && ectx->hash_vector_size < RHASH_HASH_COUNT);
	ectx->state = STATE_ACTIVE; /* re-activate the structure */

	/* re-initialize every hash in a loop */
	for(i = 0; i < ectx->hash_vector_size; i++) {
		struct rhash_hash_info* info = ectx->vector[i].hash_info;
		if(info->cleanup != 0) {
			info->cleanup(ectx->vector[i].context);
		}

		assert(info->init != NULL);
		info->init(ectx->vector[i].context);
	}
	ectx->flags &= ~RCTX_FINALIZED; /* clear finalized state */
}

/**
 * Calculate hashes of message.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param ctx the rhash context
 * @param message message chunk
 * @param length length of the message chunk
 * @return 0 on success; On fail return -1 and set errno
 */
RHASH_API int rhash_update(rhash ctx, const void* message, size_t length)
{
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	unsigned i;
	
	assert(ectx->hash_vector_size <= RHASH_HASH_COUNT);
	if(ectx->state != STATE_ACTIVE) return 0; /* do nothing if canceled */

	ctx->msg_size += length;

	/* call update method for every algorithm */
	for(i = 0; i < ectx->hash_vector_size; i++) {
		struct rhash_hash_info* info = ectx->vector[i].hash_info;
		assert(info->update != 0);
		info->update(ectx->vector[i].context, message, length);
	}
	return 0; /* no error processing at the moment */
}

/**
 * Finalize hash calculation and optionally store the first hash.
 *
 * @param ctx the rhash context
 * @param first_result optional buffer to store a calculated hash with the lowest available id
 * @return 0 on success; On fail return -1 and set errno
 */
RHASH_API int rhash_final(rhash ctx, unsigned char* first_result)
{
	unsigned i = 0;
	unsigned char buffer[130];
	unsigned char* out = (first_result ? first_result : buffer);
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	assert(ectx->hash_vector_size <= RHASH_HASH_COUNT);

	/* skip final call if already finalized and auto-final is on */
	if((ectx->flags & RCTX_FINALIZED_MASK) ==
		(RCTX_AUTO_FINAL | RCTX_FINALIZED)) return 0;

	/* call final method for every algorithm */
	for(i = 0; i < ectx->hash_vector_size; i++) {
		struct rhash_hash_info* info = ectx->vector[i].hash_info;
		assert(info->final != 0);
		assert(info->info.digest_size < sizeof(buffer));
		info->final(ectx->vector[i].context, out);
		out = buffer;
	}
	ectx->flags |= RCTX_FINALIZED;
	return 0; /* no error processing at the moment */
}

/**
 * Store digest for given hash_id.
 * If hash_id is zero, function stores digest for a hash with the lowest id found in the context.
 * For nonzero hash_id the context must contain it, otherwise function silently does nothing.
 *
 * @param ctx rhash context
 * @param hash_id id of hash to retrieve or zero for hash with the lowest available id
 * @param result buffer to put the hash into
 */
static void rhash_put_digest(rhash ctx, unsigned hash_id, unsigned char* result)
{
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	unsigned i;
	rhash_vector_item *item;
	struct rhash_hash_info* info;
	unsigned char* digest;

	assert(ectx);
	assert(ectx->hash_vector_size > 0 && ectx->hash_vector_size <= RHASH_HASH_COUNT);

	/* finalize context if not yet finalized and auto-final is on */
	if((ectx->flags & RCTX_FINALIZED_MASK) == RCTX_AUTO_FINAL) {
		rhash_final(ctx, NULL);
	}

	if(hash_id == 0) {
		item = &ectx->vector[0]; /* get the first hash */
		info = item->hash_info;
	} else {
		for(i = 0;; i++) {
			if(i >= ectx->hash_vector_size) {
				return; /* hash_id not found, do nothing */
			}
			item = &ectx->vector[i];
			info = item->hash_info;
			if(info->info->hash_id == hash_id) break;
		}
	}
	digest = ((unsigned char*)item->context + info->digest_diff);
	if(info->info->flags & F_SWAP32) {
		rhash_u32_swap_copy(result, 0, digest, info->info->digest_size);
	} else if(info->info->flags & F_SWAP64) {
		rhash_u64_swap_copy(result, 0, digest, info->info->digest_size);
	} else {
		memcpy(result, digest, info->info->digest_size);
	}
}

/**
 * Set the callback function to be called from the
 * rhash_file() and rhash_file_update() functions
 * on processing every file block. The file block
 * size is set internally by rhash and now is 8 KiB.
 *
 * @param ctx rhash context
 * @param callback pointer to the callback function
 * @param callback_data pointer to data passed to the callback
 */
RHASH_API void rhash_set_callback(rhash ctx, rhash_callback_t callback, void* callback_data)
{
	((rhash_context_ext*)ctx)->callback = callback;
	((rhash_context_ext*)ctx)->callback_data = callback_data;
}


/* hi-level message hashing interface */

/**
 * Compute a hash of the given message.
 *
 * @param hash_id id of hash sum to compute
 * @param message the message to process
 * @param length message length
 * @param result buffer to receive binary hash string
 * @return 0 on success, -1 on error
 */
RHASH_API int rhash_msg(unsigned hash_id, const void* message, size_t length, unsigned char* result)
{
	rhash ctx;
	hash_id &= RHASH_ALL_HASHES;
	ctx = rhash_init(hash_id);
	if(ctx == NULL) return -1;
	rhash_update(ctx, message, length);
	rhash_final(ctx, result);
	rhash_free(ctx);
	return 0;
}

/**
 * Hash a file or stream. Multiple hashes can be computed.
 * First, inintialize ctx parameter with rhash_init() before calling
 * rhash_file_update(). Then use rhash_final() and rhash_print()
 * to retrive hash values. Finaly call rhash_free() on ctx
 * to free allocated memory or call rhash_reset() to reuse ctx.
 *
 * @param ctx rhash context
 * @param fd descriptor of the file to hash
 * @return 0 on success, -1 on error and errno is set
 */
RHASH_API int rhash_file_update(rhash ctx, FILE* fd)
{
	rhash_context_ext* const ectx = (rhash_context_ext*)ctx;
	const size_t block_size = 8192;
	unsigned char *buffer, *pmem;
	size_t length = 0, align8;
	int res = 0;
	if(ectx->state != STATE_ACTIVE) return 0; /* do nothing if canceled */

	if(ctx == NULL) {
		errno = EINVAL;
		return -1;
	}

	pmem = (unsigned char*)malloc(block_size + 8);
	if(!pmem) return -1; /* errno is set to ENOMEM according to UNIX 98 */

	align8 = ((unsigned char*)0 - pmem) & 7;
	buffer = pmem + align8;

	while(!feof(fd)) {
		if(ectx->state != STATE_ACTIVE) break; /* stop if canceled */

		length = fread(buffer, 1, block_size, fd);
		/* read can return -1 on error */
		if(length == (size_t)-1) {
			res = -1; /* note: fread sets errno */
			break;
		}
		rhash_update(ctx, buffer, length);

		if(ectx->callback) {
			((rhash_callback_t)ectx->callback)(ectx->callback_data, ectx->rc.msg_size);
		}
	}

	free(buffer);
	return res;
}

/**
 * Compute a single hash for given file.
 *
 * @param hash_id id of hash sum to compute
 * @param filepath path to the file to hash
 * @param result buffer to receive hash value with the lowest requested id
 * @return 0 on success, -1 on error and errno is set
 */
RHASH_API int rhash_file(unsigned hash_id, const char* filepath, unsigned char* result)
{
	FILE* fd;
	rhash ctx;
	int res;

	hash_id &= RHASH_ALL_HASHES;
	if(hash_id == 0) return -1;

	if((fd = fopen(filepath, "rb")) == NULL) return -1;

	if((ctx = rhash_init(hash_id)) == NULL) return -1;

	res = rhash_file_update(ctx, fd); /* hash the file */
	fclose(fd);

	rhash_final(ctx, result);
	rhash_free(ctx);
	return res;
}

#ifdef _WIN32 /* windows only function */
#include <share.h>

/**
 * Compute a single hash for given file.
 *
 * @param hash_id id of hash sum to compute
 * @param filepath path to the file to hash
 * @param result buffer to receive hash value with the lowest requested id
 * @return 0 on success, -1 on error, -1 on error and errno is set
 */
RHASH_API int rhash_wfile(unsigned hash_id, const wchar_t* filepath, unsigned char* result)
{
	FILE* fd;
	rhash ctx;
	int res;

	hash_id &= RHASH_ALL_HASHES;
	if(hash_id == 0) return -1;

	if((fd = _wfsopen(filepath, L"rb", _SH_DENYWR)) == NULL) return -1;

	if((ctx = rhash_init(hash_id)) == NULL) return -1;

	res = rhash_file_update(ctx, fd); /* hash the file */
	fclose(fd);

	rhash_final(ctx, result);
	rhash_free(ctx);
	return 0;
}
#endif

/* RHash information functions */

/**
 * Returns information about a hash function by its hash_id.
 *
 * @param hash_id the id of hash algorithm
 * @return pointer to the rhash_info structure containing the information
 */
const rhash_info* rhash_info_by_id(unsigned hash_id)
{
	hash_id &= RHASH_ALL_HASHES;
	/* check that only one bit is set */
	if(hash_id != (hash_id & -(int)hash_id)) return NULL;
	/* note: alternative condition is (hash_id == 0 || (hash_id & (hash_id - 1)) != 0) */
	return rhash_info_table[rhash_ctz(hash_id)].info;
}

/**
 * Detect default digest output format for given hash algorithm.
 *
 * @param hash_id the id of hash algorithm
 * @return 1 for base32 format, 0 for hexadecimal
 */
RHASH_API int rhash_is_base32(unsigned hash_id)
{
	/* fast method is just to test a bit-mask */
	return ((hash_id & (RHASH_TTH | RHASH_AICH)) != 0);
}

/**
 * Returns size of binary digest for given hash algorithm.
 *
 * @param hash_id the id of hash algorithm
 * @return digest size in bytes
 */
RHASH_API int rhash_get_digest_size(unsigned hash_id)
{
	hash_id &= RHASH_ALL_HASHES;
	if(hash_id == 0 || (hash_id & (hash_id - 1)) != 0) return -1;
	return (int)rhash_info_table[rhash_ctz(hash_id)].info->digest_size;
}

/**
 * Returns length of digest hash string in default output format.
 *
 * @param hash_id the id of hash algorithm
 * @return the length of hash string
 */
RHASH_API int rhash_get_hash_length(unsigned hash_id)
{
	const rhash_info* info = rhash_info_by_id(hash_id);
	return (int)(info ? (info->flags & F_BS32 ?
		BASE32_LENGTH(info->digest_size) : info->digest_size * 2) : 0);
}

/**
 * Returns a name of given hash algorithm.
 *
 * @param hash_id the id of hash algorithm
 * @return algorithm name
 */
RHASH_API const char* rhash_get_name(unsigned hash_id)
{
	const rhash_info* info = rhash_info_by_id(hash_id);
	return (info ? info->name : 0);
}

/**
 * Returns a name part of magnet urn of the given hash algorithm.
 * Such magnet_name is used to generate a magnet link of the form
 * urn:&lt;magnet_name&gt;=&lt;hash_value&gt;.
 *
 * @param hash_id the id of hash algorithm
 * @return name
 */
RHASH_API const char* rhash_get_magnet_name(unsigned hash_id)
{
	const rhash_info* info = rhash_info_by_id(hash_id);
	return (info ? info->magnet_name : 0);
}

/* hash sum output */

/**
 * Print a text presentation of a given hash sum to the specified buffer,
 *
 * @param output a buffer to print the hash to
 * @param bytes a hash sum to print
 * @param size a size of hash sum in bytes
 * @param flags  a bit-mask controlling how to format the hash sum,
 *               can be a mix of the flags: RHPR_RAW, RHPR_HEX, RHPR_BASE32,
 *               RHPR_BASE64, RHPR_UPPERCASE, RHPR_REVERSE
 * @return the number of written characters
 */
size_t rhash_print_bytes(char* output, const unsigned char* bytes,
	size_t size, int flags)
{
	size_t str_len;
	int upper_case = (flags & RHPR_UPPERCASE);
	int format = (flags & ~(RHPR_UPPERCASE | RHPR_REVERSE));

	switch(format) {
	case RHPR_HEX:
		str_len = size * 2;
		rhash_byte_to_hex(output, bytes, (unsigned)size, upper_case);
		break;
	case RHPR_BASE32:
		str_len = BASE32_LENGTH(size);
		rhash_byte_to_base32(output, bytes, (unsigned)size, upper_case);
		break;
	case RHPR_BASE64:
		str_len = BASE64_LENGTH(size);
		rhash_byte_to_base64(output, bytes, (unsigned)size);
		break;
	default:
		str_len = size;
		memcpy(output, bytes, size);
		break;
	}
	return str_len;
}

/**
 * Print text presentation of a hash sum with given hash_id to output buffer.
 * If hash_id is zero, then print hash sum with the lowest id stored in hash
 * context. Function fails if hash_id doesn't exist within the context.
 *
 * @param output a buffer to print the hash to
 * @param context algorithms state
 * @param hash_id id of the hash sum to print or 0 to print the first hash
 *                saved in the context.
 * @param flags  controls how to print the sum, can contain flags
 *               RHPR_UPPERCASE, RHPR_HEX, RHPR_BASE32, RHPR_BASE64, etc.
 * @return number of writen characters on success, 0 on fail
 */
size_t RHASH_API rhash_print(char* output, rhash context, unsigned hash_id, int flags)
{
	const rhash_info* info;
	unsigned char digest[80];
	size_t digest_size;

	info = (hash_id != 0 ? rhash_info_by_id(hash_id) :
		((rhash_context_ext*)context)->vector[0].hash_info->info);

	if(info == NULL) return 0;
	digest_size = info->digest_size;
	assert(digest_size <= 64);

	if(output == NULL) {
		int format = (flags & ~(RHPR_UPPERCASE | RHPR_REVERSE));
		switch(format) {
		case RHPR_HEX:
			return (digest_size * 2);
		case RHPR_BASE32:
			return BASE32_LENGTH(digest_size);
		case RHPR_BASE64:
			return BASE64_LENGTH(digest_size);
		default:
			return digest_size;
		}
	}

	/* note: use info->hash_id, cause hash_id can be 0 */
	rhash_put_digest(context, info->hash_id, digest);

	/* use default text presentation if not specified by flags */
	if((flags & ~(RHPR_UPPERCASE|RHPR_REVERSE)) == 0) {
		flags |= (info->flags & RHASH_INFO_BASE32 ? RHPR_BASE32 : RHPR_HEX);
	}

	if((flags & ~RHPR_UPPERCASE) == (RHPR_REVERSE|RHPR_HEX)) {
		/* reverse the digest */
		unsigned char *p = digest, *r = digest + digest_size - 1;
		char tmp;
		for(; p < r; p++, r--) {
			tmp = *p;
			*p = *r;
			*r = tmp;
		}
	}

	return rhash_print_bytes(output, digest, digest_size, flags);
}

#if defined(_WIN32) && defined(RHASH_EXPORTS)
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	(void)hModule;
	(void)reserved;
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		rhash_library_init();
		break;
	case DLL_PROCESS_DETACH:
		/*rhash_library_free();*/
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
#endif

/**
 * Process a BitTorrent-related rhash message.
 *
 * @param msg_id message identifier
 * @param bt BitTorrent context
 * @param ldata data depending on message
 * @param rdata data depending on message
 * @return message-specific data
 */
static rhash_uptr_t process_bt_msg(unsigned msg_id, torrent_ctx* bt, rhash_uptr_t ldata, rhash_uptr_t rdata)
{
	if(bt == NULL) return RHASH_ERROR;

	switch(msg_id) {
	case RMSG_BT_ADD_FILE:
		rhash_torrent_add_file(bt, (const char*)ldata, *(unsigned long long*)rdata);
		break;
	case RMSG_BT_SET_OPTIONS:
		rhash_torrent_set_options(bt, (unsigned)ldata);
		break;
	case RMSG_BT_SET_ANNOUNCE:
		rhash_torrent_set_announce(bt, (const char*)ldata);
		break;
	case RMSG_BT_SET_PIECE_LENGTH:
		rhash_torrent_set_piece_length(bt, (size_t)ldata);
		break;
	case RMSG_BT_SET_PROGRAM_NAME:
		rhash_torrent_set_program_name(bt, (const char*)ldata);
		break;
	case RMSG_BT_GET_TEXT:
		return RHASH_STR2UPTR(rhash_torrent_get_text(bt, (char**)ldata));
	default:
		return RHASH_ERROR; /* unknown message */
	}
	return 0;
}

#define PVOID2UPTR(p) ((rhash_uptr_t)((char*)p - 0))

/**
 * Process a rhash message.
 *
 * @param msg_id message identifier
 * @param dst message destination (can be NULL for generic messages)
 * @param ldata data depending on message
 * @param rdata data depending on message
 * @return message-specific data
 */
RHASH_API rhash_uptr_t rhash_transmit(unsigned msg_id, void* dst, rhash_uptr_t ldata, rhash_uptr_t rdata)
{
	/* for messages working with rhash context */
	rhash_context_ext* const ctx = (rhash_context_ext*)dst;

	switch(msg_id) {
	case RMSG_GET_CONTEXT:
		{
			unsigned i;
			for(i = 0; i < ctx->hash_vector_size; i++) {
				struct rhash_hash_info* info = ctx->vector[i].hash_info;
				if(info->info->hash_id == (unsigned)ldata)
					return PVOID2UPTR(ctx->vector[i].context);
			}
			return (rhash_uptr_t)0;
		}

	case RMSG_CANCEL:
		/* mark rhash context as canceled, in a multithreaded program */
		atomic_compare_and_swap(&ctx->state, STATE_ACTIVE, STATE_STOPED);
		return 0;

	case RMSG_IS_CANCELED:
		return (ctx->state == STATE_STOPED);

	case RMSG_GET_FINALIZED:
		return ((ctx->flags & RCTX_FINALIZED) != 0);
	case RMSG_SET_AUTOFINAL:
		ctx->flags &= ~RCTX_AUTO_FINAL;
		if(ldata) ctx->flags |= RCTX_AUTO_FINAL;
		break;

	/* OpenSSL related messages */
#ifdef USE_OPENSSL
	case RMSG_SET_OPENSSL_MASK:
		rhash_openssl_hash_mask = (unsigned)ldata;
		break;
	case RMSG_GET_OPENSSL_MASK:
		return rhash_openssl_hash_mask;
#endif

	/* BitTorrent related messages */
	case RMSG_BT_ADD_FILE:
	case RMSG_BT_SET_OPTIONS:
	case RMSG_BT_SET_ANNOUNCE:
	case RMSG_BT_SET_PIECE_LENGTH:
	case RMSG_BT_SET_PROGRAM_NAME:
	case RMSG_BT_GET_TEXT:
		return process_bt_msg(msg_id, (torrent_ctx*)(((rhash_context_ext*)dst)->bt_ctx), ldata, rdata);

	default:
		return RHASH_ERROR; /* unknown message */
	}
	return 0;
}
