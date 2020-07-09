/*
 * This file is a part of Java Bindings for Librhash
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

using System;
using System.IO;
using System.Text;

namespace RHash {

	public sealed class Hasher {

		private const int DEFAULT   = 0x0;
		/* output binary message digest */
		private const int RAW       = 0x1;
		/* print message digest as a hexadecimal string */
		private const int HEX       = 0x2;
		/* print message digest as a base32-encoded string */
		private const int BASE32    = 0x3;
		/* print message digest as a base64-encoded string */
		private const int BASE64    = 0x4;
		/* Print message digest as an uppercase string. */
		private const int UPPERCASE = 0x8;
		/* Reverse bytes order for hexadecimal message digest. */
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
			if (ptr == IntPtr.Zero) throw new ArgumentException("Hash functions bit-mask must be non-zero", "hashmask");
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
				throw new ArgumentException("This hasher has not computed message digest for id: "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, 0);
			return sb.ToString();
		}

		public string ToHex(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher has not computed message digest for id: "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, HEX);
			return sb.ToString();
		}

		public string ToBase32(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher has not computed message digest for id: "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, BASE32);
			return sb.ToString();
		}

		public string ToBase64(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher has not computed message digest for id: "+type, "type");
			}
			StringBuilder sb = new StringBuilder(130);
			Bindings.rhash_print(sb, ptr, (uint)type, BASE64);
			return sb.ToString();
		}

		public string ToRaw(HashType type) {
			if ((hash_ids & (uint)type) == 0) {
				throw new ArgumentException("This hasher has not computed message digest for id: "+type, "type");
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
