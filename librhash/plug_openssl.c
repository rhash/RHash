/* plug_openssl.c - plug-in openssl algorithms
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
#if defined(USE_OPENSSL) || defined(OPENSSL_RUNTIME)

#include "util.h"
#include "plug_openssl.h"
#include <string.h>
#include <assert.h>
#include <openssl/opensslconf.h>

#ifndef OPENSSL_NO_MD4
#  include <openssl/md4.h>
#  define PLUGIN_MD4 RHASH_MD4
#else
#  define PLUGIN_MD4 0
#endif

#ifndef OPENSSL_NO_MD5
#  include <openssl/md5.h>
#  define PLUGIN_MD5 RHASH_MD5
#else
#  define PLUGIN_MD5 0
#endif

#ifndef OPENSSL_NO_SHA
#include <openssl/sha.h>
#  define PLUGIN_SHA1_SHA2 \
	(RHASH_SHA1 | RHASH_SHA224 | RHASH_SHA256 | RHASH_SHA384 | RHASH_SHA512)
#else
#  define PLUGIN_SHA1_SHA2 0
#endif

#ifndef OPENSSL_NO_RIPEMD
#  include <openssl/ripemd.h>
#  define PLUGIN_RIPEMD160 RHASH_RIPEMD160
#else
#  define PLUGIN_RIPEMD160 0
#endif

#ifndef OPENSSL_NO_WHIRLPOOL
#  include <openssl/whrlpool.h>
#  define PLUGIN_WHIRLPOOL RHASH_WHIRLPOOL
#else
#  define PLUGIN_WHIRLPOOL 0
#endif

#if defined(OPENSSL_RUNTIME)
#  if defined(_WIN32) || defined(__CYGWIN__)
#    include <windows.h>
#  else
#    include <dlfcn.h>
#  endif
#endif

#define OPENSSL_DEFAULT_HASH_MASK (PLUGIN_MD5 | PLUGIN_SHA1_SHA2)
#define PLUGIN_SUPPORTED_HASH_MASK \
	(PLUGIN_MD4 | PLUGIN_MD5 | PLUGIN_SHA1_SHA2 | PLUGIN_RIPEMD160 | PLUGIN_WHIRLPOOL)

/* the mask of ids of hashing algorithms to use from the OpenSSL library */
unsigned openssl_enabled_hash_mask = OPENSSL_DEFAULT_HASH_MASK;
unsigned openssl_available_algorithms_hash_mask = 0;

#ifdef OPENSSL_RUNTIME
typedef void (*os_fin_t)(void*, void*);
#  define OS_METHOD(name) os_fin_t p##name##_final = 0
OS_METHOD(MD4);
OS_METHOD(MD5);
OS_METHOD(RIPEMD160);
OS_METHOD(SHA1);
OS_METHOD(SHA224);
OS_METHOD(SHA256);
OS_METHOD(SHA384);
OS_METHOD(SHA512);
OS_METHOD(WHIRLPOOL);

#  define CALL_FINAL(name, result, ctx) p##name##_final(result, ctx)
#  define HASH_INFO_METHODS(name) 0, 0, wrap##name##_Final, 0

#else
/* for load-time linking */
#  define CALL_FINAL(name, result, ctx) name##_Final(result, ctx)
#  define HASH_INFO_METHODS(name) (pinit_t)(void(*)(void))name##_Init, (pupdate_t)(void(*)(void))name##_Update, wrap##name##_Final, 0
#endif


/* The openssl *_Update functions have the same signature as RHash ones:
 * void update_func(void* ctx, const void* msg, size_t size),
 * so we can use them in RHash directly. But the _Final functions
 * have different order of arguments, so we need to wrap them. */
#define WRAP_FINAL(name, CTX_TYPE) \
	static void wrap##name##_Final(void* ctx, unsigned char* result) { \
		CALL_FINAL(name, result, (CTX_TYPE*)ctx); \
	}

#ifndef OPENSSL_NO_MD4
WRAP_FINAL(MD4, MD4_CTX)
#endif
#ifndef OPENSSL_NO_MD5
WRAP_FINAL(MD5, MD5_CTX)
#endif
#ifndef OPENSSL_NO_SHA
WRAP_FINAL(SHA1, SHA_CTX)
WRAP_FINAL(SHA224, SHA256_CTX)
WRAP_FINAL(SHA256, SHA256_CTX)
WRAP_FINAL(SHA384, SHA512_CTX)
WRAP_FINAL(SHA512, SHA512_CTX)
#endif
#ifndef OPENSSL_NO_RIPEMD
WRAP_FINAL(RIPEMD160, RIPEMD160_CTX)
#endif

#ifndef OPENSSL_NO_WHIRLPOOL
/* wrapping WHIRLPOOL_Final requires special attention. */
static void wrapWHIRLPOOL_Final(void* ctx, unsigned char* result)
{
	/* must pass NULL as the result argument, otherwise ctx will be zeroed */
	CALL_FINAL(WHIRLPOOL, NULL, ctx);
	memcpy(result, ((WHIRLPOOL_CTX*)ctx)->H.c, 64);
}

