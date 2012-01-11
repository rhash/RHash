                                                                     
                                                                     
                                                                     
                                             
import rhash
import os
import unittest

class TestRHash(unittest.TestCase):

    def test_all_hashes(self):
	r = rhash.RHash(rhash.ALL)
	r.update('a')
	r.finish()
	self.assertEqual('e8b7be43', r.__str__(rhash.CRC32))
	self.assertEqual('bde52cb31de33e46245e05fbdbd6fb24', r.__str__(rhash.MD4))
	self.assertEqual('0cc175b9c0f1b6a831c399e269772661', r.__str__(rhash.MD5))
	self.assertEqual('86f7e437faa5a7fce15d1ddcb9eaeaea377667b8', r.__str__(rhash.SHA1))
	self.assertEqual('77befbef2e7ef8ab2ec8f93bf587a7fc613e247f5f247809', r.__str__(rhash.TIGER))
	self.assertEqual('czquwh3iyxbf5l3bgyugzhassmxu647ip2ike4y', r.__str__(rhash.TTH))
	self.assertEqual('fd408e9d024b58a57aa1313eff14005ff8b2c5d1', r.__str__(rhash.BTIH))
	self.assertEqual('bde52cb31de33e46245e05fbdbd6fb24', r.__str__(rhash.ED2K))
	self.assertEqual('q336in72uwt7zyk5dxolt2xk5i3xmz5y', r.__str__(rhash.AICH))
	self.assertEqual('8aca2602792aec6f11a67206531fb7d7f0dff59413145e6973c45001d0087b42d11bc645413aeff63a42391a39145a591a92200d560195e53b478584fdae231a', r.__str__(rhash.WHIRLPOOL))
	self.assertEqual('0bdc9d2d256b3ee9daae347be6f4dc835a467ffe', r.__str__(rhash.RIPEMD160))
	self.assertEqual('d42c539e367c66e9c88a801f6649349c21871b4344c6a573f849fdce62f314dd', r.__str__(rhash.GOST))
	self.assertEqual('e74c52dd282183bf37af0079c9f78055715a103f17e3133ceff1aacf2f403011', r.__str__(rhash.GOST_CRYPTOPRO))
	self.assertEqual('4872bcbc4cd0f0a9dc7c2f7045e5b43b6c830db8', r.__str__(rhash.HAS160))
	self.assertEqual('bf5ce540ae51bc50399f96746c5a15bd', r.__str__(rhash.SNEFRU128))
	self.assertEqual('45161589ac317be0ceba70db2573ddda6e668a31984b39bf65e4b664b584c63d', r.__str__(rhash.SNEFRU256))
	self.assertEqual('abd37534c7d9a2efb9465de931cd7055ffdb8879563ae98078d6d6d5', r.__str__(rhash.SHA224))
	self.assertEqual('ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb', r.__str__(rhash.SHA256))
	self.assertEqual('54a59b9f22b0b80880d8427e548b7c23abd873486e1f035dce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31', r.__str__(rhash.SHA384))
	self.assertEqual('1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75', r.__str__(rhash.SHA512))
	self.assertEqual('943aa9225a2cf154ec2e4dd81237720ba538ca8df2fd83c0b893c5d265f353a0', r.__str__(rhash.EDONR256))
	self.assertEqual('b59ec44f7beef8a04ceed38a973d77c65e22e9458d5f67b497948da34986c093b5efc5483fbee55f2f740fcad31f18d80db44bb6b8843e7fd599188e7c07233b', r.__str__(rhash.EDONR512))
	# test reset
	self.assertEqual('d41d8cd98f00b204e9800998ecf8427e', r.reset().finish().__str__(rhash.MD5)) # MD5( '' )

    def test_update(self):
	r = rhash.RHash(rhash.CRC32 | rhash.MD5)
	r.update('Hello, ').update('world!').finish()
	self.assertEqual('EBE6C6E6', r.HEX(rhash.CRC32));
	self.assertEqual('6cd3556deb0da54bca060b4c39479839', r.hex(rhash.MD5));

    def test_shift_operator(self):
	r = rhash.RHash(rhash.MD5)
	r << 'a' << 'bc'
	self.assertEqual('900150983cd24fb0d6963f7d28e17f72', str(r.finish())) # MD5( 'abc' )
	self.assertEqual('900150983cd24fb0d6963f7d28e17f72', rhash.hash_for_msg('abc', rhash.MD5))

    def test_output(self):
	r = rhash.RHash(rhash.MD5 | rhash.TTH).finish()
	self.assertEqual('5d9ed00a030e638bdb753a6a24fb900e5a63b8e73e6c25b6', r.hex(rhash.TTH))
	self.assertEqual('2qoyzwmpaczaj2mabgmoz6ccpy', r.base32(rhash.MD5))
	self.assertEqual('1B2M2Y8AsgTpgAmY7PhCfg==', r.base64(rhash.MD5))
	self.assertEqual('\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e', r.raw(rhash.MD5))

    def test_magnet(self):
	r = rhash.RHash(rhash.MD5 | rhash.TTH)
	r.update('abc').finish()
	self.assertEqual('magnet:?xl=3&dn=file.txt&xt=urn:md5:900150983cd24fb0d6963f7d28e17f72&xt=urn:tree:tiger:asd4ujseh5m47pdyb46kbtsqtsgdklbhyxomuia', r.magnet('file.txt'))

    def test_update_file(self):
	path = 'python_test_input_123.txt'
	f = open(path, 'w')
	f.write("\0\1\2\n")
	f.close()

	r = rhash.RHash(rhash.SHA1)
	r.update_file(path).finish()
	self.assertEqual('e3869ec477661fad6b9fc25914bb2eee5455b483', str(r))
	self.assertEqual('e3869ec477661fad6b9fc25914bb2eee5455b483', rhash.hash_for_file(path, rhash.SHA1))
	os.remove(path)

if __name__ == '__main__':
    unittest.main()
