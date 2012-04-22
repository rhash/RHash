--TEST--
test RHash file   methods
--FILE--
<?php
$filename = '003_test.txt';

$handle = fopen($filename, "w+b");
fwrite($handle, "012abc678");
$r = new RHash(RHASH_MD5);
$r->update_stream($handle, 3, 3);
echo $r->final()->magnet($filename) . "\n";
fclose($handle);

$r->reset()->update_file($filename);
echo $r->final()->magnet($filename) . "\n";
$r->reset()->update_file($filename, 2, 1);
echo $r->final()->magnet($filename) . "\n";

// test global file reading functions
echo rhash_file(RHASH_TTH, $filename) . "\n";
echo rhash_magnet(RHASH_TTH, $filename) . "\n";
unlink($filename);
echo "Done\n";
?>
--EXPECTF--
magnet:?xl=3&dn=003_test.txt&xt=urn:md5:900150983cd24fb0d6963f7d28e17f72
magnet:?xl=12&dn=003_test.txt&xt=urn:md5:5a73501e89118bfc45e19f26c0bb2c33
magnet:?xl=13&dn=003_test.txt&xt=urn:md5:c81e728d9d4c2f636f067f89cc14862c
2yu5rdna7qwl6e3ivtgzleuft766u72dxnlzdri
magnet:?xl=9&dn=003_test.txt&xt=urn:tree:tiger:2yu5rdna7qwl6e3ivtgzleuft766u72dxnlzdri
Done
