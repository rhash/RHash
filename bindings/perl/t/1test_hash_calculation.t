use Test::More tests => 22;
BEGIN { use_ok('Rhash') };

#########################
# test script

$msg = "message digest";

ok( $r = new Rhash(Rhash::MD5 | Rhash::TTH));
ok( $r->update($msg) );

is( $r->hash(), "f96b697d7cb7938d525a2f31aaf161d0"); # prints the first hash by default
is( $r->hash(Rhash::TTH), "ym432msox5qilih2l4tno62e3o35wygwsbsjoba");
is( $r->hash_hex(Rhash::TTH), "c339bd324ebf6085a0fa5f26d77b44dbb7db60d690649704");
is( $r->hash_hex(Rhash::MD5), "f96b697d7cb7938d525a2f31aaf161d0");
is( $r->hash_rhex(Rhash::MD5), "d061f1aa312f5a528d93b77c7d696bf9");
is( $r->hash_base32(Rhash::MD5), "7fvws7l4w6jy2us2f4y2v4lb2a");
#is( $r->hash_base64(Rhash::MD5), "+WtpfXy3k41SWi8xqvFh0A=");
is( $r->hash_raw(Rhash::MD5), "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61\xd0");
is( $r->reset()->hash(), "d41d8cd98f00b204e9800998ecf8427e");

is( $r->hashed_length(), length($msg));
is( $r->hash_id(), (Rhash::MD5 | Rhash::TTH));
$r = undef; # destruct the Rhash object

#########################
# test hashing a file

$file = "msg.txt";
open FILE, ">$file" or die $!;
binmode FILE;
print FILE $msg;
close FILE;

$r = new Rhash(Rhash::MD5);
is( $r->update_file($file), 14);
is( $r->hash(), "f96b697d7cb7938d525a2f31aaf161d0");
#print  "MD5 (\"$msg\") = ". $r->update_file($file)->hash() . "\n";

is( $r->reset()->update_file($file, 4, 1), 1);
is( $r->hash(), "0cc175b9c0f1b6a831c399e269772661");

open $fd, "<$file" or die $!;
binmode $fd;
is( $r->reset()->update_fd($fd), 14);
is( $r->hash(), "f96b697d7cb7938d525a2f31aaf161d0");
close $fd;
unlink($file);

#########################
# test magnet_link() method
$r = new Rhash(Rhash::ALL);
$r->update("a")->final();
is( $r->magnet_link("test.txt", Rhash::MD5 | Rhash::SHA1), "magnet:?xl=1&dn=test.txt&xt=urn:md5:0cc175b9c0f1b6a831c399e269772661&xt=urn:sha1:q336in72uwt7zyk5dxolt2xk5i3xmz5y");
is( $r->magnet_link(undef, Rhash::ED2K | Rhash::AICH | Rhash::TTH | Rhash::BTIH), "magnet:?xl=1&xt=urn:ed2k:bde52cb31de33e46245e05fbdbd6fb24&xt=urn:aich:q336in72uwt7zyk5dxolt2xk5i3xmz5y&xt=urn:tree:tiger:czquwh3iyxbf5l3bgyugzhassmxu647ip2ike4y&xt=urn:btih:7vai5hicjnmkk6vbge7p6faal74lfror");

$r = new Rhash(Rhash::CRC32 | Rhash::MD4);
$r->update("abc")->final();
is( $r->magnet_link(), "magnet:?xl=3&xt=urn:crc32:352441c2&xt=urn:md4:a448017aaf21d8525fc10ae87aa6729d");

__END__
