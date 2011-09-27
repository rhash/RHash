using System;
using System.Text;

namespace RHash {
		
	public class RHash {

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
		
		private uint hash_ids;
		private RHashPtr ptr;
		
		public RHash (uint hashtype) {
			//TODO: throw exception if argument is invalid
			this.hash_ids = hashtype;
			this.ptr = RHashPtr.rhash_init(hashtype);
		}
		
		public RHash Update(string message) {
			RHashPtr.rhash_update(ptr, message, message.Length);
			return this;
		}
		
		public void Finish() {
			RHashPtr.rhash_final(ptr);
		}
		
		public void Reset() {
			RHashPtr.rhash_reset(ptr);
		}
		
		public override string ToString() {
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, 0, 0);
			return sb.ToString();
		}
		
		public string ToString(HashType type) {
			//TODO: throw exception if argument is invalid
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, (uint)type, 0);
			return sb.ToString();
		}
		
		public string ToHex(HashType type) {
			//TODO: throw exception if argument is invalid
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, (uint)type, HEX);
			return sb.ToString();			
		}

		public string ToBase32(HashType type) {
			//TODO: throw exception if argument is invalid
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, (uint)type, BASE32);
			return sb.ToString();			
		}

		public string ToBase64(HashType type) {
			//TODO: throw exception if argument is invalid
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, (uint)type, BASE32);
			return sb.ToString();			
		}

		public string ToRaw(HashType type) {
			//TODO: throw exception if argument is invalid
			StringBuilder sb = new StringBuilder(130);
			RHashPtr.rhash_print(sb, ptr, (uint)type, RAW);
			return sb.ToString();
		}	
	}
}
