/* algorithms.c - the algorithms supported by the rhash library
 * written by Alexei Kravchenko.
 *
 * Copyleft:
 * I, the author, hereby place this code into the public domain.
 * This applies worldwide. I grant any entity the right to use this work for
 * ANY PURPOSE, without any conditions, unless such conditions are required
 * by law.
 */

#include <stdio.h>
#include <assert.h>

#include "byte_order.h"
#include "rhash.h"
#include "algorithms.h"

/* header files of all supported hash sums */
#include "aich.h"
#include "crc32.h"
#include "ed2k.h"
#include "edonr.h"
#include "gost.h"
#include "has160.h"
#include "md4.h"
#include "md5.h"
#include "ripemd-160.h"
#include "snefru.h"
#include "sha1.h"
#include "sha256.h"
#include "sha512.h"
#include "tiger.h"
#include "torrent.h"
#include "tth.h"
#include "whirlpool.h"

#ifdef USE_OPENSSL
/* note: BTIH and AICH depends on the used SHA1 algorithm */
# define NEED_OPENSSL_INIT (RHASH_MD4 | RHASH_MD5 | \
	RHASH_SHA1 | RHASH_SHA224 | RHASH_SHA256 | RHASH_SHA384 | RHASH_SHA512 | \
	RHASH_BTIH | RHASH_AICH | RHASH_RIPEMD160 | RHASH_WHIRLPOOL)
#else
# define NEED_OPENSSL_INIT 0
#endif /* USE_OPENSSL */
#ifdef GENERATE_GOST_LOOKUP_TABLE
# define NEED_GOST_INIT (RHASH_GOST | RHASH_GOST_CRYPTOPRO)
#else
# define NEED_GOST_INIT 0
#endif /* GENERATE_GOST_LOOKUP_TABLE */
#ifdef GENERATE_CRC32_TABLE
# define NEED_CRC32_INIT RHASH_CRC32
#else
# define NEED_CRC32_INIT 0
#endif /* GENERATE_CRC32_TABLE */

#define RHASH_NEED_INIT_ALG (NEED_CRC32_INIT | NEED_GOST_INIT | NEED_OPENSSL_INIT)
unsigned rhash_uninitialized_algorithms = RHASH_NEED_INIT_ALG;

rhash_hash_info* rhash_info_table = rhash_hash_info_default;
int rhash_info_size = RHASH_HASH_COUNT;

static void rhash_crc32_init(uint32_t* crc32);
static void rhash_crc32_update(uint32_t* crc32, const unsigned char* msg, size_t size);
static void rhash_crc32_final(uint32_t* crc32, unsigned char* result);

/* some helper macroses
 * like shift in bytes of a message digest in a hash sum context */
#define dgshft(name) (((char*)&((name##_ctx*)0)->hash) - (char*)0)
#define dgshft2(name, field) (((char*)&((name##_ctx*)0)->field) - (char*)0)
#define ini(name) ((pinit_t)(name##_init))
#define upd(name) ((pupdate_t)(name##_update))
#define fin(name) ((pfinal_t)(name##_final))
#define iuf(name) ini(name), upd(name), fin(name)
#define diuf(name) dgshft(name), ini(name), upd(name), fin(name)

