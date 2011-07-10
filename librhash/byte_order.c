/* byte_order.c - byte order related platform dependent routines,
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

#include <unistd.h>
#include <stdlib.h> /* size_t for vc6.0 */
#include "byte_order.h"

#if !(__GNUC__ >= 4 || (__GNUC__ ==3 && __GNUC_MINOR__ >= 4)) /* if !GCC or GCC < 4.3 */

#  if _MSC_VER >= 1300 && (_M_IX86 || _M_AMD64 || _M_IA64) /* if MSVC++ >= 2002 on x86/x64 */
#  pragma intrinsic(_BitScanForward)

/**
 * Returns index of the traling bit of x.
 *
 * @param x the number to process
 * @return zero-based index of the trailing bit
 */
unsigned rhash_ctz(unsigned x)
{
	unsigned long index;
	unsigned char isNonzero = _BitScanForward(&index, x); /* use MS VC intrisic */
	return (isNonzero ? (unsigned)index : 0);
}
#  else /* _MSC_VER >= 1300... */

/**
 * Count number of non-zero bits in x, where x is in 0b0..01..11 form.
 * For details see
 * http://stackoverflow.com/questions/355967/how-to-use-msvc-intrinsics-to-get-the-equivalent-of-this-gcc-code
 *
 * @param x the number in which the number of unities is counted
 * @return number of non-zero bits
 */
static inline unsigned popcnt(unsigned x)
{
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return (x & 0x3f);
}

/**
 * Returns index of the traling bit of x.
 * This is a plain C equivalent for GCC __builtin_ctz() bit scan.
 *
 * @param x the number to process
 * @return zero-based index of the trailing bit
 */
unsigned rhash_ctz(unsigned x)
{
	return popcnt((x & -(int)x) - 1);
}
#  endif /* _MSC_VER >= 1300... */
#endif /* !(GCC >= 4.3) */

/**
 * Copy a memory block with simultanious exchanging byte order.
 * The byte order is changed from little-endian 32-bit integers
 * to big-endian (or vice-versa).
 *
 * @param to the pointer where to copy memory block
 * @param from the source block to copy
 * @param length length of the memory block
 */
void rhash_u32_swap_copy(void* to, int index, const void* from, size_t length)
{
	/* if all pointers and length are 32-bits aligned */
	if( 0 == (( (int)((char*)to - (char*)0) | ((char*)from - (char*)0) | index | length ) & 3) ) {
		/* copy memory as dwords */
		const uint32_t* src = (const uint32_t*)from;
		const uint32_t* end = (const uint32_t*)((const char*)src + length);
		uint32_t* dst = (uint32_t*)((char*)to + index);
		while(src < end) *(dst++) = bswap_32( *(src++) );
	} else {
		const char* src = (const char*)from;
		for(length += index; (size_t)index < length; index++) ((char*)to)[index ^ 3] = *(src++);
	}
}

/**
 * Copy a memory block with simultanious exchanging byte order.
 * The byte order is changed from little-endian 64-bit integers
 * to big-endian (or vice-versa).
 *
 * @param to     the pointer where to copy memory block
 * @param from   the source block to copy
 * @param length length of the memory block
 */
void rhash_u64_swap_copy(void* to, int index, const void* from, size_t length)
{
	/* if all pointers and length are 64-bits aligned */
	if( 0 == (( (int)((char*)to - (char*)0) | ((char*)from - (char*)0) | index | length ) & 7) ) {
		/* copy aligned memory block as 64-bit integers */
		const uint64_t* src = (const uint64_t*)from;
		const uint64_t* end = (const uint64_t*)((const char*)src + length);
		uint64_t* dst = (uint64_t*)((char*)to + index);
		while(src < end) *(dst++) = bswap_64( *(src++) );
	} else {
		const char* src = (const char*)from;
		for(length += index; (size_t)index < length; index++) ((char*)to)[index ^ 7] = *(src++);
	}
}

/**
 * Exchange byte order in the given array of 32-bit integers.
 *
 * @param arr    the array to process
 * @param length array length
 */
void rhash_u32_memswap(unsigned *arr, int length)
{
	unsigned* end = arr + length;
	for(; arr<end; arr++) {
		*arr = bswap_32(*arr);
	}
}