rhash_info info_ossl_whirlpool = { RHASH_WHIRLPOOL, 0, 64, "WHIRLPOOL", "whirlpool" };
#endif

#define NO_HASH_INFO { 0, 0, 0, 0, 0, 0, 0 }

/* The table of supported OpenSSL hash functions */
rhash_hash_info rhash_openssl_hash_info[9] =
{
#ifndef OPENSSL_NO_MD4
	{ &info_md4, sizeof(MD4_CTX), offsetof(MD4_CTX, A), HASH_INFO_METHODS(MD4) }, /* 128 bit */
#else
	NO_HASH_INFO,
#endif
#ifndef OPENSSL_NO_MD5
	{ &info_md5, sizeof(MD5_CTX), offsetof(MD5_CTX, A), HASH_INFO_METHODS(MD5) }, /* 128 bit */
#else
	NO_HASH_INFO,
#endif
#ifndef OPENSSL_NO_SHA
	{ &info_sha1, sizeof(SHA_CTX), offsetof(SHA_CTX, h0),  HASH_INFO_METHODS(SHA1) }, /* 160 bit */
	{ &info_sha224, sizeof(SHA256_CTX), offsetof(SHA256_CTX, h), HASH_INFO_METHODS(SHA224) }, /* 224 bit */
	{ &info_sha256, sizeof(SHA256_CTX), offsetof(SHA256_CTX, h), HASH_INFO_METHODS(SHA256) }, /* 256 bit */
	{ &info_sha384, sizeof(SHA512_CTX), offsetof(SHA512_CTX, h), HASH_INFO_METHODS(SHA384) }, /* 384 bit */
	{ &info_sha512, sizeof(SHA512_CTX), offsetof(SHA512_CTX, h), HASH_INFO_METHODS(SHA512) }, /* 512 bit */
#else
	NO_HASH_INFO, NO_HASH_INFO, NO_HASH_INFO, NO_HASH_INFO, NO_HASH_INFO,
#endif
#ifndef OPENSSL_NO_RIPEMD
	{ &info_rmd160, sizeof(RIPEMD160_CTX), offsetof(RIPEMD160_CTX, A), HASH_INFO_METHODS(RIPEMD160) }, /* 160 bit */
#else
	NO_HASH_INFO,
#endif
#ifndef OPENSSL_NO_WHIRLPOOL
	{ &info_ossl_whirlpool, sizeof(WHIRLPOOL_CTX), offsetof(WHIRLPOOL_CTX, H.c), HASH_INFO_METHODS(WHIRLPOOL) }, /* 512 bit */
#else
	NO_HASH_INFO,
#endif
};

/* The rhash_updated_hash_info static array initialized by rhash_plug_openssl() replaces
 * rhash internal algorithms table. It is kept in an uninitialized-data segment
 * taking no space in the executable. */
rhash_hash_info rhash_updated_hash_info[RHASH_HASH_COUNT];

#ifdef OPENSSL_RUNTIME

