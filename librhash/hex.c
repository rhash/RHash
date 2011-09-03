/* hex.c - conversion for hexadecimal and base32 strings.
 *
 * Copyright: 2008 Alexey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 */
#include "hex.h"

/**
 * Store hexadecimal representation of a byte to given buffer.
 *
 * @param dst  the buffer to receive two symbols of hex representation
 * @param byte the byte to decode
 * @param upper_case flag to print string in uppercase
 * @return pointer to the next char in buffer (dst+2)
 */
char* rhash_print_hex_byte(char *dst, const unsigned char byte, int upper_case)
{
	const char add = (upper_case ? 'A'-10 : 'a'-10);
	unsigned char c = (byte >> 4) & 15;
	*dst++ = (c>9 ? c+add : c+'0');
	c = byte & 15;
	*dst++ = (c>9 ? c+add : c+'0');
	return dst;
}

/**
 * Store hexadecimal representation of a binary string to given buffer.
 *
 * @param dst the buffer to receive hexadecimal representation
 * @param str binary string
 * @param len string length
 * @param upper_case flag to print string in uppercase
 */
void rhash_byte_to_hex(char *dst, const unsigned char *src, unsigned len, int upper_case)
{
	while(len-- > 0) {
		dst = rhash_print_hex_byte(dst, *src++, upper_case);
	}
	*dst = '\0';
}

/**
 * Convert a hexadecimal string to a string of bytes.
 *
 * @param str string to parse
 * @param bin result
 * @param len string length
 */
void rhash_hex_to_byte(const char* str, unsigned char* bin, int len)
{
	/* parse the highest hexadecimal digit */
	if((len & 1) != 0) {
		*(bin++) = HEX2DIGIT(*(str++));
		len--;
	}

	/* parse the rest - an even-sized hexadecimal string */
	for(; len >= 2; len -= 2, str += 2) {
		*(bin++) = (HEX2DIGIT(str[0]) << 4) | HEX2DIGIT(str[1]);
	}
}

/**
 * Encode a binary string to base32.
 *
 * @param dst the buffer to store result
 * @param str binary string
 * @param len string length
 * @param upper_case flag to print string in uppercase
 */
void rhash_byte_to_base32(char* dest, const unsigned char* src, unsigned len, int upper_case)
{
	const char a = (upper_case ? 'A' : 'a');
	unsigned shift = 0;
	unsigned char word;
	const unsigned char* e = src + len;
	while(src < e) {
		if(shift > 3) {
			word = (*src & (0xFF >> shift));
			shift = (shift + 5) % 8;
			word <<= shift;
			if(src + 1 < e)
				word |= *(src + 1) >> (8 - shift);
			++src;
		} else {
			shift = (shift + 5) % 8;
			word = ( *src >> ( (8 - shift)&7 ) ) & 0x1F;
			if(shift == 0) src++;
		}
		*dest++ = ( word < 26 ? word + a : word + '2' - 26 );
	}
	*dest = '\0';
}

/**
 * Parse given base32 string and store result to bin.
 *
 * @param str string to parse
 * @param bin result
 * @param len string length
 */
void rhash_base32_to_byte(const char* str, unsigned char* bin, int len)
{
	const char* e = str + len;
	unsigned shift = 0;
	unsigned char b;
	for(; str<e; str++) {
		b = BASE32_TO_DIGIT(*str);
		shift = (shift + 5) % 8;
		if(shift < 5) {
			*bin++ |= (b >> shift);
		}
		*bin |= b << (8 - shift);
	}
}

/**
 * Encode a binary string to base64.
 * Encoded output length is always a multiple of 4 bytes.
 *
 * @param dst the buffer to store result
 * @param str binary string
 * @param len string length
 */
void rhash_byte_to_base64(char* dest, const unsigned char* src, unsigned len)
{
	static const char* tail = "0123456789+/";
	unsigned shift = 0;
	unsigned char word;
	const unsigned char* e = src + len;
	while(src < e) {
		if(shift > 2) {
			word = (*src & (0xFF >> shift));
			shift = (shift + 6) % 8;
			word <<= shift;
			if(src + 1 < e)
				word |= *(src + 1) >> (8 - shift);
			++src;
		} else {
			shift = (shift + 6) % 8;
			word = ( *src >> ( (8 - shift) & 7 ) ) & 0x3F;
			if(shift == 0) src++;
		}
		*dest++ = ( word < 52 ? (word < 26 ? word + 'A' : word - 26 + 'a') : tail[word - 52]);
	}
	if(shift > 0) {
		*dest++ = '=';
		if(shift == 4) *dest++ = '=';
	}
	*dest = '\0';
}
