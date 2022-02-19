/* plug_openssl.h - plug-in openssl algorithms */
#ifndef RHASH_PLUG_OPENSSL_H
#define RHASH_PLUG_OPENSSL_H
#if defined(USE_OPENSSL) || defined(OPENSSL_RUNTIME)

#include "algorithms.h"

#ifdef __cplusplus
extern "C" {
#endif

int rhash_plug_openssl(void); /* load openssl algorithms */
unsigned rhash_get_openssl_supported_hash_mask(void);
unsigned rhash_get_openssl_available_hash_mask(void);

extern unsigned rhash_openssl_hash_mask; /* mask of hash sums to use */
extern rhash_hash_info rhash_openssl_hash_info[9];
#define rhash_ossl_sha1_init() (rhash_openssl_hash_info[2].init)
#define rhash_ossl_sha1_update() (rhash_openssl_hash_info[2].update)
#define rhash_ossl_sha1_final() (rhash_openssl_hash_info[2].final)

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#else
# define rhash_get_openssl_supported_hash_mask() (0)
# define rhash_get_openssl_available_hash_mask() (0)
#endif /* defined(USE_OPENSSL) || defined(OPENSSL_RUNTIME) */
#endif /* RHASH_PLUG_OPENSSL_H */
