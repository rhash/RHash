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

using System;
using System.IO;
using System.Text;

namespace RHash {
		
	public sealed class Hasher {

		private const int DEFAULT   = 0x0;
		/* output as binary message digest */
		private const int RAW       = 0x1;
		/* print as a hexadecimal string */
		private const int HEX       = 0x2;
		/* print as a base32-encoded string */
		private const int BASE32    = 0x3;
		/* print as a base64-encoded string */
		private const int BASE64    = 0x4;
		/* Print as an uppercase string. */
		private const int UPPERCASE = 0x8;
		/* Reverse hash bytes. */
		private const int REVERSE   = 0x10;
		/* Print file size. */
		private const int FILESIZE  = 0x40;
		
		private uint hash_ids;
		/* Pointer to the native structure. */
		private IntPtr ptr;
		
		public Hasher (HashType hashtype) {
			this.hash_ids = (uint)hashtype;
			this.ptr = Bindings.rhash_init(hash_ids);
		}
		
		public Hasher (uint hashmask) {
			this.hash_ids = hashmask;
			this.ptr = Bindings.rhash_init(hash_ids);
			if (ptr == IntPtr.Zero) throw new ArgumentException("Invalid mask of hashes", "hashmask");
		}
		
		~Hasher() {
			if (ptr != IntPtr.Zero) {
				Bindings.rhash_free(ptr);
				ptr = IntPtr.Zero;
			}
		}
		
		public Hasher Update(byte[] buf) {
			Bindings.rhash_update(ptr, buf, buf.Length);
			return this;
		}

		public Hasher Update(byte[] buf, int len) {
			if (len < 0 || len >= buf.Length) {
				throw new IndexOutOfRangeException();
			}
			Bindings.rhash_update(ptr, buf, len);
			return this;
		}
		
		public Hasher UpdateFile(string filename) {
			Stream file = new FileStream(filename, FileMode.Open);
			byte[] buf = new byte[8192];
			int len = file.Read(buf, 0, 8192);
			while (len > 0) {
				Bindings.rhash_update(ptr, buf, len);
				len = file.Read(buf, 0, 8192);
			}
			file.Close();
			return this;
		}
		
		public void Finish() {
			Bindings.rhash_final(ptr, IntPtr.Zero);
		}
		
		public void Reset() {
			Bindings.rhash_reset(ptr);
		}
		
		public override string ToString() {
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, 0, 0);
			return sb.ToString();
		}
		
		public string ToString(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher does not support hash type "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, 0);
			return sb.ToString();
		}
		
		public string ToHex(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher does not support hash type "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, HEX);
			return sb.ToString();
		}

		public string ToBase32(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher does not support hash type "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, BASE32);
			return sb.ToString();
		}

		public string ToBase64(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher does not support hash type "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, BASE32);
			return sb.ToString();
		}

		public string ToRaw(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher does not support hash type "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, RAW);
			return sb.ToString();
		}

		public string GetMagnet(string filepath) {
			return GetMagnet(filepath, hash_ids);
		}

		public string GetMagnet(string filepath, uint hashmask) {
			int len = Bindings.rhash_print_magnet(null, filepath, ptr, hashmask, FILESIZE);
			StringBuilder sb = new StringBuilder(len);
			Bindings.rhash_print_magnet(sb, filepath, ptr, hashmask, FILESIZE);
			return sb.ToString();
		}
		
		public static string GetHashForMsg(byte[] buf, HashType type) {
			return new Hasher(type).Update(buf).ToString(type);
		}
		
		public static string GetHashForFile(string filename, HashType type) {
			return new Hasher(type).UpdateFile(filename).ToString(type);
		}
		
		public static string GetMagnetFor(string filepath, uint hashmask) {
			return new Hasher(hashmask).UpdateFile(filepath).GetMagnet(filepath);
		}
	}
}
