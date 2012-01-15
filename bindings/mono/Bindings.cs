/*
 * This file is a part of Mono Bindings for Librhash
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
using System.Runtime.InteropServices;
using System.Text;

namespace RHash {
	/* Pointer to native structure. */
	sealed class Bindings {
		
#if UNIX
		private const string librhash = "librhash.so.0";
#else
		private const string librhash = "librhash.dll";
#endif

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
