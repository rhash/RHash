/* algorithms.c - the algorithms supported by the rhash library
 *
 * Copyright (c) 2011, Aleksey Kravchenko <rhash.admin@gmail.com>
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

#include "algorithms.h"
#include "byte_order.h"
#include "rhash.h"
#include "util.h"

/* header files of all supported hash functions */
#include "aich.h"
#include "blake2b.h"
#include "blake2s.h"
#include "blake3.h"
#include "crc32.h"
#include "ed2k.h"
#include "edonr.h"
#include "gost12.h"
#include "gost94.h"
#include "has160.h"
#include "md4.h"
#include "md5.h"
#include "ripemd-160.h"
#include "snefru.h"
#include "sha_ni.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#include "sha3.h"
#include "tiger.h"
#include "torrent.h"
#include "tth.h"
#include "whirlpool.h"

#ifdef USE_OPENSSL
# include "plug_openssl.h"
#endif /* USE_OPENSSL */
#include <assert.h>

static unsigned algorithms_initialized_flag = 0;

rhash_hash_info* rhash_info_table = rhash_hash_info_default;
int rhash_info_size = RHASH_HASH_COUNT;

static void rhash_crc32_init(uint32_t* crc32);
static void rhash_crc32_update(uint32_t* crc32, const unsigned char* msg, size_t size);
static void rhash_crc32_final(uint32_t* crc32, unsigned char* result);
static void rhash_crc32c_init(uint32_t* crc32);
static void rhash_crc32c_update(uint32_t* crc32, const unsigned char* msg, size_t size);
static void rhash_crc32c_final(uint32_t* crc32, unsigned char* result);

rhash_info info_crc32      = { EXTENDED_HASH_ID(0),  F_BE32, 4, "CRC32", "crc32" };
rhash_info info_crc32c     = { EXTENDED_HASH_ID(26), F_BE32, 4, "CRC32C", "crc32c" };
rhash_info info_md4        = { EXTENDED_HASH_ID(1),  F_LE32, 16, "MD4", "md4" };
rhash_info info_md5        = { EXTENDED_HASH_ID(2),  F_LE32, 16, "MD5", "md5" };
rhash_info info_sha1       = { EXTENDED_HASH_ID(3),  F_BE32, 20, "SHA1", "sha1" };
rhash_info info_tiger      = { EXTENDED_HASH_ID(4),  F_LE64, 24, "TIGER", "tiger" };
rhash_info info_tth        = { EXTENDED_HASH_ID(5),  F_BS32 | F_SPCEXP, 24, "TTH", "tree:tiger" };
rhash_info info_btih       = { EXTENDED_HASH_ID(6),  F_SPCEXP, 20, "BTIH", "btih" };
rhash_info info_ed2k       = { EXTENDED_HASH_ID(7),  F_LE32, 16, "ED2K", "ed2k" };
rhash_info info_aich       = { EXTENDED_HASH_ID(8),  F_BS32 | F_SPCEXP, 20, "AICH", "aich" };
rhash_info info_whirlpool  = { EXTENDED_HASH_ID(9),  F_BE64, 64, "WHIRLPOOL", "whirlpool" };
rhash_info info_rmd160     = { EXTENDED_HASH_ID(10), F_LE32, 20, "RIPEMD-160", "ripemd160" };
rhash_info info_gost94     = { EXTENDED_HASH_ID(11), F_LE32, 32, "GOST94", "gost94" };
rhash_info info_gost94pro  = { EXTENDED_HASH_ID(12), F_LE32, 32, "GOST94-CRYPTOPRO", "gost94-cryptopro" };
rhash_info info_has160     = { EXTENDED_HASH_ID(13), F_LE32, 20, "HAS-160", "has160" };
rhash_info info_gost12_256 = { EXTENDED_HASH_ID(14), F_LE64, 32, "GOST12-256", "gost12-256" };
rhash_info info_gost12_512 = { EXTENDED_HASH_ID(15), F_LE64, 64, "GOST12-512", "gost12-512" };
rhash_info info_sha224     = { EXTENDED_HASH_ID(16), F_BE32, 28, "SHA-224", "sha224" };
rhash_info info_sha256     = { EXTENDED_HASH_ID(17), F_BE32, 32, "SHA-256", "sha256" };
rhash_info info_sha384     = { EXTENDED_HASH_ID(18), F_BE64, 48, "SHA-384", "sha384" };
rhash_info info_sha512     = { EXTENDED_HASH_ID(19), F_BE64, 64, "SHA-512", "sha512" };
rhash_info info_edr256     = { EXTENDED_HASH_ID(20), F_LE32, 32, "EDON-R256", "edon-r256" };
rhash_info info_edr512     = { EXTENDED_HASH_ID(21), F_LE64, 64, "EDON-R512", "edon-r512" };
rhash_info info_sha3_224   = { EXTENDED_HASH_ID(22), F_LE64, 28, "SHA3-224", "sha3-224" };
rhash_info info_sha3_256   = { EXTENDED_HASH_ID(23), F_LE64, 32, "SHA3-256", "sha3-256" };
rhash_info info_sha3_384   = { EXTENDED_HASH_ID(24), F_LE64, 48, "SHA3-384", "sha3-384" };
rhash_info info_sha3_512   = { EXTENDED_HASH_ID(25), F_LE64, 64, "SHA3-512", "sha3-512" };
rhash_info info_snf128     = { EXTENDED_HASH_ID(27), F_BE32, 16, "SNEFRU-128", "snefru128" };
rhash_info info_snf256     = { EXTENDED_HASH_ID(28), F_BE32, 32, "SNEFRU-256", "snefru256" };
rhash_info info_blake2s    = { EXTENDED_HASH_ID(29), F_LE32, 32, "BLAKE2S", "blake2s" };
rhash_info info_blake2b    = { EXTENDED_HASH_ID(30), F_LE64, 64, "BLAKE2B", "blake2b" };
rhash_info info_blake3     = { EXTENDED_HASH_ID(31), F_LE32 | F_SPCEXP, 32, "BLAKE3", "blake3" };

