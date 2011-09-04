use Test::More tests => 17;
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

# test internal static functions
ok( ($ctx = &Rhash::rhash_init(Rhash::MD5 | Rhash::SHA1)) );
is(&Rhash::rhash_update($ctx, $msg), 0);
is(&Rhash::rhash_final($ctx), 0);
is(&Rhash::rhash_print($ctx, Rhash::MD5), "f96b697d7cb7938d525a2f31aaf161d0");
is(&Rhash::rhash_print($ctx, Rhash::SHA1), "c12252ceda8be8994d5fa0290a47231c1d16aae3");
&Rhash::rhash_free($ctx);

is(&Rhash::rhash_msg(Rhash::MD5, $msg), "f96b697d7cb7938d525a2f31aaf161d0");
