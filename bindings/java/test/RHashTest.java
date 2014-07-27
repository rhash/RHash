import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.EnumSet;

import org.sf.rhash.*;
import static org.sf.rhash.HashType.*;

import org.junit.Test;
import junit.framework.JUnit4TestAdapter;
import static junit.framework.TestCase.*;

public class RHashTest {

	@Test
	public void testAllHashes() {
		RHash r = new RHash(EnumSet.allOf(HashType.class));
		r.update("a").finish();
		
		assertEquals("e8b7be43", r.getDigest(CRC32).toString());
		assertEquals("bde52cb31de33e46245e05fbdbd6fb24", r.getDigest(MD4).toString());
		assertEquals("0cc175b9c0f1b6a831c399e269772661", r.getDigest(MD5).toString());
		assertEquals("86f7e437faa5a7fce15d1ddcb9eaeaea377667b8", r.getDigest(SHA1).toString());
		assertEquals("77befbef2e7ef8ab2ec8f93bf587a7fc613e247f5f247809", r.getDigest(TIGER).toString());
		assertEquals("czquwh3iyxbf5l3bgyugzhassmxu647ip2ike4y", r.getDigest(TTH).toString());
		assertEquals(40, r.getDigest(BTIH).toString().length());
		assertEquals("bde52cb31de33e46245e05fbdbd6fb24", r.getDigest(ED2K).toString());
		assertEquals("q336in72uwt7zyk5dxolt2xk5i3xmz5y", r.getDigest(AICH).toString());
		assertEquals("8aca2602792aec6f11a67206531fb7d7f0dff59413145e6973c45001d0087b42d11bc645413aeff63a42391a39145a591a92200d560195e53b478584fdae231a", r.getDigest(WHIRLPOOL).toString());
		assertEquals("0bdc9d2d256b3ee9daae347be6f4dc835a467ffe", r.getDigest(RIPEMD160).toString());
		assertEquals("d42c539e367c66e9c88a801f6649349c21871b4344c6a573f849fdce62f314dd", r.getDigest(GOST).toString());
		assertEquals("e74c52dd282183bf37af0079c9f78055715a103f17e3133ceff1aacf2f403011", r.getDigest(GOST_CRYPTOPRO).toString());
		assertEquals("4872bcbc4cd0f0a9dc7c2f7045e5b43b6c830db8", r.getDigest(HAS160).toString());
		assertEquals("bf5ce540ae51bc50399f96746c5a15bd", r.getDigest(SNEFRU128).toString());
		assertEquals("45161589ac317be0ceba70db2573ddda6e668a31984b39bf65e4b664b584c63d", r.getDigest(SNEFRU256).toString());
		assertEquals("abd37534c7d9a2efb9465de931cd7055ffdb8879563ae98078d6d6d5", r.getDigest(SHA224).toString());
		assertEquals("ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", r.getDigest(SHA256).toString());
		assertEquals("54a59b9f22b0b80880d8427e548b7c23abd873486e1f035dce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31", r.getDigest(SHA384).toString());
		assertEquals("1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75", r.getDigest(SHA512).toString());
		assertEquals("943aa9225a2cf154ec2e4dd81237720ba538ca8df2fd83c0b893c5d265f353a0", r.getDigest(EDONR256).toString());
		assertEquals("b59ec44f7beef8a04ceed38a973d77c65e22e9458d5f67b497948da34986c093b5efc5483fbee55f2f740fcad31f18d80db44bb6b8843e7fd599188e7c07233b", r.getDigest(EDONR512).toString());
		assertEquals("9e86ff69557ca95f405f081269685b38e3a819b309ee942f482b6a8b", r.getDigest(SHA3_224).toString());
		assertEquals("80084bf2fba02475726feb2cab2d8215eab14bc6bdd8bfb2c8151257032ecd8b", r.getDigest(SHA3_256).toString());
		assertEquals("1815f774f320491b48569efec794d249eeb59aae46d22bf77dafe25c5edc28d7ea44f93ee1234aa88f61c91912a4ccd9", r.getDigest(SHA3_384).toString());
		assertEquals("697f2d856172cb8309d6b8b97dac4de344b549d4dee61edfb4962d8698b7fa803f4f93ff24393586e28b5b957ac3d1d369420ce53332712f997bd336d09ab02a", r.getDigest(SHA3_512).toString());

		r.reset();
		r.finish();
		assertEquals("d41d8cd98f00b204e9800998ecf8427e", r.getDigest(MD5).toString()); // MD5 of ""
	}

	@Test
	public void testMagnet() {
		RHash r = new RHash(MD5, TTH);
		r.update("abc").finish();
		assertEquals("magnet:?xl=3&dn=file.txt&xt=urn:md5:900150983cd24fb0d6963f7d28e17f72&xt=urn:tree:tiger:asd4ujseh5m47pdyb46kbtsqtsgdklbhyxomuia", r.getMagnet("file.txt"));
    }

	@Test
	public void testUpdateFile() throws IOException {
		File f = new File("java_test_input_123.txt");
		PrintStream out = new PrintStream(f);
		out.println("\0\1\2");
		out.flush();
		out.close();
		assertEquals("e3869ec477661fad6b9fc25914bb2eee5455b483", RHash.computeHash(SHA1, f).toString());
		f.delete();
    }

	public static junit.framework.Test suite(){
		return new JUnit4TestAdapter(RHashTest.class);
	}
}
