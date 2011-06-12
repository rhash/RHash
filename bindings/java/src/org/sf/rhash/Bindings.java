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

import java.io.IOException;

/**
 * Glue to the native API.
 */
final class Bindings {

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
	 * @param  message  the data to process
	 * @param  ofs      offset in data array from which to start processing
	 * @param  len      data length
	 * @return  pointer to native hash object
	 */
	static native long rhash_msg(int hash_id, byte[] message, int ofs, int len);
	
	/**
	 * Compute a single hash for given file.
	 *
	 * @param  hash_id   id of hash function
	 * @param  filepath  path to the file
	 * @return  pointer to native hash object
	 * @throws IOException  if an I/O error occurs
	 */
	static native long rhash_file(int hash_id, String filepath) throws IOException;

	/**
	 * Print text representation of a given hash sum.
	 *
	 * @param  hash   pointer to native hash object
	 * @param  flags  output flags
	 * @return  text representation as byte array
	 */
	static native byte[] rhash_print_bytes(long hash, int flags);

	/**
	 * Frees previously created native hash object.
	 */
	static native void freeHashObject(long hash);

	/**
	 * Creates new copy of native hash object.
	 */
	static native long cloneHashObject(long hash);

	/**
	 * Tests whether given default hash algorithm output is base32.
	 * @param  hash_id  id of hash function
	 * @return <code>true</code> if default output for hash algorithm is base32,
	 *         <code>false</code> otherwise
	 */
	static native boolean rhash_is_base32(int hash_id);

	/**
	 * Compares two native hash objects.
	 * @param  hash1  pointer to first object
	 * @param  hash2  pointer to second object
	 * @return  <code>true</code> if objects are the same,
	 *          <code>false</code> otherwise
	 */
	static native boolean compareHashObjects(long hash1, long hash2);

	/**
	 * Computes hashcode for native hash object.
	 * @param hash  pointer to first object
	 * @return  hash code for the object
	 */
	static native int hashcodeForHashObject(long hash);

	static {
		System.loadLibrary("rhash-jni");
		rhash_library_init();
	}
}