/* some helper macros */
#define dgshft(name) ((uintptr_t)((char*)&((name##_ctx*)0)->hash))
#define dgshft2(name, field) ((uintptr_t)((char*)&((name##_ctx*)0)->field))
#define ini(name) ((pinit_t)(name##_init))
#define upd(name) ((pupdate_t)(name##_update))
#define fin(name) ((pfinal_t)(name##_final))
#define iuf(name) ini(name), upd(name), fin(name)
#define iuf2(name1, name2) ini(name1), upd(name2), fin(name2)

/* information about all supported hash functions */
rhash_hash_info rhash_hash_info_default[] =
{
	{ &info_crc32, sizeof(uint32_t), 0, iuf(rhash_crc32), 0 }, /* 32 bit */
	{ &info_md4, sizeof(md4_ctx), dgshft(md4), iuf(rhash_md4), 0 }, /* 128 bit */
	{ &info_md5, sizeof(md5_ctx), dgshft(md5), iuf(rhash_md5), 0 }, /* 128 bit */
	{ &info_sha1, sizeof(sha1_ctx), dgshft(sha1), iuf(rhash_sha1), 0 }, /* 160 bit */
	{ &info_tiger, sizeof(tiger_ctx), dgshft(tiger), iuf(rhash_tiger), 0 }, /* 192 bit */
	{ &info_tth, sizeof(tth_ctx), dgshft2(tth, tiger.hash), iuf(rhash_tth), 0 }, /* 192 bit */
	{ &info_btih, sizeof(torrent_ctx), dgshft2(torrent, btih), iuf(bt), (pcleanup_t)bt_cleanup }, /* 160 bit */
	{ &info_ed2k, sizeof(ed2k_ctx), dgshft2(ed2k, md4_context_inner.hash), iuf(rhash_ed2k), 0 }, /* 128 bit */
	{ &info_aich, sizeof(aich_ctx), dgshft2(aich, sha1_context.hash), iuf(rhash_aich), (pcleanup_t)rhash_aich_cleanup }, /* 160 bit */
	{ &info_whirlpool, sizeof(whirlpool_ctx), dgshft(whirlpool), iuf(rhash_whirlpool), 0 }, /* 512 bit */
	{ &info_rmd160, sizeof(ripemd160_ctx), dgshft(ripemd160), iuf(rhash_ripemd160), 0 }, /* 160 bit */
	{ &info_gost94, sizeof(gost94_ctx), dgshft(gost94), iuf(rhash_gost94), 0 }, /* 256 bit */
	{ &info_gost94pro, sizeof(gost94_ctx), dgshft(gost94), iuf2(rhash_gost94_cryptopro, rhash_gost94), 0 }, /* 256 bit */
	{ &info_has160, sizeof(has160_ctx), dgshft(has160), iuf(rhash_has160), 0 }, /* 160 bit */
	{ &info_gost12_256, sizeof(gost12_ctx), dgshft2(gost12, h) + 32, iuf2(rhash_gost12_256, rhash_gost12), 0 }, /* 256 bit */
	{ &info_gost12_512, sizeof(gost12_ctx), dgshft2(gost12, h), iuf2(rhash_gost12_512, rhash_gost12), 0 }, /* 512 bit */
	{ &info_sha224, sizeof(sha256_ctx), dgshft(sha256), iuf2(rhash_sha224, rhash_sha256), 0 }, /* 224 bit */
	{ &info_sha256, sizeof(sha256_ctx), dgshft(sha256), iuf(rhash_sha256), 0 },  /* 256 bit */
	{ &info_sha384, sizeof(sha512_ctx), dgshft(sha512), iuf2(rhash_sha384, rhash_sha512), 0 }, /* 384 bit */
	{ &info_sha512, sizeof(sha512_ctx), dgshft(sha512), iuf(rhash_sha512), 0 },  /* 512 bit */
	{ &info_edr256, sizeof(edonr_ctx),  dgshft2(edonr, u.data256.hash) + 32, iuf(rhash_edonr256), 0 },  /* 256 bit */
	{ &info_edr512, sizeof(edonr_ctx),  dgshft2(edonr, u.data512.hash) + 64, iuf(rhash_edonr512), 0 },  /* 512 bit */
	{ &info_sha3_224, sizeof(sha3_ctx), dgshft(sha3), iuf2(rhash_sha3_224, rhash_sha3), 0 }, /* 224 bit */
	{ &info_sha3_256, sizeof(sha3_ctx), dgshft(sha3), iuf2(rhash_sha3_256, rhash_sha3), 0 }, /* 256 bit */
	{ &info_sha3_384, sizeof(sha3_ctx), dgshft(sha3), iuf2(rhash_sha3_384, rhash_sha3), 0 }, /* 384 bit */
	{ &info_sha3_512, sizeof(sha3_ctx), dgshft(sha3), iuf2(rhash_sha3_512, rhash_sha3), 0 }, /* 512 bit */
	{ &info_crc32c, sizeof(uint32_t), 0, iuf(rhash_crc32c), 0 }, /* 32 bit */
	{ &info_snf128, sizeof(snefru_ctx), dgshft(snefru), iuf2(rhash_snefru128, rhash_snefru), 0 }, /* 128 bit */
	{ &info_snf256, sizeof(snefru_ctx), dgshft(snefru), iuf2(rhash_snefru256, rhash_snefru), 0 }, /* 256 bit */
	{ &info_blake2s, sizeof(blake2s_ctx),  dgshft(blake2s), iuf(rhash_blake2s), 0 },  /* 256 bit */
	{ &info_blake2b, sizeof(blake2b_ctx),  dgshft(blake2b), iuf(rhash_blake2b), 0 },  /* 512 bit */
	{ &info_blake3, sizeof(blake3_ctx),  dgshft2(blake3, root.hash), iuf(rhash_blake3), 0 }       /* 256 bit */
};

