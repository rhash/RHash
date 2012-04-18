--TEST--
test RHash global functions
--FILE--
<?php
echo rhash_count() . "\n";
echo rhash_get_digest_size(RHASH_CRC32) . "\n";
echo rhash_get_hash_length(RHASH_MD4) . "\n";
echo (int)rhash_is_base32(RHASH_MD5) . "\n";
echo (int)rhash_is_base32(RHASH_AICH) . "\n";
echo rhash_get_name(RHASH_SHA1) . "\n";
echo rhash_get_magnet_name(RHASH_TTH) . "\n";
echo "Done\n";
?>
--EXPECTF--
22
4
32
0
1
SHA1
tree:tiger
Done
