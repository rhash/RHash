# Copyright (c) 2011, Aleksey Kravchenko <rhash.admin@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
# OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
# PERFORMANCE OF THIS SOFTWARE.

"""Unit-tests for the rhash module."""

import os
import unittest
import rhash


# pylint: disable=too-many-public-methods, pointless-statement
class TestRHash(unittest.TestCase):
    """The test-case class for the rhash module."""

    maxDiff = 1024

    def test_all_hashes(self):
        """Verify all hash functions."""
        ctx = rhash.RHash(rhash.ALL)
        ctx.update("a")
        ctx.finish()
        self.assertEqual("e8b7be43", ctx.hash(rhash.CRC32))
        self.assertEqual("c1d04330", ctx.hash(rhash.CRC32C))
        self.assertEqual("bde52cb31de33e46245e05fbdbd6fb24", ctx.hash(rhash.MD4))
        self.assertEqual("0cc175b9c0f1b6a831c399e269772661", ctx.hash(rhash.MD5))
        self.assertEqual("86f7e437faa5a7fce15d1ddcb9eaeaea377667b8", ctx.hash(rhash.SHA1))
        self.assertEqual(
            "77befbef2e7ef8ab2ec8f93bf587a7fc613e247f5f247809", ctx.hash(rhash.TIGER)
        )
        self.assertEqual("czquwh3iyxbf5l3bgyugzhassmxu647ip2ike4y", ctx.hash(rhash.TTH))
        self.assertEqual(40, len(ctx.hash(rhash.BTIH)))
        self.assertEqual("bde52cb31de33e46245e05fbdbd6fb24", ctx.hash(rhash.ED2K))
        self.assertEqual("q336in72uwt7zyk5dxolt2xk5i3xmz5y", ctx.hash(rhash.AICH))
        self.assertEqual(
            "8aca2602792aec6f11a67206531fb7d7f0dff59413145e6973c45001d0087b42"
            "d11bc645413aeff63a42391a39145a591a92200d560195e53b478584fdae231a",
            ctx.hash(rhash.WHIRLPOOL),
        )
        self.assertEqual(
            "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe", ctx.hash(rhash.RIPEMD160)
        )
        self.assertEqual(
            "d42c539e367c66e9c88a801f6649349c21871b4344c6a573f849fdce62f314dd",
            ctx.hash(rhash.GOST94),
        )
        self.assertEqual(
            "e74c52dd282183bf37af0079c9f78055715a103f17e3133ceff1aacf2f403011",
            ctx.hash(rhash.GOST94_CRYPTOPRO),
        )
        self.assertEqual(
            "ba31099b9cc84ec2a671e9313572378920a705b363b031a1cb4fc03e01ce8df3",
            ctx.hash(rhash.GOST12_256),
        )
        self.assertEqual(
            "8b2a40ecab7b7496bc4cc0f773595452baf658849b495acc3ba017206810efb0"
            "0420ccd73fb3297e0f7890941b84ac4a8bc27e3c95e1f97c094609e2136abb7e",
            ctx.hash(rhash.GOST12_512),
        )
        self.assertEqual(
            "4872bcbc4cd0f0a9dc7c2f7045e5b43b6c830db8", ctx.hash(rhash.HAS160)
        )
        self.assertEqual("bf5ce540ae51bc50399f96746c5a15bd", ctx.hash(rhash.SNEFRU128))
        self.assertEqual(
            "45161589ac317be0ceba70db2573ddda6e668a31984b39bf65e4b664b584c63d",
            ctx.hash(rhash.SNEFRU256),
        )
        self.assertEqual(
            "abd37534c7d9a2efb9465de931cd7055ffdb8879563ae98078d6d6d5",
            ctx.hash(rhash.SHA2_224),
        )
        self.assertEqual(
            "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb",
            ctx.hash(rhash.SHA2_256),
        )
        self.assertEqual(
            "54a59b9f22b0b80880d8427e548b7c23abd873486e1f035d"
            "ce9cd697e85175033caa88e6d57bc35efae0b5afd3145f31",
            ctx.hash(rhash.SHA2_384),
        )
        self.assertEqual(
            "1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f53"
            "02860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75",
            ctx.hash(rhash.SHA2_512),
        )
        self.assertEqual(
            "943aa9225a2cf154ec2e4dd81237720ba538ca8df2fd83c0b893c5d265f353a0",
            ctx.hash(rhash.EDONR256),
        )
        self.assertEqual(
            "b59ec44f7beef8a04ceed38a973d77c65e22e9458d5f67b497948da34986c093"
            "b5efc5483fbee55f2f740fcad31f18d80db44bb6b8843e7fd599188e7c07233b",
            ctx.hash(rhash.EDONR512),
        )
        self.assertEqual(
            "9e86ff69557ca95f405f081269685b38e3a819b309ee942f482b6a8b",
            ctx.hash(rhash.SHA3_224),
        )
        self.assertEqual(
            "80084bf2fba02475726feb2cab2d8215eab14bc6bdd8bfb2c8151257032ecd8b",
            ctx.hash(rhash.SHA3_256),
        )
        self.assertEqual(
            "1815f774f320491b48569efec794d249eeb59aae46d22bf7"
            "7dafe25c5edc28d7ea44f93ee1234aa88f61c91912a4ccd9",
            ctx.hash(rhash.SHA3_384),
        )
        self.assertEqual(
            "697f2d856172cb8309d6b8b97dac4de344b549d4dee61edfb4962d8698b7fa80"
            "3f4f93ff24393586e28b5b957ac3d1d369420ce53332712f997bd336d09ab02a",
            ctx.hash(rhash.SHA3_512),
        )
        self.assertEqual(
            "4a0d129873403037c2cd9b9048203687f6233fb6738956e0349bd4320fec3e90",
            ctx.hash(rhash.BLAKE2S),
        )
        self.assertEqual(
            "333fcb4ee1aa7c115355ec66ceac917c8bfd815bf7587d325aec1864edd24e34"
            "d5abe2c6b1b5ee3face62fed78dbef802f2a85cb91d455a8f5249d330853cb3c",
            ctx.hash(rhash.BLAKE2B),
        )
        self.assertEqual(
            "17762fddd969a453925d65717ac3eea21320b66b54342fde15128d6caf21215f",
            ctx.hash(rhash.BLAKE3),
        )
        # Test reset
        ctx.reset().finish()
        # Verify MD5( "" )
        self.assertEqual("d41d8cd98f00b204e9800998ecf8427e", ctx.hash(rhash.MD5))

    def test_update(self):
        """Test sequential updates."""
        ctx = rhash.RHash(rhash.CRC32, rhash.MD5)
        ctx.update("Hello, ").update("world!").finish()
        self.assertEqual("EBE6C6E6", ctx.hex_upper(rhash.CRC32))
        self.assertEqual("6cd3556deb0da54bca060b4c39479839", ctx.hex(rhash.MD5))

    def test_shift_operator(self):
        """Test the << operator."""
        ctx = rhash.RHash(rhash.MD5)
        ctx << "a" << "bc"
        # Verify MD5( "abc" )
        self.assertEqual("900150983cd24fb0d6963f7d28e17f72", str(ctx.finish()))

    def test_hash_msg(self):
        """Test hash_msg() function."""
        self.assertEqual(
            "900150983cd24fb0d6963f7d28e17f72", rhash.hash_msg("abc", rhash.MD5)
        )

    def test_output_formats(self):
        """Test all output formats of a message digest."""
        ctx = rhash.RHash(rhash.MD5, rhash.TTH).finish()
        self.assertEqual(
            "5d9ed00a030e638bdb753a6a24fb900e5a63b8e73e6c25b6", ctx.hex(rhash.TTH)
        )
        self.assertEqual("2qoyzwmpaczaj2mabgmoz6ccpy", ctx.base32(rhash.MD5))
        self.assertEqual("1B2M2Y8AsgTpgAmY7PhCfg==", ctx.base64(rhash.MD5))
        self.assertEqual(
            b"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e",
            ctx.raw(rhash.MD5),
        )

    def test_magnet(self):
        """Test calculation of a magnet link."""
        ctx = rhash.RHash(rhash.MD5, rhash.TTH)
        ctx.update("abc").finish()
        self.assertEqual(
            "magnet:?xl=3&dn=file.txt&"
            "xt=urn:md5:900150983cd24fb0d6963f7d28e17f72&"
            "xt=urn:tree:tiger:asd4ujseh5m47pdyb46kbtsqtsgdklbhyxomuia",
            ctx.magnet("file.txt"),
        )

    def test_update_file(self):
        """Test update_file() method."""
        path = "python_test_input_123.txt"
        with open(path, "wb") as file:
            file.write(b"\0\1\2\n")
        ctx = rhash.RHash(rhash.SHA1)
        ctx.update_file(path).finish()
        self.assertEqual("e3869ec477661fad6b9fc25914bb2eee5455b483", str(ctx))
        self.assertEqual(
            "e3869ec477661fad6b9fc25914bb2eee5455b483",
            rhash.hash_file(path, rhash.SHA1),
        )
        self.assertEqual(
            "magnet:?xl=4&dn=python_test_input_123.txt&"
            "xt=urn:crc32:f2653eb7&"
            "xt=urn:tree:tiger:c6docz63fpef5pdfpz35z7mw2iozshxlpr4erza",
            rhash.make_magnet(path, rhash.CRC32, rhash.TTH),
        )
        os.remove(path)

    def test_librhash_version(self):
        """Test get_librhash_version() function."""
        version = rhash.get_librhash_version()
        self.assertTrue(isinstance(version, str))
        self.assertRegex(version, r"^[1-9]\d*\.\d+\.\d+$")
        ver = rhash.get_librhash_version_int()
        major, minor, patch = (ver >> 24, (ver >> 16) & 255, (ver >> 8) & 255)
        self.assertTrue(1 <= major <= 9)
        self.assertTrue(0 <= minor <= 9)
        self.assertTrue(0 <= patch <= 9)

    def test_the_with_operator(self):
        """Test the with operator."""
        with rhash.RHash(rhash.CRC32, rhash.MD5) as ctx:
            ctx.update("a").finish()
            self.assertEqual("e8b7be43", ctx.hash(rhash.CRC32))
            self.assertEqual("btaxlooa6g3kqmodthrgs5zgme", ctx.base32(rhash.MD5))
            if rhash.get_librhash_version_int() > 0x01040400:
                with self.assertRaises(rhash.InvalidArgumentError):
                    ctx.base32(rhash.SHA1)
                with self.assertRaises(rhash.InvalidArgumentError):
                    ctx.base64(rhash.SHA1)
                with self.assertRaises(rhash.InvalidArgumentError):
                    ctx.hex(rhash.SHA1)
                with self.assertRaises(rhash.InvalidArgumentError):
                    ctx.raw(rhash.SHA1)
        if rhash.get_librhash_version_int() > 0x01040400:
            with self.assertRaises(rhash.InvalidArgumentError):
                with rhash.RHash(rhash.CRC32, rhash.MD5) as ctx:
                    ctx.hash(rhash.SHA1)

    def test_store_and_load(self):
        """Test store/load methods."""
        try:
            with rhash.RHash(rhash.CRC32, rhash.MD5) as ctx1:
                ctx1.update("a")
                data = ctx1.store()
                self.assertTrue(data)
                self.assertTrue(isinstance(data, bytes))
                ctx2 = rhash.RHash.load(data)
                self.assertEqual(data, ctx2.store())
                ctx1.update("bc").finish()
                ctx2.update("bc").finish()
                self.assertEqual("352441c2", ctx2.hash(rhash.CRC32))
                self.assertEqual("900150983cd24fb0d6963f7d28e17f72", ctx2.hash(rhash.MD5))
                self.assertEqual(ctx1.store(), ctx2.store())
        except NotImplementedError:
            pass


if __name__ == "__main__":
    unittest.main()