#if defined(RHASH_SSE4_SHANI) && !defined(RHASH_DISABLE_SHANI)
static void table_init_sha_ext(void)
{
	if (has_cpu_feature(CPU_FEATURE_SHANI))
	{
		assert(rhash_hash_info_default[3].init == (pinit_t)rhash_sha1_init);
		rhash_hash_info_default[3].update = (pupdate_t)rhash_sha1_ni_update;
		rhash_hash_info_default[3].final = (pfinal_t)rhash_sha1_ni_final;
		assert(rhash_hash_info_default[16].init == (pinit_t)rhash_sha224_init);
		rhash_hash_info_default[16].update = (pupdate_t)rhash_sha256_ni_update;
		rhash_hash_info_default[16].final = (pfinal_t)rhash_sha256_ni_final;
		assert(rhash_hash_info_default[17].init == (pinit_t)rhash_sha256_init);
		rhash_hash_info_default[17].update = (pupdate_t)rhash_sha256_ni_update;
		rhash_hash_info_default[17].final = (pfinal_t)rhash_sha256_ni_final;
	}
}
#else
# define table_init_sha_ext() {}
#endif

/**
 * Initialize requested algorithms.
 */
void rhash_init_algorithms(void)
{
	if (algorithms_initialized_flag)
		return;
	/* check RHASH_HASH_COUNT */
	RHASH_ASSERT((RHASH_LOW_HASHES_MASK >> RHASH_HASH_COUNT) == 0);
	RHASH_ASSERT(RHASH_COUNTOF(rhash_hash_info_default) == RHASH_HASH_COUNT);

#ifdef GENERATE_GOST94_LOOKUP_TABLE
	rhash_gost94_init_table();
#endif
	table_init_sha_ext();
	atomic_compare_and_swap(&algorithms_initialized_flag, 0, 1);
}

