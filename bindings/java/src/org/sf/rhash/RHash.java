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

import java.io.File;
import java.io.IOException;

/**
 * Utility class to compute hashes.
 */
public final class RHash {
	
	/** This class is not instantiable. */
	private RHash() { }

	/**
	 * Computes hash of given data.
	 * @param  type  type of hash algorithm
	 * @param  data  the data to process
	 * @param  ofs   start offset in data array
	 * @param  len   count of bytes to process
	 * @return  data hash as a string
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 */
	static public Hash computeHash(HashType type, byte[] data, int ofs, int len) {
		if (type == null || data == null) throw new NullPointerException();
		return new Hash(Bindings.rhash_msg(type.hashId(), data, ofs, len), type);
	}
	
	/**
	 * Computes hash of given data.
	 * @param  type  type of hash algorithm
	 * @param  data  the data to process
	 * @return  data hash
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 */
	static public Hash computeHash(HashType type, byte[] data) {
		return computeHash(type, data, 0, data.length);
	}

	/**
	 * Computes hash of given string.
	 * @param  type  type of hash algorithm
	 * @param  str   the string to process
	 * @return  data hash
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 */
	static public Hash computeHash(HashType type, String str) {
		if (type == null || str == null) throw new NullPointerException();
		return computeHash(type, str.getBytes());
	}
	
	/**
	 * Computes hash of given string.
	 * @param  type  type of hash algorithm
	 * @param  file  the file to process
	 * @return  data hash
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 * @throws IOException
	 *   if an I/O error occurs while hashing
	 */
	static public Hash computeHash(HashType type, File file) throws IOException {
		if (type == null || file == null) throw new NullPointerException();
		return new Hash(Bindings.rhash_file(type.hashId(), file.toString()), type);
	}
}
