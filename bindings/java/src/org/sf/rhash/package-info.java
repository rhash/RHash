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

/**
 * Java bindings for librhash.
 * Librhash is a library for computing and verifying hash sums.
 * List of all supported hash functions can be found in
 * {@link org.sf.rhash.HashType} class description.
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
