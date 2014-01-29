/*
 * This file is a part of Java Bindings for Librhash
 * Copyright (c) 2011-2012, 2014, Sergey Basalaev <sbasalaev@gmail.com>
 * Librhash is (c) 2011-2014, Aleksey Kravchenko <rhash.admin@gmail.com>
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
 * Supported algorithms are MD4, MD5, SHA1/SHA2, Tiger,
 * DC++ TTH, BitTorrent BTIH, AICH, EDonkey 2000 hash, GOST R 34.11-94,
 * RIPEMD-160, HAS-160, EDON-R 256/512, Whirlpool and Snefru-128/256.
 */
public enum HashType {
	/** CRC32 checksum. */
	CRC32(1),
	/** MD4 hash. */
	MD4(1 << 1),
	/** MD5 hash. */
	MD5(1 << 2),
	/** SHA-1 hash. */
	SHA1(1 << 3),
	/** Tiger hash. */
	TIGER(1 << 4),
	/** Tiger tree hash */
	TTH(1 << 5),
	/** BitTorrent info hash. */
	BTIH(1 << 6),
	/** EDonkey 2000 hash. */
	ED2K(1 << 7),
	/** eMule AICH. */
	AICH(1 << 8),
	/** Whirlpool hash. */
	WHIRLPOOL(1 << 9),
	/** RIPEMD-160 hash. */
	RIPEMD160(1 << 10),
	/** GOST R 34.11-94. */
	GOST(1 << 11),
	GOST_CRYPTOPRO(1 << 12),
	/** HAS-160 hash. */
	HAS160(1 << 13),
	/** Snefru-128 hash. */
	SNEFRU128(1 << 14),
	/** Snefru-256 hash. */
	SNEFRU256(1 << 15),
	/** SHA-224 hash. */
	SHA224(1 << 16),
	/** SHA-256 hash. */
	SHA256(1 << 17),
	/** SHA-384 hash. */
	SHA384(1 << 18),
	/** SHA-512 hash. */
	SHA512(1 << 19),
	/** EDON-R 256. */
	EDONR256(1 << 20),
	/** EDON-R 512. */
	EDONR512(1 << 21),
	/** SHA3-224 hash. */
	SHA3_224(1 << 22),
	/** SHA3-256 hash. */
	SHA3_256(1 << 23),
	/** SHA3-384 hash. */
	SHA3_384(1 << 24),
	/** SHA3-512 hash. */
	SHA3_512(1 << 25);

	/** hash_id for the native API */
    private int hashId;

    /**
     * Construct HashType for specified native hash_id
     * @param hashId hash identifier for native API
     */
    private HashType(int hashId) {
        this.hashId = hashId;
    }

	/**
	 * Returns hash_id for the native API.
     * @return hash identifier
     */
	int hashId() {
	    return hashId;
	}

	/**
	 * Returns lowest <code>HashType</code> for given id mask.
	 * Used by <code>RHash.getDigest()</code>.
	 * @param flags  mask of hash identifiers
	 * @return  <code>HashType</code> object or <code>null</code>
	 *   if no <code>HashType</code> for given mask exists
	 */
	static HashType forHashFlags(int flags) {
		if (flags == 0) return null;
		int lowest = 0;
		while ((flags % 2) == 0) {
			flags >>>= 1;
			lowest++;
		}
		for (HashType t : HashType.values()) {
			if (t.hashId == (1 << lowest)) return t;
		}
		return null;
	}

	/**
	 * Returns size of binary digest for this type.
	 * @return size of binary digest, in bytes
	 */
	public int getDigestSize() {
		//TODO: rhash_get_digest_size
		return -1;
	}
}