/* information about all hashes */
rhash_hash_info rhash_hash_info_default[RHASH_HASH_COUNT] =
{
	{ { RHASH_CRC32,     F_BE32,  4, "CRC32" }, sizeof(uint32_t), 0, iuf(rhash_crc32), 0 }, /* 32 bit */
	{ { RHASH_MD4,       F_LE32, 16, "MD4" }, sizeof(md4_ctx), dgshft(md4), iuf(rhash_md4), 0 }, /* 128 bit */
	{ { RHASH_MD5,       F_LE32, 16,  "MD5" }, sizeof(md5_ctx), dgshft(md5), iuf(rhash_md5), 0 }, /* 128 bit */
	{ { RHASH_SHA1,      F_BE32, 20, "SHA1" }, sizeof(sha1_ctx), dgshft(sha1), iuf(rhash_sha1), 0 }, /* 160 bit */
	{ { RHASH_TIGER,     F_LE64, 24, "TIGER" }, sizeof(tiger_ctx), dgshft(tiger), iuf(rhash_tiger), 0 }, /* 192 bit */
	{ { RHASH_TTH,       F_BS32, 24, "TTH" }, sizeof(tth_ctx), dgshft2(tth, tiger.hash), iuf(rhash_tth), 0 }, /* 192 bit */
	{ { RHASH_BTIH,      0, 20, "BTIH" }, sizeof(torrent_ctx), dgshft2(torrent, btih), iuf(rhash_torrent), (pcleanup_t)rhash_torrent_cleanup }, /* 160 bit */
	{ { RHASH_ED2K,      F_LE32, 16, "ED2K" }, sizeof(ed2k_ctx), dgshft2(ed2k, md4_context_inner.hash), iuf(rhash_ed2k), 0 }, /* 128 bit */
	{ { RHASH_AICH,      F_BS32, 20, "AICH" }, sizeof(aich_ctx), dgshft2(aich, sha1_context.hash), iuf(rhash_aich), (pcleanup_t)rhash_aich_cleanup }, /* 160 bit */
	{ { RHASH_WHIRLPOOL, F_BE64, 64, "WHIRLPOOL" }, sizeof(whirlpool_ctx), dgshft(whirlpool), iuf(rhash_whirlpool), 0 }, /* 512 bit */
	{ { RHASH_RIPEMD160, F_LE32, 20, "RIPEMD-160" }, sizeof(ripemd160_ctx), dgshft(ripemd160), iuf(rhash_ripemd160), 0 }, /* 160 bit */
	{ { RHASH_GOST,      F_LE32, 32, "GOST" }, sizeof(gost_ctx), dgshft(gost), iuf(rhash_gost), 0 }, /* 256 bit */
	{ { RHASH_GOST_CRYPTOPRO, F_LE32, 32, "GOST-CRYPTOPRO" }, sizeof(gost_ctx), dgshft(gost), ini(rhash_gost_cryptopro), upd(rhash_gost), fin(rhash_gost), 0 }, /* 256 bit */
	{ { RHASH_HAS160,    F_LE32, 20, "HAS-160" }, sizeof(has160_ctx), dgshft(has160), iuf(rhash_has160), 0 }, /* 160 bit */
	{ { RHASH_SNEFRU128, F_BE32, 16, "SNEFRU-128" }, sizeof(snefru_ctx), dgshft(snefru), ini(rhash_snefru128), upd(rhash_snefru), fin(rhash_snefru), 0 }, /* 128 bit */
	{ { RHASH_SNEFRU256, F_BE32, 32, "SNEFRU-256" }, sizeof(snefru_ctx), dgshft(snefru), ini(rhash_snefru256), upd(rhash_snefru), fin(rhash_snefru), 0 }, /* 256 bit */
	{ { RHASH_SHA224,    F_BE32, 28, "SHA-224" }, sizeof(sha256_ctx), dgshft(sha256), ini(rhash_sha224), upd(rhash_sha256), fin(rhash_sha256), 0 }, /* 224 bit */
	{ { RHASH_SHA256,    F_BE32, 32, "SHA-256" }, sizeof(sha256_ctx), dgshft(sha256), iuf(rhash_sha256), 0 },  /* 256 bit */
	{ { RHASH_SHA384,    F_BE64, 48, "SHA-384" }, sizeof(sha512_ctx), dgshft(sha512), ini(rhash_sha384), upd(rhash_sha512), fin(rhash_sha512), 0 }, /* 384 bit */
	{ { RHASH_SHA512,    F_BE64, 64, "SHA-512" }, sizeof(sha512_ctx), dgshft(sha512), iuf(rhash_sha512), 0 },  /* 512 bit */
	{ { RHASH_EDONR256,  F_LE32, 32,  "EDON-R256" }, sizeof(edonr_ctx), dgshft2(edonr, u.data256.hash) + 32, iuf(rhash_edonr256), 0 },  /* 256 bit */
	{ { RHASH_EDONR512,  F_LE64, 64,  "EDON-R512" }, sizeof(edonr_ctx), dgshft2(edonr, u.data512.hash) + 64, iuf(rhash_edonr512), 0 },  /* 512 bit */
};

/**
 * Initialize requested algorithms.
 */
void rhash_init_algorithms(unsigned mask)
{
	(void)mask; /* unused now */

	/* verify that RHASH_HASH_COUNT is the index of the major bit of RHASH_ALL_HASHES */
	assert(1 == (RHASH_ALL_HASHES >> (RHASH_HASH_COUNT - 1)));

#ifdef GENERATE_CRC32_TABLE
	rhash_crc32_init_table();
#endif
#ifdef GENERATE_GOST_LOOKUP_TABLE
	rhash_gost_init_table();
#endif
	rhash_uninitialized_algorithms = 0;
}

/* CRC32 helper functions */

/**
 * Initialize crc32 hash.
 *
 * @param crc32 pointer to the hash to initalize
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