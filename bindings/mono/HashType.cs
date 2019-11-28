/*
 * This file is a part of Java Bindings for Librhash
 *
 * Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
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

using System;

namespace RHash {

	/*
	 * Type of hashing algorithm.
	 * Supported algorithms are MD4, MD5, SHA1/SHA2, Tiger,
	 * DC++ TTH, BitTorrent BTIH, AICH, EDonkey 2000 hash, GOST R 34.11-94,
	 * RIPEMD-160, HAS-160, EDON-R 256/512, Whirlpool and Snefru-128/256.
	 */
	public enum HashType : uint {
		/* CRC32 checksum. */
		CRC32 = 1,
		/* MD4 hash. */
		MD4 = 1 << 1,
		/* MD5 hash. */
		MD5 = 1 << 2,
		/* SHA-1 hash. */
		SHA1 = 1 << 3,
		/* Tiger hash. */
		TIGER = 1 << 4,
		/* Tiger tree hash */
		TTH = 1 << 5,
		/* BitTorrent info hash. */
		BTIH = 1 << 6,
		/* EDonkey 2000 hash. */
		ED2K = 1 << 7,
		/* eMule AICH. */
		AICH = 1 << 8,
		/* Whirlpool hash. */
		WHIRLPOOL = 1 << 9,
		/* RIPEMD-160 hash. */
		RIPEMD160 = 1 << 10,
		/* GOST R 34.11-94. */
		GOST94 = 1 << 11,
		GOST94_CRYPTOPRO = 1 << 12,
		/* HAS-160 hash. */
		HAS160 = 1 << 13,
		/* GOST R 34.11-2012. */
		GOST12_256 = 1 << 14,
		GOST12_512 = 1 << 15,
		/* SHA-224 hash. */
		SHA224 = 1 << 16,
		/* SHA-256 hash. */
		SHA256 = 1 << 17,
		/* SHA-384 hash. */
		SHA384 = 1 << 18,
		/* SHA-512 hash. */
		SHA512 = 1 << 19,
		/* EDON-R 256. */
		EDONR256 = 1 << 20,
		/* EDON-R 512. */
		EDONR512 = 1 << 21,
		/** SHA3-224 hash. */
		SHA3_224 = 1 << 22,
		/** SHA3-256 hash. */
		SHA3_256 = 1 << 23,
		/** SHA3-384 hash. */
		SHA3_384 = 1 << 24,
		/** SHA3-512 hash. */
		SHA3_512 = 1 << 25,
		/* CRC32C checksum. */
		CRC32C = 1 << 26,
		/* Snefru-128 hash. */
		SNEFRU128 = 1 << 27,
		/* Snefru-256 hash. */
		SNEFRU256 = 1 << 28
	}
}
