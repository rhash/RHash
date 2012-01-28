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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.util.Set;

/**
 * Incremental hasher.
 * This class allows you to do incremental hashing for set
 * of hashing algorithms.
 * <p>
 * To do hashing <code>RHash</code> instance is first created
 * and then filled with message chunks using <code>update()</code>
 * methods. Finally, <code>finish()</code> should be called to end
 * all calculations and generate digests, which then can be obtained
 * with <code>getDigest()</code> method. Note, that trying to update
 * finished <code>RHash</code> has no effect other than throwing
 * <code>IllegalStateException</code> though you can reuse this class
 * by calling <code>reset()</code> method, returning it to the state
 * which was immediately after creating.
 * </p><p>
 * To quickly produce message digest for a single message/file
 * and a single algorithm you may use convenience methods
 * <code>RHash.computeHash()</code>.
 * </p><p>
 * This class is thread safe.
 * </p>
 */
public final class RHash {

	/* == EXCEPTION MESSAGES == */

	static private final String ERR_FINISHED   = "RHash is finished, data update is not possible";
	static private final String ERR_NOHASH     = "No HashTypes specified";
	static private final String ERR_UNFINISHED = "RHash should be finished before generating Digest";
	static private final String ERR_WRONGTYPE  = "RHash was not created to generate Digest for ";

	/**
	 * Computes hash of given range in data.
	 * This method calculates message digest for byte subsequence
	 * in array <code>data</code> starting from <code>data[ofs]</code>
	 * and ending at <code>data[ofs+len-1]</code>.
	 * 
	 * @param  type  type of hash algorithm
	 * @param  data  the bytes to process
	 * @param  ofs   index of the first byte in array to process
	 * @param  len   count of bytes to process
	 * @return  message digest for specified subarray
	 * @throws NullPointerException
	 *   if either <code>type</code> or <code>data</code>
	 *   is <code>null</code>
	 * @throws IndexOutOfBoundsException
	 *   if <code>ofs &lt; 0</code>, <code>len &lt; 0</code> or
	 *   <code>ofs+len &gt; data.length</code>
	 */
	static public Digest computeHash(HashType type, byte[] data, int ofs, int len) {
		if (type == null || data == null) {
			throw new NullPointerException();
		}
		if (ofs < 0 || len < 0 || ofs+len > data.length) {
			throw new IndexOutOfBoundsException();
		}
		return new Digest(Bindings.rhash_msg(type.hashId(), data, ofs, len), type);
	}
	
	/**
	 * Computes hash of given data.
	 * 
	 * @param  type  type of hash algorithm
	 * @param  data  the bytes to process
	 * @return  message digest for specified array
	 * @throws NullPointerException
	 *   if either <code>type</code> or <code>data</code>
	 *   is <code>null</code>
	 */
	static public Digest computeHash(HashType type, byte[] data) {
		return computeHash(type, data, 0, data.length);
	}

	/**
	 * Computes hash of given string.
	 * String is encoded into a sequence of bytes
	 * using the specified charset.
	 *
	 * @param  type      type of hash algorithm
	 * @param  str       the string to process
	 * @param  encoding  encoding to use
	 * @return  message digest for specified string
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 * @throws UnsupportedEncodingException if specified encoding is not supported
	 */
	static public Digest computeHash(HashType type, String str, String encoding)
			throws UnsupportedEncodingException {
		if (type == null || str == null || encoding == null) {
			throw new NullPointerException();
		}
		return computeHash(type, str.getBytes(encoding));
	}

