lib_LTLIBRARIES = librhash.la
noinst_HEADERS = algorithms.h byte_order.h plug_openssl.h rhash.h rhash_timing.h util.h aich.h crc32.h ed2k.h edonr.h hex.h md4.h md5.h sha1.h sha256.h sha512.h sha3.h \
	ripemd-160.h gost.h has160.h snefru.h tiger.h tth.h torrent.h ustd.h whirlpool.h
include_HEADERS = rhash.h rhash_timing.h
librhash_la_SOURCES = algorithms.c byte_order.c plug_openssl.c rhash.c rhash_timing.c util.c aich.c crc32.c ed2k.c edonr.c hex.c md4.c md5.c sha1.c sha256.c sha512.c sha3.c ripemd-160.c gost.c has160.c snefru.c tiger.c tiger_sbox.c tth.c torrent.c whirlpool.c whirlpool_sbox.c

noinst_PROGRAMS = test_hashes

TESTS = test_hashes

test_hashes_LDADD = librhash.la
noinst_HEADERS += test_hashes.h
