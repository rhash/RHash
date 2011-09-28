using System;
using System.Runtime.InteropServices;
using System.Text;

namespace RHash {
	/* Pointer to native structure. */
	sealed class Bindings {
		
		private const string librhash = "librhash.so.0";
		
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
		
		[DllImport (librhash,  CharSet=CharSet.Ansi)]
		public static extern
			void rhash_update(IntPtr ctx, string message, int length);
		
		//may crash, rhash_final actually have 2 arguments
		[DllImport (librhash)]
		public static extern
			void rhash_final(IntPtr ctx);

		[DllImport (librhash)]
		public static extern
			void rhash_reset(IntPtr ctx);
		
		[DllImport (librhash)]
		public static extern
			void rhash_free(IntPtr ctx);
		
		[DllImport (librhash)]
		public static extern
			void rhash_print(StringBuilder output, IntPtr ctx, uint hash_id, int flags);
	}
}
