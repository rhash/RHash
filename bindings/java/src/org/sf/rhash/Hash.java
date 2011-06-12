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
 * Hash sum.
 */
public final class Hash extends Pointer implements Cloneable {

	static final int RAW = 0x1;
	static final int HEX = 0x2;
	static final int BASE32 = 0x3;
	static final int BASE64 = 0x4;
	static final int UPPERCASE = 0x8;
	static final int REVERSE   = 0x10;

	private final HashType type;

	/**
	 * Creates new Hash object.
	 * @param ptr   pointer to the native object
	 * @param type  hash type
	 */
	Hash(long ptr, HashType type) {
		super(ptr);
		this.type = type;
	}
	
	/**
	 * Returns type of hashing algorithm that produced
	 * this hash sum.
	 * 
	 * @return type of hashing algorithm
	 */
	public HashType hashType() {
		return type;
	}

	/**
	 * Returns value of this hash sum as raw bytes.
	 * This method allocates new byte array, modifying it
	 * has no effect on this <code>Hash</code> object.
	 *
	 * @return  value of this hash sum as raw bytes
	 * @see #hex()
	 * @see #base32()
	 * @see #base64() 
	 */
	public byte[] raw() {
		return Bindings.rhash_print_bytes(getAddr(), RAW);
	}

	/**
	 * Returns value of this hash sum as hexadecimal string.
	 *
	 * @return value of the hash sum as hexadecimal string
	 * @see #raw()
	 * @see #base32()
	 * @see #base64() 
	 */
	public String hex() {
		return new String(Bindings.rhash_print_bytes(getAddr(), HEX));
	}

	/**
	 * Returns value of this hash sum as base32 string.
	 *
	 * @return value of the hash sum as base32 string
	 * @see #raw()
	 * @see #hex()
	 * @see #base64()
	 */
	public String base32() {
		return new String(Bindings.rhash_print_bytes(getAddr(), BASE32));
	}

	/**
	 * Returns value of this hash sum as base64 string.
	 *
	 * @return value of the hash sum as base64 string
	 * @see #raw()
	 * @see #hex()
	 * @see #base32()
	 */
	public String base64() {
		return new String(Bindings.rhash_print_bytes(getAddr(), BASE64));
	}
	
	/**
	 * Called by garbage collector to free native resources.
	 */
	@Override
	protected void finalize() {
		Bindings.freeHashObject(getAddr());
	}

	/**
	 * Returns string representation of this object.
	 * If default output for hashing algorithm is base32 then
	 * returned value is the same as if <code>base32()</code>
	 * method was called; otherwise value is the same as returned
	 * by <code>hex()</code> method.
	 *
	 * @return string representation of this object
	 * @see #base32()
	 * @see #hex() 
	 */
	@Override
	public String toString() {
		return (Bindings.rhash_is_base32(type.hashId())) ? base32() : hex();
	}

	/**
	 * Creates verbatim copy of this object.
	 * @return  copy of this object
	 */
	@Override
	public Hash clone() {
		return new Hash(Bindings.cloneHashObject(getAddr()), type);
	}

	/**
	 * Tests whether this object equals to another one
	 * @param  obj  object to compare to
	 * @return
	 *   <code>true</code> if <code>obj</code> is <code>Hash</code>
	 *   instance with the same <code>HashType</code> and value;
	 *   otherwise <code>false</code>
	 */
	@Override
	public boolean equals(Object obj) {
		if (!(obj instanceof Hash)) return false;
		final Hash other = (Hash)obj;
		if (!this.hashType().equals(other.hashType())) return false;
		if (this.getAddr() == other.getAddr()) return true;
		return Bindings.compareHashObjects(this.getAddr(), other.getAddr());
	}

	/**
	 * Returns hash code for this object.
	 * @return hash code for the object
	 */
	@Override
	public int hashCode() {
		return Bindings.hashcodeForHashObject(getAddr());
	}
}