/**
 * Returns information about a hash function by its hash_id.
 *
 * @param hash_id the id of hash algorithm
 * @return pointer to the rhash_info structure containing the information
 */
const rhash_hash_info* rhash_hash_info_by_id(unsigned hash_id)
{
	unsigned index;
	if (IS_EXTENDED_HASH_ID(hash_id)) {
		index = hash_id & ~RHASH_EXTENDED_BIT;
		if (index >= RHASH_HASH_COUNT)
			return NULL;
	} else {
		hash_id &= RHASH_LOW_HASHES_MASK;
		/* check that one and only one bit is set */
		if (!hash_id || (hash_id & (hash_id - 1)) != 0)
			return NULL;
		index = rhash_ctz(hash_id);
	}
	return &rhash_info_table[index];
}

/**
 * Return array of hash identifiers of supported hash functions.
 * If the all_id is different from RHASH_ALL_HASHES,
 * then return hash identifiers of legacy hash functions
 * to support old library clients.
 *
 * @param all_id constant used to get all hash identifiers
 * @param count pointer to store the number of returned ids to
 * @return array of hash identifiers
 */
const unsigned* rhash_get_all_hash_ids(unsigned all_id, size_t* count)
{
	static const unsigned all_ids[] = {
		EXTENDED_HASH_ID(0), EXTENDED_HASH_ID(1), EXTENDED_HASH_ID(2), EXTENDED_HASH_ID(3),
		EXTENDED_HASH_ID(4), EXTENDED_HASH_ID(5), EXTENDED_HASH_ID(6), EXTENDED_HASH_ID(7),
		EXTENDED_HASH_ID(8), EXTENDED_HASH_ID(9), EXTENDED_HASH_ID(10), EXTENDED_HASH_ID(11),
		EXTENDED_HASH_ID(12), EXTENDED_HASH_ID(13), EXTENDED_HASH_ID(14), EXTENDED_HASH_ID(15),
		EXTENDED_HASH_ID(16), EXTENDED_HASH_ID(17), EXTENDED_HASH_ID(18), EXTENDED_HASH_ID(19),
		EXTENDED_HASH_ID(20), EXTENDED_HASH_ID(21), EXTENDED_HASH_ID(22), EXTENDED_HASH_ID(23),
		EXTENDED_HASH_ID(24), EXTENDED_HASH_ID(25), EXTENDED_HASH_ID(26), EXTENDED_HASH_ID(27),
		EXTENDED_HASH_ID(28), EXTENDED_HASH_ID(29), EXTENDED_HASH_ID(30), EXTENDED_HASH_ID(31)
	};
	static const unsigned count_low = rhash_popcount(RHASH_LOW_HASHES_MASK);
	RHASH_ASSERT(RHASH_COUNTOF(all_ids) == RHASH_HASH_COUNT);
	*count = (all_id == RHASH_ALL_HASHES ? RHASH_HASH_COUNT : count_low);
	return all_ids;
}

/* CRC32 helper functions */

/**
 * Initialize crc32 hash.
 *
 * @param crc32 pointer to the hash to initialize
 */
static void rhash_crc32_init(uint32_t* crc32)
{
	*crc32 = 0; /* note: context size is sizeof(uint32_t) */
}

/**
 * Calculate message CRC32 hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param crc32 pointer to the hash
 * @param msg message chunk
 * @param size length of the message chunk
 */
static void rhash_crc32_update(uint32_t* crc32, const unsigned char* msg, size_t size)
{
	*crc32 = rhash_get_crc32(*crc32, msg, size);
}

/**
 * Store calculated hash into the given array.
 *
 * @param crc32 pointer to the current hash value
 * @param result calculated hash in binary form
 */
static void rhash_crc32_final(uint32_t* crc32, unsigned char* result)
{
#if defined(CPU_IA32) || defined(CPU_X64)
	/* intel CPUs support assigment with non 32-bit aligned pointers */
	*(unsigned*)result = be2me_32(*crc32);
#else
	/* correct saving BigEndian integer on all archs */
	result[0] = (unsigned char)(*crc32 >> 24), result[1] = (unsigned char)(*crc32 >> 16);
	result[2] = (unsigned char)(*crc32 >> 8), result[3] = (unsigned char)(*crc32);
#endif
}

