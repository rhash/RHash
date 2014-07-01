--TEST--
test RHash global functions
--FILE--
<?php
echo (rhash_count() >= 26) . "\n";
echo rhash_get_digest_size(RHASH_CRC32) . "\n";
echo (int)rhash_is_base32(RHASH_MD5) . "\n";
echo (int)rhash_is_base32(RHASH_AICH) . "\n";
echo rhash_get_name(RHASH_SHA1) . "\n";
echo rhash_msg(RHASH_TTH, 'abc') . "\n";
echo "Done\n";
?>
--EXPECTF--
1
4
0
1
SHA1
asd4ujseh5m47pdyb46kbtsqtsgdklbhyxomuia
Done
