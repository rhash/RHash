#!/usr/bin/ruby

require "test/unit"
require "rhash"

class TestRHash < Test::Unit::TestCase

    def test_all_hashes
	r = RHash.new(RHash::ALL)
	r.update("a").finish()

	assert_equal("e8b7be43", r.to_s(RHash::CRC32))
	assert_equal("bde52cb31de33e46245e05fbdbd6fb24", r.to_s(RHash::MD4))
	assert_equal("0cc175b9c0f1b6a831c399e269772661", r.to_s(RHash::MD5))
	assert_equal("86f7e437faa5a7fce15d1ddcb9eaeaea377667b8", r.to_s(RHash::SHA1))
	assert_equal("77befbef2e7ef8ab2ec8f93bf587a7fc613e247f5f247809", r.to_s(RHash::TIGER))
	assert_equal("czquwh3iyxbf5l3bgyugzhassmxu647ip2ike4y", r.to_s(RHash::TTH))
	assert_equal("fd408e9d024b58a57aa1313eff14005ff8b2c5d1", r.to_s(RHash::BTIH))
	assert_equal("bde52cb31de33e46245e05fbdbd6fb24", r.to_s(RHash::ED2K))
	assert_equal("q336in72uwt7zyk5dxolt2xk5i3xmz5y", r.to_s(RHash::AICH))
	assert_equal("8aca2602792aec6f11a67206531fb7d7f0dff59413145e6973c45001d0087b42d11bc645413aeff63a42391a39145a591a92200d560195e53b478584fdae231a", r.to_s(RHash::WHIRLPOOL))
	assert_equal("0bdc9d2d256b3ee9daae347be6f4dc835a467ffe", r.to_s(RHash::RIPEMD160))
	assert_equal("d42c539e367c66e9c88a801f6649349c21871b4344c6a573f849fdce62f314dd", r.to_s(RHash::GOST))
	assert_equal("e74c52dd282183bf37af0079c9f78055715a103f17e3133ceff1aacf2f403011", r.to_s(RHash::GOST_CRYPTOPRO))
	assert_equal("4872bcbc4cd0f0a9dc7c2f7045e5b43b6c830db8", r.to_s(RHash::HAS160))
	assert_equal("bf5ce540ae51bc50399f96746c5a15bd", r.to_s(RHash::SNEFRU128))
	assert_equal("45161589ac317be0ceba70db2573ddda6e668a31984b39bf65e4b664b584c63d", r.to_s(RHash::SNEFRU256))
	assert_equal("abd37534c7d9a2efb9465de931cd7055ffdb8879563ae98078d6d6d5", r.to_s(RHash::SHA224))
	assert_equal("ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", r.to_s(RHash::SHA256))
	assert_equal("54a59b9f22b0b80880d8427e548b7c23abd873486e1f035dce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31", r.to_s(RHash::SHA384))
	assert_equal("1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f5302860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75", r.to_s(RHash::SHA512))
	assert_equal("943aa9225a2cf154ec2e4dd81237720ba538ca8df2fd83c0b893c5d265f353a0", r.to_s(RHash::EDONR256))
	assert_equal("b59ec44f7beef8a04ceed38a973d77c65e22e9458d5f67b497948da34986c093b5efc5483fbee55f2f740fcad31f18d80db44bb6b8843e7fd599188e7c07233b", r.to_s(RHash::EDONR512))

	assert_equal("d41d8cd98f00b204e9800998ecf8427e", r.reset().finish().to_s(RHash::MD5)) # MD5( "" )
    end

    def test_shift_operator
	r = RHash.new(RHash::MD5)
	r << "a" << "bc"
	assert_equal("900150983cd24fb0d6963f7d28e17f72", r.finish().to_s()) # MD5( "abc" )
    end

    def test_output
	r = RHash.new(RHash::MD5, RHash::TTH)
	r.finish()
	assert_equal("5d9ed00a030e638bdb753a6a24fb900e5a63b8e73e6c25b6", r.to_hex(RHash::TTH))
	assert_equal("2qoyzwmpaczaj2mabgmoz6ccpy", r.to_base32(RHash::MD5))
#	assert_equal("1B2M2Y8AsgTpgAmY7PhCfg==", r.to_base64(RHash::MD5)) <- BASE64 is broken in 1.2.7 :-/
	assert_equal("\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e", r.to_raw(RHash::MD5))
    end

    def test_update_file
	path = "ruby_test_input_123.txt"
	File.open(path, 'wb') { |f| f.write("\0\1\2\n") }
	r = RHash.new(RHash::SHA1)
	r.update_file(path).finish()
	assert_equal("e3869ec477661fad6b9fc25914bb2eee5455b483", r.to_s(RHash::SHA1))
	File.delete(path)
    end

end