/**
 * Initialize crc32c hash.
 *
 * @param crc32c pointer to the hash to initialize
 */
static void rhash_crc32c_init(uint32_t* crc32c)
{
	*crc32c = 0; /* note: context size is sizeof(uint32_t) */
}

/**
 * Calculate message CRC32C hash.
 * Can be called repeatedly with chunks of the message to be hashed.
 *
 * @param crc32c pointer to the hash
 * @param msg message chunk
 * @param size length of the message chunk
 */
static void rhash_crc32c_update(uint32_t* crc32c, const unsigned char* msg, size_t size)
{
	*crc32c = rhash_get_crc32c(*crc32c, msg, size);
}

/**
 * Store calculated hash into the given array.
 *
 * @param crc32c pointer to the current hash value
 * @param result calculated hash in binary form
 */
static void rhash_crc32c_final(uint32_t* crc32c, unsigned char* result)
{
#if defined(CPU_IA32) || defined(CPU_X64)
	/* intel CPUs support assigment with non 32-bit aligned pointers */
	*(unsigned*)result = be2me_32(*crc32c);
#else
	/* correct saving BigEndian integer on all archs */
	result[0] = (unsigned char)(*crc32c >> 24), result[1] = (unsigned char)(*crc32c >> 16);
	result[2] = (unsigned char)(*crc32c >> 8), result[3] = (unsigned char)(*crc32c);
#endif
}

#if !defined(NO_IMPORT_EXPORT)
#define EXTENDED_TTH EXTENDED_HASH_ID(5)
#define EXTENDED_AICH EXTENDED_HASH_ID(8)

/**
 * Export a hash function context to a memory region,
 * or calculate the size required for context export.
 *
 * @param hash_id identifier of the hash function
 * @param ctx the algorithm context containing current hashing state
 * @param out pointer to the memory region or NULL
 * @param size size of memory region
 * @return the size of the exported data on success, 0 on fail.
 */
size_t rhash_export_alg(unsigned hash_id, const void* ctx, void* out, size_t size)
{
	switch (hash_id)
	{
		case RHASH_TTH:
		case EXTENDED_TTH:
			return rhash_tth_export((const tth_ctx*)ctx, out, size);
		case RHASH_BTIH:
		case EXTENDED_BTIH:
			return bt_export((const torrent_ctx*)ctx, out, size);
		case RHASH_AICH:
		case EXTENDED_AICH:
			return rhash_aich_export((const aich_ctx*)ctx, out, size);
		case RHASH_BLAKE3:
			return rhash_blake3_export((const blake3_ctx*)ctx, out, size);
	}
	return 0;
}

/**
 * Import a hash function context from a memory region.
 *
 * @param hash_id identifier of the hash function
 * @param ctx pointer to the algorithm context
 * @param in pointer to the data to import
 * @param size size of data to import
 * @return the size of the imported data on success, 0 on fail.
 */
size_t rhash_import_alg(unsigned hash_id, void* ctx, const void* in, size_t size)
{
	switch (hash_id)
	{
		case RHASH_TTH:
		case EXTENDED_TTH:
			return rhash_tth_import((tth_ctx*)ctx, in, size);
		case RHASH_BTIH:
		case EXTENDED_BTIH:
			return bt_import((torrent_ctx*)ctx, in, size);
		case RHASH_AICH:
		case EXTENDED_AICH:
			return rhash_aich_import((aich_ctx*)ctx, in, size);
		case RHASH_BLAKE3:
			return rhash_blake3_import((blake3_ctx*)ctx, in, size);
	}
	return 0;
}
#endif /* !defined(NO_IMPORT_EXPORT) */

#ifdef USE_OPENSSL
void rhash_load_sha1_methods(rhash_hashing_methods* methods, int methods_type)
{
	int use_openssl;
	switch (methods_type) {
		case METHODS_OPENSSL:
			use_openssl = 1;
			break;
		case METHODS_SELECTED:
			assert(rhash_info_table[3].info->hash_id == EXTENDED_SHA1);
			use_openssl = ARE_OPENSSL_METHODS(rhash_info_table[3]);
			break;
		default:
			use_openssl = 0;
			break;
	}
	if (use_openssl) {
		methods->init = rhash_ossl_sha1_init();
		methods->update = rhash_ossl_sha1_update();
		methods->final = rhash_ossl_sha1_final();
	} else {
		methods->init = (pinit_t)&rhash_sha1_init;
		methods->update = (pupdate_t)&rhash_sha1_update;
		methods->final = (pfinal_t)&rhash_sha1_final;
	}
}
#endif
