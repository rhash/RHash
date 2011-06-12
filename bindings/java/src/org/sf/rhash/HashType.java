/*
 * This file is a part of Java Bindings for Librhash
 * Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
 * Librhash is (c) 2011, Alexey S Kravchenko <rhash.admin@gmail.com>
 * 
 * Permission is hereby granted, free of charge,  to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction,  including without limitation the rights
 * to  use,  copy,  modify,  merge, publish, distribute, sublicense, and/or sell
 * copies  of  the Software,  and  to permit  persons  to whom  the Software  is
 * furnished to do so.
 * 
 * This library  is distributed  in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. Use it at your own risk!
 */

package org.sf.rhash;

/**
 * Type of hashing algorithm.
 * MD4, MD5, SHA1/SHA2, Tiger, DC++ TTH, BitTorrent BTIH, AICH, EDonkey 2000 хэш, GOST R 34.11-94, RIPEMD-160, HAS-160, EDON-R 256/512, Whirlpool и Snefru-128/256.
 */
public enum HashType {

	/** CRC32 checksum. */
	CRC32,
	/** MD4 hash. */
	MD4,
	/** MD5 hash. */
	MD5,
	/** SHA-1 hash. */
	SHA1,
	/** Tiger hash. */
	TIGER,
	/** Tiger tree hash */
	TTH,
	/** BitTorrent info hash. */
	BTIH,
	/** EDonkey 2000 hash. */
	ED2K,
	/** eMule AICH. */
	AICH,
	/** Whirlpool hash. */
	WHIRLPOOL,
	/** RIPEMD-160 hash. */
	RIPEMD160,
	/** GOST R 34.11-94. */
	GOST,
	GOST_CRYPTOPRO,
	/** HAS-160 hash. */
	HAS160,
	/** Snefru-128 hash. */
	SNEFRU128,
	/** Snefru-256 hash. */
	SNEFRU256,
	/** SHA-224 hash. */
	SHA224,
	/** SHA-256 hash. */
	SHA256,
	/** SHA-384 hash. */
	SHA384,
	/** SHA-512 hash. */
	SHA512,
	/** EDON-R 256. */
	EDONR256,
	/** EDON-R 512. */
	EDONR512;
	
	/**
	 * Returns hash_id for the native API.
	 */
	int hashId() {
		switch (this) {
			case CRC32:
				return 1;
			case MD4:
				return 1 << 1;
			case MD5:
				return 1 << 2;
			case SHA1:
				return 1 << 3;
			case TIGER:
				return 1 << 4;
			case TTH:
				return 1 << 5;
			case BTIH:
				return 1 << 6;
			case ED2K:
				return 1 << 7;
			case AICH:
				return 1 << 8;
			case WHIRLPOOL:
				return 1 << 9;
			case RIPEMD160:
				return 1 << 10;
			case GOST:
				return 1 << 11;
			case GOST_CRYPTOPRO:
				return 1 << 12;
			case HAS160:
				return 1 << 13;
			case SNEFRU128:
				return 1 << 14;
			case SNEFRU256:
				return 1 << 15;
			case SHA224:
				return 1 << 16;
			case SHA256:
				return 1 << 17;
			case SHA384:
				return 1 << 18;
			case SHA512:
				return 1 << 19;
			case EDONR256:
				return 1 << 20;
			case EDONR512:
				return 1 << 21;
			default:
				return 0;
		}
	}
}
