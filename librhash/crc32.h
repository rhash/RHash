/* crc32.h */
#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus
extern "C" {
#endif

/* hash functions */

#ifndef DISABLE_CRC32
unsigned rhash_get_crc32(unsigned crcinit, const unsigned char* msg, size_t size);
#endif

#ifndef DISABLE_CRC32C
unsigned rhash_get_crc32c(unsigned crcinit, const unsigned char* msg, size_t size);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CRC32_H */
