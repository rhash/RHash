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
using System.Collections.Generic;
using RHash;

class Test {
	static void Main(string[] args) {
		byte[] testbytes = {(byte)'1', (byte)'2', (byte)'3', (byte)'4', (byte)'5', (byte)'\n'};
	
		Dictionary<HashType,string> hashes = new Dictionary<HashType,string>();
		hashes.Add(HashType.CRC32,     "261dafe6");
		hashes.Add(HashType.MD4,       "b1a45cdad19cb02482323fac9cea9b9f");
		hashes.Add(HashType.MD5,       "d577273ff885c3f84dadb8578bb41399");
		hashes.Add(HashType.SHA1,      "2672275fe0c456fb671e4f417fb2f9892c7573ba");
		hashes.Add(HashType.SHA224,    "ea2fa9708c96b4acb281be31fa98827addc5017305b7a038a3fca413");
		hashes.Add(HashType.SHA256,    "f33ae3bc9a22cd7564990a794789954409977013966fb1a8f43c35776b833a95");
		hashes.Add(HashType.SHA384,    "4e1cbb008acaa65ba788e3f150f7a8689c8fca289a57a65ef65b28f11ba61e59c3f4ddf069ca9521a9ac0e02eade4dae");
		hashes.Add(HashType.SHA512,    "f2dc0119c9dac46f49d3b7d0be1f61adf7619b770ff076fb11a2f61ff3fcba6b68d224588c4983670da31b33b4efabd448e38a2fda508622cc33ff8304ddf49c");
		hashes.Add(HashType.TIGER,     "6a31f8b7b80bab8b45263f56b5f609f93daf47d0a086bda5");
		hashes.Add(HashType.TTH,       "dctamcmte5tqwam5afghps2xpx3yeozwj2odzcq");
		//hashes.Add(HashType.BTIH,      "d4344cf79b89e4732c6241e730ac3f945d7a774c");
		hashes.Add(HashType.AICH,      "ezzcox7ayrlpwzy6j5ax7mxzrewhk452");
		hashes.Add(HashType.ED2K,      "b1a45cdad19cb02482323fac9cea9b9f");
		hashes.Add(HashType.WHIRLPOOL, "0e8ce019c9d5185d2103a4ff015ec92587da9b22e77ad34f2eddbba9705b3602bc6ede67f5b5e4dd225e7762208ea54895b26c39fc550914d6eca9604b724d11");
		hashes.Add(HashType.GOST,      "0aaaf17200323d024437837d6f6f6384a4a108474cff03cd349ac12776713f5f");
		hashes.Add(HashType.GOST_CRYPTOPRO, "2ed45a995ffdd7a2e5d9ab212c91cec5c65448e6a0840749a00f326ccb0c936d");
		hashes.Add(HashType.RIPEMD160, "ead888178685c5d3a0400befba9188e4da3d5144");
		hashes.Add(HashType.HAS160,    "c7589afd23462e76703b1f7a031010eec70180d4");
		hashes.Add(HashType.SNEFRU128, "d559a2b62f6f44111324f85208723707");
		hashes.Add(HashType.SNEFRU256, "1b59927d85a9349a87796620fe2ff401a06a7ba48794498ebab978efc3a68912");
		hashes.Add(HashType.EDONR256,  "c3d2bbfd63f7461a806f756bf4efeb224036331a9c1d867d251e9e480b18e6fb");
		hashes.Add(HashType.EDONR512,  "a040056378fbd1f9a528677defd141c964fab9c429003fecf2eadfc20c8980cf2e083a1b4e74d5369af3cc0537bcf9b386fedf3613c9ee6c44f54f11bcf3feae");
		hashes.Add(HashType.SHA3_224,  "85c4da0ecd8a48e3037b386bd8eda3fda126da3718e5915a7a278852");
		hashes.Add(HashType.SHA3_256,  "2cdf0ba4e0e4030458a78d501aeacc7a7ae41828a426f131e6cfc970b2c9da07");
		hashes.Add(HashType.SHA3_384,  "f3b149af9526580999ad5a114c1ee143840fe1b6d76d7a8c26a3fab38b0e8c70c989aed2c858903f5c4b66400de50874");
		hashes.Add(HashType.SHA3_512,  "5edfdeca660f19975c192e8e24ef6deb8f76f334e6e3fa11dc4d7bfcd7dbfa760fc9a9fb205e4b2f92a427208dcfe6c03c26e02bfb143e0c2d8eefc645f4d076");

		Console.WriteLine("\nTests: hashes for message");
		int errcount1 = 0;
		foreach (HashType t in hashes.Keys) {
			string mustbe = hashes[t];
			string got    = Hasher.GetHashForMsg(testbytes, t);
			if (!got.Equals(mustbe)) {
				Console.WriteLine("Test for {0} failed: expected '{1}', got '{2}'\n", t, mustbe, got);
				errcount1++;
			}
		}
		Console.WriteLine("{0} tests / {1} failed\n", hashes.Count, errcount1);

		Console.WriteLine("\nTests: hashes for file");
		int errcount2 = 0;
		foreach (HashType t in hashes.Keys) {
			string mustbe = hashes[t];
			string got    = Hasher.GetHashForFile("12345.txt", t);
			if (!got.Equals(mustbe)) {
				Console.WriteLine("Test for {0} failed: expected '{1}', got '{2}'\n", t, mustbe, got);
				errcount2++;
			}
		}
		Console.WriteLine("{0} tests / {1} failed\n", hashes.Count, errcount2);
		
        Console.WriteLine("\nTests: magnet links");
        int errcount3 = 0;
		{
	        // magnet by static method
    	    string mustbe = "magnet:?xl=6&dn=12345.txt&xt=urn:crc32:261dafe6&xt=urn:md5:d577273ff885c3f84dadb8578bb41399";
        	string got = Hasher.GetMagnetFor("12345.txt", (uint)HashType.CRC32 | (uint)HashType.MD5);
	        if (!got.Equals(mustbe)) {
    	        Console.WriteLine("Magnet by static method test failed: expected '{0}', got '{1}'\n", mustbe, got);
        	    errcount3++;
	        }
    	    // magnet with null argument
	        Hasher hasher = new Hasher((uint)HashType.CRC32 | (uint)HashType.MD5);
	        hasher.UpdateFile("12345.txt").Finish();
	        mustbe = "magnet:?xl=6&xt=urn:crc32:261dafe6";
	        got = hasher.GetMagnet(null, (uint)HashType.CRC32 | (uint)HashType.AICH);
	        if (!got.Equals(mustbe)) {
	            Console.WriteLine("Magnet with null argument test failed: expected '{0}', got '{1}'\n", mustbe, got);
	            errcount3++;
	        }
		}
        Console.WriteLine("{0} tests / {1} failed\n", 2, errcount3);

		System.Environment.ExitCode = errcount1 + errcount2 + errcount3;
	}
}
