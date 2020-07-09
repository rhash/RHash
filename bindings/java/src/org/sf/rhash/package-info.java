/*
 * This file is a part of Java Bindings for LibRHash
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

/**
 * Java bindings for LibRHash.
 * LibRHash  is  a library for computing message digests  and magnet links
 * for various hash functions. List of all supported hash functions can be
 * found in {@link org.sf.rhash.HashType} class description.
 * <p>
 * In its simplest usage to calculate a hash for message or file
 * you just need to use one of <code>RHash.computeHash()</code>
 * methods:
 * <pre>
 *     RHash.computeHash("Hello, world!");
 *     RHash.computeHash(new byte[] { 0, 1, 2, 3});
 *     RHash.computeHash(new File("SomeFile.txt"));</pre>
 * These methods return value of type <code>Digest</code> which is
 * a message digest. To convert <code>Digest</code> in human readable
 * format you might use one of methods
 * {@link org.sf.rhash.Digest#hex() hex()},
 * {@link org.sf.rhash.Digest#base32() base32()},
 * {@link org.sf.rhash.Digest#base64() base64()} or
 * {@link org.sf.rhash.Digest#raw() raw()}.
 * </p><p>
 * Next, <code>RHash</code> allows you to do incremental hashing,
 * processing data given in portions like it was one big byte sequence.
 * To do this you first need to create <code>RHash</code> instance
 * with a set of needed hash algorithms and then to fill it using
 * <code>update()</code>:
 * <pre>
 *     RHash hasher = new RHash(HashType.MD5);
 *     hasher.update("Foo").update(new File("Bar.zip")).finish();
 *     Digest result = hasher.getDigest();</pre>
 * Method <code>finish()</code> should be called before obtaining
 * digest to end all calculations and generate result.
 * </p><p>
 * You can setup <code>RHash</code> to calculate several digests
 * at once, passing corresponding <code>HashType</code>s in
 * constructor. Specifically, you can calculate all of them creating
 * <code>RHash</code> like
 * <pre>
 *     new Rhash(EnumSet.allOf(HashType.class));</pre>
 * In this case to obtain digest for particular hash type use
 * {@link org.sf.rhash.RHash#getDigest(HashType) }
 * method.
 * </p>
 */
package org.sf.rhash;
