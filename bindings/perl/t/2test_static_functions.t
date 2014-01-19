use Test::More tests => 11;
use Crypt::Rhash;# qw( :Functions );

#########################
# Rhash module static functions

ok(RHASH_CRC32 > 0);
ok(&Crypt::Rhash::count() > 0);
is(&Crypt::Rhash::get_digest_size(RHASH_CRC32), 4);
is(&Crypt::Rhash::get_hash_length(RHASH_CRC32), 8);
is(&Crypt::Rhash::is_base32(RHASH_CRC32), 0);
is(&Crypt::Rhash::is_base32(RHASH_TTH), 1);
is(&Crypt::Rhash::get_name(RHASH_CRC32), "CRC32");

# test conversion functions
is(&raw2hex("test msg"), "74657374206d7367");
is(&raw2base32("test msg"), "orsxg5banvzwo");
is(&raw2base64("test msg"), "dGVzdCBtc2c=");

$msg = "message digest";

# test msg() hashing method
is(&Crypt::Rhash::msg(RHASH_MD5, $msg), "f96b697d7cb7938d525a2f31aaf161d0");
