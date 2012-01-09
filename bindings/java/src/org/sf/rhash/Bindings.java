/*
 * This file is a part of Java Bindings for Librhash
 * Copyright (c) 2011-2012, Sergey Basalaev <sbasalaev@gmail.com>
 * Librhash is (c) 2011-2012, Aleksey Kravchenko <rhash.admin@gmail.com>
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
 * Glue to the native API.
 */
final class Bindings {

	/** This class is not instantiable. */
	private Bindings() { }

	/**
	 * Initializes library.
	 */
	static native void rhash_library_init();
	
	/**
	 * Returns the number of supported hash algorithms.
	 */
	static native int rhash_count();

	/**
	 * Computes a hash of the given data.
	 *
	 * @param  hash_id  id of hash function
	 * @param  data     the data to process
	 * @param  ofs      offset in data array from which to start processing
	 * @param  len      data length
	 * @return  pointer to the native digest object
	 */
	static native long rhash_msg(int hash_id, byte[] data, int ofs, int len);
	
	/**
	 * Prints text representation of a given digest.
	 *
	 * @param  rhash  pointer to native digest object
	 * @param  flags  output flags
	 * @return  text representation as byte array
	 */
	static native byte[] rhash_print_bytes(long rhash, int flags);

	/**
	 * Returns magnet link for given hash context and hashing algorithms.
	 * 
	 * @param  rhash     pointer to native digest object
	 * @param  filename  the name of the file to incorporate in magnet
	 * @param  flags     mask of hash_id values
	 * @return  magnet string
	 */
	static native String rhash_print_magnet(long rhash, String filename, int flags);

	/**
	 * Tests whether given default hash algorithm output is base32.
	 * @param  hash_id  id of hash function
	 * @return <code>true</code> if default output for hash algorithm is base32,
	 *         <code>false</code> otherwise
	 */
	static native boolean rhash_is_base32(int hash_id);

	/**
	 * Returns size of binary message digest.
	 * @param hash_id  id of hash function
	 * @return  size of message digest
	 */
	static native int rhash_get_digest_size(int hash_id);

	/**
	 * Creates new hash context.
	 * @param  flags  mask of hash_id values
	 * @return  pointer to the native hash context
	 */
	static native long rhash_init(int flags);

	/**
	 * Updates hash context with given data.
	 * @param rhash  pointer to native hash context
	 * @param data   data to process
	 * @param ofs    index of the first byte to process
	 * @param len    count of bytes to process
	 */
	static native void rhash_update(long rhash, byte[] data, int ofs, int len);

	/**
	 * Finalizes hash context.
	 * @param rhash  pointer to native hash context
	 */
	static native void rhash_final(long rhash);

	/**
	 * Resets hash context.
	 * @param rhash  pointer to native hash context
	 */
	static native void rhash_reset(long rhash);

	/**
	 * Generates message digest for given context and hash_id.
	 * @param  rhash    pointer to native hash context
	 * @param  hash_id  id of hashing algorithm
	 * @return  pointer to native digest
	 */
	static native long rhash_print(long rhash, int hash_id);

	/**
	 * Frees hash context.
	 * @param rhash  pointer to native hash context
	 */
	static native void rhash_free(long rhash);

	/**
	 * Compares two native hash objects.
	 * @param  hash1  pointer to first object
	 * @param  hash2  pointer to second object
	 * @return  <code>true</code> if objects are the same,
	 *          <code>false</code> otherwise
	 */
	static native boolean compareDigests(long hash1, long hash2);

	/**
	 * Computes hashcode for native digest object.
	 * @param hash  pointer to first object
	 * @return  hash code for the object
	 */
	static native int hashcodeForDigest(long hash);

	/**
	 * Frees previously created native digest object.
	 */
	static native void freeDigest(long hash);

	static {
		System.loadLibrary("rhash-jni");
		rhash_library_init();
	}
}