	/**
	 * Computes hash of given string.
	 * String is encoded into a sequence of bytes using the
	 * default platform encoding.
	 *
	 * @param  type  type of hash algorithm
	 * @param  str   the string to process
	 * @return  message digest for specified string
	 * @throws NullPointerException if any of arguments is <code>null</code>
	 */
	static public Digest computeHash(HashType type, String str) {
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
	static public Digest computeHash(HashType type, File file) throws IOException {
		if (type == null || file == null) {
			throw new NullPointerException();
		}
		RHash hasher = new RHash(type);
		hasher.update(file).finish();
		return hasher.getDigest();
	}
	
	/**
	 * Produces magnet link for specified file with given hashes.
	 *
	 * @param  filename   the file to generate magnet for
	 * @param  types      types of hashing algorithms
	 */
	static public String getMagnetFor(String filename, HashType... types) throws IOException {
		RHash hasher = new RHash(types);
		hasher.update(new File(filename)).finish();
		return hasher.getMagnet(filename);
	}
	
	/**
	 * Produces magnet link for specified file with given hashes.
	 *
	 * @param  filename   the file to generate magnet for
	 * @param  types      set of hashing types
	 */
	static public String getMagnetFor(String filename, Set<HashType> types) throws IOException {
		RHash hasher = new RHash(types);
		hasher.update(new File(filename)).finish();
		return hasher.getMagnet(filename);
	}

	/** Indicates whether this <code>RHash</code> is finished. */
	private boolean finished = false;

	/** Mask of hash_id values. */
	private final int hash_flags;

	/** Pointer to the native hash context. */
	private final long context_ptr;

	/** Default hash type used in <code>getDigest()</code>. */
	private final HashType deftype;

	/**
	 * Creates new <code>RHash</code> to compute
	 * message digests for given types.
	 * @param  types  types of hashing algorithms
	 * @throws NullPointerException
	 *   if any of arguments is <code>null</code>
	 * @throws IllegalArgumentException
	 *   if zero hash types specified
	 */
	public RHash(HashType... types) {
		if (types.length == 0) {
			throw new IllegalArgumentException(ERR_NOHASH);
		}
		int flags = 0;
		HashType def = types[0];
		for (HashType t : types) {
			flags |= t.hashId();
			if (def.compareTo(t) > 0) def = t;
		}
		this.deftype = def;
		this.hash_flags = flags;
		this.context_ptr = Bindings.rhash_init(flags);
	}

	/**
	 * Creates new <code>RHash</code> to compute
	 * message digests for given types.
	 * @param  types  set of hashing types
	 * @throws NullPointerException
	 *   if argument is <code>null</code>
	 * @throws IllegalArgumentException
	 *   if argument is empty set
	 */
	public RHash(Set<HashType> types) {
		if (types.isEmpty()) {
			throw new IllegalArgumentException(ERR_NOHASH);
		}
		int flags = 0;
		HashType def = null;
		for (HashType t : types) {
			flags |= t.hashId();
			if (def == null || def.compareTo(t) > 0) def = t;
		}
		this.deftype = def;
		this.hash_flags = flags;
		this.context_ptr = Bindings.rhash_init(flags);
	}

	/**
	 * Updates this <code>RHash</code> with new data chunk.
	 * This method hashes bytes from <code>data[ofs]</code>
	 * through <code>data[ofs+len-1]</code>.
	 * 
	 * @param  data  data to be hashed
	 * @param  ofs   index of the first byte to hash
	 * @param  len   number of bytes to hash
	 * @return  this object
	 * @throws NullPointerException
	 *   if <code>data</code> is <code>null</code>
	 * @throws IndexOutOfBoundsException
	 *   if <code>ofs &lt; 0</code>, <code>len &lt; 0</code> or
	 *   <code>ofs+len &gt; data.length</code>
	 * @throws IllegalStateException
	 *   if <code>finish()</code> was called and there were no
	 *   subsequent calls of <code>reset()</code>
	 */
	public synchronized RHash update(byte[] data, int ofs, int len) {
		if (finished) {
			throw new IllegalStateException(ERR_FINISHED);
		}
		if (ofs < 0 || len < 0 || ofs+len > data.length) {
			throw new IndexOutOfBoundsException();
		}
		Bindings.rhash_update(context_ptr, data, ofs, len);
		return this;
	}

	/**
	 * Updates this <code>RHash</code> with new data chunk.
	 * This method has the same effect as
	 * <pre>update(data, 0, data.length)</pre>
	 *
	 * @param  data  data to be hashed
	 * @return  this object
	 * @throws NullPointerException
	 *   if <code>data</code> is <code>null</code>
	 * @throws IllegalStateException
	 *   if <code>finish()</code> was called and there were no
	 *   subsequent calls of <code>reset()</code>
	 */
	public RHash update(byte[] data) {
		return update(data, 0, data.length);
	}

	/**
	 * Updates this <code>RHash</code> with new data chunk.
	 * String is encoded into a sequence of bytes using the
	 * default platform encoding.
	 * 
	 * @param str  string to be hashed
	 * @return this object
	 * @throws NullPointerException
	 *   if <code>str</code> is <code>null</code>
	 * @throws IllegalStateException
	 *   if <code>finish()</code> was called and there were no
	 *   subsequent calls of <code>reset()</code>
	 */
	public RHash update(String str) {
		return update(str.getBytes());
	}

	/**
	 * Updates this <code>RHash</code> with data from given file.
	 * 
	 * @param  file  file to be hashed
	 * @return this object
	 * @throws IOException if an I/O error occurs
	 * @throws NullPointerException
	 *   if <code>file</code> is <code>null</code>
	 * @throws IllegalStateException
	 *   if <code>finish()</code> was called and there were no
	 *   subsequent calls of <code>reset()</code>
	 */
	public synchronized RHash update(File file) throws IOException {
		if (finished) {
			throw new IllegalStateException(ERR_FINISHED);
		}
		InputStream in = new FileInputStream(file);
		byte[] buf = new byte[8192];  //shouldn't we avoid magic numbers?
		int len = in.read(buf);
		while (len > 0) {
			this.update(buf, 0, len);
			len = in.read(buf);
		}
		in.close();
		return this;
	}

	/**
	 * Finishes calculation of hash codes.
	 * Does nothing if <code>RHash</code> is already finished.
	 */
	public synchronized void finish() {
		if (!finished) {
			Bindings.rhash_final(context_ptr);
			finished = true;
		}
	}

	/**
	 * Resets this <code>RHash</code> to initial state.
	 * The <code>RHash</code> becomes available to process
	 * new data chunks. Note, that this method returns
	 * <code>RHash</code> to the state after creating the
	 * object, NOT the state when hashing continues.
	 * Therefore, all previously calculated hashes are lost
	 * and process starts from the very beginning.
	 */
	public synchronized void reset() {
		Bindings.rhash_reset(context_ptr);
		finished = false;
	}

	/**
	 * Tests whether this <code>RHash</code> is finished or not.
	 * @return
	 *   <code>false</code> if this <code>RHash</code> is ready to
	 *   receive new data for hashing;
	 *   <code>true</code> if hash calculations are finished
	 */
	public boolean isFinished() {
		return finished;
	}

	/**
	 * Returns digest for given hash type.
	 * 
	 * @param type  hash type
	 * @return  <code>Digest</code> for processed data
	 * @throws NullPointerException
	 *   if <code>type</code> is <code>null</code>
	 * @throws IllegalStateException
	 *   if this <code>RHash</code> is not finished
	 * @throws IllegalArgumentException
	 *   if this <code>RHash</code> was not created to calculate
	 *   hash for specified algorithm
	 */
	public Digest getDigest(HashType type) {
		if (type == null) {
			throw new NullPointerException();
		}
		if (!finished) {
			throw new IllegalStateException(ERR_UNFINISHED);
		}
		if ((hash_flags & type.hashId()) == 0) {
			throw new IllegalArgumentException(ERR_WRONGTYPE+type);
		}
		return new Digest(Bindings.rhash_print(context_ptr, type.hashId()), type);
	}

	/**
	 * Returns digest for processed data.
	 * If more than one hashing type was passed to the
	 * <code>RHash</code> constructor, then the least
	 * hash type (in the order induced by
	 * {@link Enum#compareTo(Enum) compareTo()}) is used.
	 * 
	 * @return <code>Digest</code> for processed data
	 * @throws IllegalStateException
	 *   if this <code>RHash</code> is not finished
	 */
	public Digest getDigest() {
		return getDigest(deftype);
	}
	
	/**
	 * Returns magnet link that includes specified filename
	 * and hashes for given algorithms. Only hashes that were
	 * computed by this <code>RHash</code> are included.
	 *
	 * @param  filename  file name to include in magnet, may be <code>null</code>
	 * @return magnet link
	 * @throws IllegalStateException
	 *   if this <code>RHash</code> is not finished
	 */
	public String getMagnet(String filename, HashType... types) {
		if (!finished) {
			throw new IllegalStateException(ERR_UNFINISHED);
		}
		int flags = 0;
		for (HashType t : types) {
			flags |= t.hashId();
		}
		return Bindings.rhash_print_magnet(context_ptr, filename, flags);
	}

	/**
	 * Returns magnet link for given filename.
	 * Magnet includes all hashes that were computed
	 * by this <code>RHash</code>.
	 * 
	 * @param  filename  file name to include in magnet, may be <code>null</code>
	 * @return magnet link
	 * @throws IllegalStateException
	 *   if this <code>RHash</code> is not finished
	 */
	public String getMagnet(String filename) {
		if (!finished) {
			throw new IllegalStateException(ERR_UNFINISHED);
		}
		return Bindings.rhash_print_magnet(context_ptr, filename, hash_flags);
	}
	
	/**
	 * Called by garbage collector to free native resources.
	 */
	@Override
	protected void finalize() {
		Bindings.rhash_free(context_ptr);
	}
}
