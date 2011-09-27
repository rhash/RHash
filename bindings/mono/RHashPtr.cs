using System;
using System.Runtime.InteropServices;
using System.Text;

namespace RHash {
	/* Pointer to native structure. */
	sealed class RHashPtr {
		
		
		private RHashPtr() { }
		
		static RHashPtr() {
			rhash_library_init();
		}
		
		~RHashPtr() {
			//crashes with this :(
			//rhash_free(this);
		}

		[DllImport ("librhash.so.0")]
		public static extern
			void rhash_library_init();
		
		[DllImport ("librhash.so.0")]
		public static extern
			RHashPtr rhash_init(uint hash_ids);
		
		[DllImport ("librhash.so.0",  CharSet=CharSet.Ansi)]
		public static extern
			void rhash_update(RHashPtr ptr, string message, int length);
		
		//may crash, rhash_final actually have 2 arguments
		[DllImport ("librhash.so.0")]
		public static extern
			void rhash_final(RHashPtr ptr);

		[DllImport ("librhash.so.0")]
		public static extern
			void rhash_reset(RHashPtr ptr);
		
		[DllImport ("librhash.so.0")]
		public static extern
			void rhash_free(RHashPtr ptr);
		
		[DllImport ("librhash.so.0")]
		public static extern
			void rhash_print(StringBuilder output, RHashPtr ctx, uint hash_id, int flags);
	}
}
