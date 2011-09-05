use Test::More tests => 12;
BEGIN { use_ok('Rhash') };

#########################
# Rhash module static functions

ok(Rhash::CRC32 > 0);
ok(&Rhash::count() > 0);
is(&Rhash::get_digest_size(Rhash::CRC32), 4);
is(&Rhash::get_hash_length(Rhash::CRC32), 8);
is(&Rhash::is_base32(Rhash::CRC32), 0);
is(&Rhash::is_base32(Rhash::TTH), 1);
is(&Rhash::get_name(Rhash::CRC32), "CRC32");

# test conversion functions
is(&Rhash::raw2hex("test msg"), "74657374206d7367");
is(&Rhash::raw2base32("test msg"), "orsxg5banvzwo");
is(&Rhash::raw2base64("test msg"), "dGVzdCBtc2c=");

$msg = "message digest";

# test msg() hashing method
is(&Rhash::msg(Rhash::MD5, $msg), "f96b697d7cb7938d525a2f31aaf161d0");
