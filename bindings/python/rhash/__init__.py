# Copyright (c) 2021, Aleksey Kravchenko <rhash.admin@gmail.com>
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

"""Python rhash module."""

from .rhash import (
    ALL,
    CRC32,
    CRC32C,
    MD4,
    MD5,
    SHA1,
    TIGER,
    TTH,
    BTIH,
    ED2K,
    AICH,
    WHIRLPOOL,
    RIPEMD160,
    GOST94,
    GOST94_CRYPTOPRO,
    GOST12_256,
    GOST12_512,
    HAS160,
    SHA2_224,
    SHA2_256,
    SHA2_384,
    SHA2_512,
    EDONR256,
    EDONR512,
    SHA3_224,
    SHA3_256,
    SHA3_384,
    SHA3_512,
    BLAKE2S,
    BLAKE2B,
    SNEFRU128,
    SNEFRU256,
    RHash,
    hash_msg,
    hash_file,
    make_magnet,
    get_librhash_version,
)

# Import deprecated constants and functions
from .rhash import SHA224, SHA256, SHA384, SHA512
from .rhash import hash_for_msg, hash_for_file, magnet_for_file


__all__ = [
    "ALL",
    "CRC32",
    "CRC32C",
    "MD4",
    "MD5",
    "SHA1",
    "TIGER",
    "TTH",
    "BTIH",
    "ED2K",
    "AICH",
    "WHIRLPOOL",
    "RIPEMD160",
    "GOST94",
    "GOST94_CRYPTOPRO",
    "GOST12_256",
    "GOST12_512",
    "HAS160",
    "SHA2_224",
    "SHA2_256",
    "SHA2_384",
    "SHA2_512",
    "EDONR256",
    "EDONR512",
    "SHA3_224",
    "SHA3_256",
    "SHA3_384",
    "SHA3_512",
    "BLAKE2S",
    "BLAKE2B",
    "SNEFRU128",
    "SNEFRU256",
    "RHash",
    "hash_msg",
    "hash_file",
    "make_magnet",
    "get_librhash_version",
    "SHA224",
    "SHA256",
    "SHA384",
    "SHA512",
    "hash_for_msg",
    "hash_for_file",
    "magnet_for_file",
]
