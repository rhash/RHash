/* sha_ni.h */
#ifndef SHA_NI_H
#define SHA_NI_H

#if defined(RHASH_SSE4_SHANI) && !defined(RHASH_DISABLE_SHANI)
# if defined(__GNUC__)
#  define RHASH_GCC_SHANI
/* SHA extensions are supported by Visual Studio >= 2015 */
# elif (_MSC_VER >= 1900)
#  define RHASH_MSVC_SHANI
# else
#  define RHASH_DISABLE_SHANI
# endif
#endif

#if defined(RHASH_SSE4_SHANI) && !defined(RHASH_DISABLE_SHANI)
#include "sha1.h"
#include "sha256.h"

#ifdef __cplusplus
extern "C" {
#endif

void rhash_sha1_ni_update(sha1_ctx* ctx, const unsigned char* msg, size_t size);
void rhash_sha1_ni_final(sha1_ctx* ctx, unsigned char* result);
void rhash_sha256_ni_update(sha256_ctx* ctx, const unsigned char* data, size_t length);
void rhash_sha256_ni_final(sha256_ctx* ctx, unsigned char* result);
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_SSE4_SHANI */
#endif /* SHA_NI_H */
