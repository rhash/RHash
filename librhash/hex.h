/* hex.h */
#ifndef HEX_H
#define HEX_H

#ifdef __cplusplus
extern "C" {
#endif

void rhash_byte_to_hex(char *dst, const unsigned char *src, unsigned len, int upper_case);
void rhash_byte_to_base32(char* dest, const unsigned char* src, unsigned len, int upper_case);
void rhash_byte_to_base64(char* dest, const unsigned char* src, unsigned len);
char* rhash_print_hex_byte(char *dst, const unsigned char byte, int upper_case);

/* note: IS_HEX() is defined on ASCII-8 while isxdigit() only when isascii()==true */
#define IS_HEX(c) ((c) <= '9' ? (c) >= '0' : (unsigned)(((c) - 'A') & ~0x20) <= ('F' - 'A' + 0U))
#define IS_BASE32(c) (((c) <= '7' ? ('2' <= (c)) : (unsigned)(((c) - 'A') & ~0x20) <= ('Z' - 'A' + 0U)))

#define HEX2DIGIT(c) ((c) <= '9' ? (c) & 0xF : ((c) - 'a' + 10) & 0xF)
#define BASE32_TO_DIGIT(c) ((c) < 'A' ? (c) - '2' + 26 : ((c) & ~0x20) - 'A')
#define BASE32_LENGTH(bytes) (((bytes) * 8 + 4) / 5)
#define BASE64_LENGTH(bytes) ((((bytes) + 2) / 3) * 4)

void rhash_base32_to_byte(const char* str, unsigned char* bin, int len);
void rhash_hex_to_byte(const char* str, unsigned char* bin, int len);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HEX_H */
