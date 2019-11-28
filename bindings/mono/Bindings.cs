/*
 * This file is a part of Mono Bindings for Librhash
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
using System.Runtime.InteropServices;
using System.Text;

namespace RHash {
	/* Pointer to native structure. */
	sealed class Bindings {

		private const string librhash = "librhash.dll";

		private Bindings() { }

		static Bindings() {
			rhash_library_init();
		}

		[DllImport (librhash)]
		public static extern
			void rhash_library_init();

		[DllImport (librhash)]
		public static extern
			IntPtr rhash_init(uint hash_ids);

		[DllImport (librhash)]
		public static extern
			void rhash_update(IntPtr ctx, byte[] message, int length);

		//may crash, rhash_final actually have 2 arguments
		[DllImport (librhash)]
		public static extern
			void rhash_final(IntPtr ctx, IntPtr unused);

		[DllImport (librhash)]
		public static extern
			void rhash_reset(IntPtr ctx);

		[DllImport (librhash)]
		public static extern
			void rhash_free(IntPtr ctx);

		[DllImport (librhash, CharSet=CharSet.Ansi)]
		public static extern
			void rhash_print(StringBuilder output, IntPtr ctx, uint hash_id, int flags);

		[DllImport (librhash)]
		public static extern
			int rhash_print_magnet(StringBuilder output, String filepath, IntPtr ctx, uint hash_mask, int flags);
	}
}