#if (defined(_WIN32) || defined(__CYGWIN__)) /* __CYGWIN__ is also defined in MSYS */
# define GET_DLSYM(name) (void(*)(void))GetProcAddress(handle, name)
#else  /* _WIN32 */
# define GET_DLSYM(name) dlsym(handle, name)
#endif /* _WIN32 */
#define LOAD_ADDR(n, name) \
	p##name##_final = (os_fin_t)GET_DLSYM(#name "_Final"); \
	rhash_openssl_hash_info[n].update = (pupdate_t)GET_DLSYM(#name "_Update"); \
	rhash_openssl_hash_info[n].init = (rhash_openssl_hash_info[n].update && p##name##_final ? \
		(pinit_t)GET_DLSYM(#name "_Init") : 0);

/**
 * Load OpenSSL DLL at runtime, store pointers to functions of all
 * supported hash algorithms.
 *
 * @return 1 on success, 0 if the library not found
 */
static int load_openssl_runtime(void)
{
#if defined(_WIN32) || defined(__CYGWIN__)
	HMODULE handle = 0;
	size_t i;

# if defined(_WIN32)
	static const char* libNames[] = {
		"libeay32.dll",
	};
# elif defined(__MSYS__) /*  MSYS also defines __CYGWIN__ */
	static const char* libNames[] = {
		"msys-crypto-1.1.dll",
		"msys-crypto-1.0.0.dll",
	};
# elif defined(__CYGWIN__)
	static const char* libNames[] = {
		"cygcrypto-1.1.dll",
		"cygcrypto-1.0.0.dll",
	};
# endif
	/* suppress the error popup dialogs */
	UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	SetErrorMode(oldErrorMode | SEM_FAILCRITICALERRORS);

	for (i = 0; !handle && i < RHASH_COUNTOF(libNames); i++)
		handle = LoadLibraryA(libNames[i]);

	SetErrorMode(oldErrorMode); /* restore error mode */
#else
	static const char* libNames[] = {
		"libcrypto.so.3",
		"libcrypto.so.1.1",
		"libcrypto.so.1.0.2",
		"libcrypto.so.1.0.0",
		"libcrypto.so.0.9.8",
		"libcrypto.so",
	};
	void* handle = 0;
	size_t i;
	for (i = 0; !handle && i < RHASH_COUNTOF(libNames); i++)
		handle = dlopen(libNames[i], RTLD_NOW);
#endif /* defined(_WIN32) || defined(__CYGWIN__) */

	if (!handle)
		return 0; /* could not load OpenSSL */

#ifndef OPENSSL_NO_MD4
	LOAD_ADDR(0, MD4)
#endif
#ifndef OPENSSL_NO_MD5
	LOAD_ADDR(1, MD5);
#endif
#ifndef OPENSSL_NO_SHA
	LOAD_ADDR(2, SHA1);
	LOAD_ADDR(3, SHA224);
	LOAD_ADDR(4, SHA256);
	LOAD_ADDR(5, SHA384);
	LOAD_ADDR(6, SHA512);
#endif
#ifndef OPENSSL_NO_RIPEMD
	LOAD_ADDR(7, RIPEMD160);
#endif
#ifndef OPENSSL_NO_WHIRLPOOL
	LOAD_ADDR(8, WHIRLPOOL);
#endif
	return 1;
}
#endif /* OPENSSL_RUNTIME */

/**
 * Replace several RHash internal algorithms with the OpenSSL ones.
 * It can replace MD4/MD5, SHA1/SHA2, RIPEMD, WHIRLPOOL.
 *
 * @return 1 on success, 0 if OpenSSL library not found
 */
int rhash_plug_openssl(void)
{
	size_t i;
	unsigned bit_index;

	assert(rhash_info_size <= RHASH_HASH_COUNT); /* buffer-overflow protection */

	if ((openssl_enabled_hash_mask & PLUGIN_SUPPORTED_HASH_MASK) == 0)
		return 1; /* do not load OpenSSL */

#ifdef OPENSSL_RUNTIME
	if (!load_openssl_runtime())
		return 0;
#endif

	memcpy(rhash_updated_hash_info, rhash_info_table, sizeof(rhash_updated_hash_info));

	/* replace internal rhash methods with the OpenSSL ones */
	for (i = 0; i < (int)RHASH_COUNTOF(rhash_openssl_hash_info); i++)
	{
		rhash_hash_info* method = &rhash_openssl_hash_info[i];
		if (!method->init)
			continue;
		openssl_available_algorithms_hash_mask |= method->info->hash_id;
		if ((openssl_enabled_hash_mask & method->info->hash_id) == 0)
			continue;
		bit_index = rhash_ctz(method->info->hash_id);
		assert(method->info->hash_id == rhash_updated_hash_info[bit_index].info->hash_id);
		memcpy(&rhash_updated_hash_info[bit_index], method, sizeof(rhash_hash_info));
	}

	rhash_info_table = rhash_updated_hash_info;
	return 1;
}

/**
 * Returns bit-mask of OpenSSL algorithms supported by the plugin.
 *
 * @return the bit-mask of available OpenSSL algorithms
 */
unsigned rhash_get_openssl_supported_hash_mask(void)
{
	return PLUGIN_SUPPORTED_HASH_MASK;
}

/**
 * Returns bit-mask of available OpenSSL algorithms, if the OpenSSL has
 * been successfully loaded, zero otherwise. Only supported by the plugin
 * algorithms are listed.
 *
 * @return the bit-mask of available OpenSSL algorithms
 */
unsigned rhash_get_openssl_available_hash_mask(void)
{
	return openssl_available_algorithms_hash_mask;
}

/**
 * Returns bit-mask of enabled OpenSSL algorithms, if the OpenSSL has
 * been successfully loaded, zero otherwise.
 * Only available algorithms are listed.
 *
 * @return the bit-mask of enabled OpenSSL algorithms
 */
unsigned rhash_get_openssl_enabled_hash_mask(void)
{
	return openssl_enabled_hash_mask & openssl_available_algorithms_hash_mask;
}

/**
 * Set bit-mask of enabled OpenSSL algorithms.
 *
 * @return the bit-mask of enabled OpenSSL algorithms
 */
void rhash_set_openssl_enabled_hash_mask(unsigned mask)
{
	mask &= (openssl_available_algorithms_hash_mask ?
		openssl_available_algorithms_hash_mask :
		PLUGIN_SUPPORTED_HASH_MASK);
	openssl_enabled_hash_mask = mask;
}
#else
typedef int dummy_declaration_required_by_strict_iso_c;
#endif /* defined(USE_OPENSSL) || defined(OPENSSL_RUNTIME) */
