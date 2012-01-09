import org.sf.rhash.*;
import static org.sf.rhash.HashType.*;
import java.io.File;
import java.io.IOException;
import java.util.HashMap;

public class Test {

	static final String teststring = "12345\n";
	static final File testfile = new File("test/12345.txt");

	static public void main(String[] args) throws IOException {
		HashMap<HashType,String> hashes = new HashMap<HashType,String>();
		hashes.put(CRC32,     "261dafe6");
		hashes.put(MD4,       "b1a45cdad19cb02482323fac9cea9b9f");
		hashes.put(MD5,       "d577273ff885c3f84dadb8578bb41399");
		hashes.put(SHA1,      "2672275fe0c456fb671e4f417fb2f9892c7573ba");
		hashes.put(SHA224,    "ea2fa9708c96b4acb281be31fa98827addc5017305b7a038a3fca413");
		hashes.put(SHA256,    "f33ae3bc9a22cd7564990a794789954409977013966fb1a8f43c35776b833a95");
		hashes.put(SHA384,    "4e1cbb008acaa65ba788e3f150f7a8689c8fca289a57a65ef65b28f11ba61e59c3f4ddf069ca9521a9ac0e02eade4dae");
		hashes.put(SHA512,    "f2dc0119c9dac46f49d3b7d0be1f61adf7619b770ff076fb11a2f61ff3fcba6b68d224588c4983670da31b33b4efabd448e38a2fda508622cc33ff8304ddf49c");
		hashes.put(TIGER,     "6a31f8b7b80bab8b45263f56b5f609f93daf47d0a086bda5");
		hashes.put(TTH,       "dctamcmte5tqwam5afghps2xpx3yeozwj2odzcq");
		//hashes.put(BTIH,      "d4344cf79b89e4732c6241e730ac3f945d7a774c");
		hashes.put(AICH,      "ezzcox7ayrlpwzy6j5ax7mxzrewhk452");
		hashes.put(ED2K,      "b1a45cdad19cb02482323fac9cea9b9f");
		hashes.put(WHIRLPOOL, "0e8ce019c9d5185d2103a4ff015ec92587da9b22e77ad34f2eddbba9705b3602bc6ede67f5b5e4dd225e7762208ea54895b26c39fc550914d6eca9604b724d11");
		hashes.put(GOST,      "0aaaf17200323d024437837d6f6f6384a4a108474cff03cd349ac12776713f5f");
		hashes.put(GOST_CRYPTOPRO, "2ed45a995ffdd7a2e5d9ab212c91cec5c65448e6a0840749a00f326ccb0c936d");
		hashes.put(RIPEMD160, "ead888178685c5d3a0400befba9188e4da3d5144");
		hashes.put(HAS160,    "c7589afd23462e76703b1f7a031010eec70180d4");
		hashes.put(SNEFRU128, "d559a2b62f6f44111324f85208723707");
		hashes.put(SNEFRU256, "1b59927d85a9349a87796620fe2ff401a06a7ba48794498ebab978efc3a68912");
		hashes.put(EDONR256,  "c3d2bbfd63f7461a806f756bf4efeb224036331a9c1d867d251e9e480b18e6fb");
		hashes.put(EDONR512,  "a040056378fbd1f9a528677defd141c964fab9c429003fecf2eadfc20c8980cf2e083a1b4e74d5369af3cc0537bcf9b386fedf3613c9ee6c44f54f11bcf3feae");
		
		System.err.println("\nTests: hashes for message");
		int errcount1 = 0;
		for (HashType t : hashes.keySet()) {
			String mustbe = hashes.get(t);
			String got    = RHash.computeHash(t, teststring).toString();
			if (!got.equals(mustbe)) {
				System.err.printf("Test for %s failed: expected '%s', got '%s'\n", t, mustbe, got);
				errcount1++;
			}
		}
		System.err.printf("%d tests / %d failed\n", hashes.size(), errcount1);

		System.err.println("\nTests: hashes for file");
		int errcount2 = 0;
		for (HashType t : hashes.keySet()) {
			String mustbe = hashes.get(t);
			String got    = RHash.computeHash(t, testfile).toString();
			if (!got.equals(mustbe)) {
				System.err.printf("Test for %s failed: expected '%s', got '%s'\n", t, mustbe, got);
				errcount2++;
			}
		}
		System.err.printf("%d tests / %d failed\n", hashes.size(), errcount2);

		System.err.println("\nTests: magnet links");
		int errcount3 = 0;
		// magnet by static method
		String mustbe = "magnet:?xl=6&dn=test%2F12345.txt&xt=urn:crc32:261dafe6&xt=urn:md5:d577273ff885c3f84dadb8578bb41399";
		String got = RHash.getMagnetFor("test/12345.txt", CRC32, MD5);
		if (!got.equals(mustbe)) {
			System.err.printf("Magnet test failed: expected '%s', got '%s'\n", mustbe, got);
			errcount3++;
		}
		// magnet with null argument
		RHash hasher = new RHash(CRC32, MD5);
		hasher.update(testfile).finish();
		mustbe = "magnet:?xl=6&xt=urn:crc32:261dafe6";
		got = hasher.getMagnet(null, CRC32, AICH);
		if (!got.equals(mustbe)) {
			System.err.printf("Magnet test failed: expected '%s', got '%s'\n", mustbe, got);
			errcount3++;
		}
		System.err.printf("%d tests / %d failed\n", 1, errcount3);

		System.exit(errcount1+errcount2+errcount3);
	}
}
