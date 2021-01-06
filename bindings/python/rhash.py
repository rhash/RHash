# Python Bindings for Librhash
#
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com> and
#                     Aleksey Kravchenko <rhash.admin@gmail.com>
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

"""Python bindings for librhash.

Librhash  is  a library  for  computing  message digests  and
magnet links for various hash functions. The simplest  way to
calculate a message digest of  a string  or  file is by using
one of the functions:

hash_msg(message, hash_id)
hash_file(filepath, hash_id)
make_magnet(filepath, hash_ids)

Here  hash_id  is one of the constants CRC32, CRC32C, MD4, MD5,
SHA1, TIGER, TTH, BTIH, ED2K, AICH,  WHIRLPOOL, RIPEMD160,
GOST94, GOST94_CRYPTOPRO, GOST12_256, GOST12_512, HAS160,
SHA224, SHA256, SHA384, SHA512, SHA3_224, SHA3_256, SHA3_384, SHA3_512,
BLAKE2S, BLAKE2B, EDONR256, EDONR512, SNEFRU128, SNEFRU256.
The first two functions will return the default text representation
of the message digest they compute.  The latter will return the
magnet link  for the  file. In this function  you can OR-combine
several hash_ids, like

>>> print make_magnet('rhash.py', CRC32 | MD5)
magnet:?xl=6041&dn=rhash.py&xt=urn:crc32:f5866f7a&xt=urn:md5:
f88e6c1620361da9d04e2a2a1c788059

Next, this module provides a class to calculate several message digests
simultaneously in an incremental way. Example of using it:

>>> hasher = RHash(CRC32 | MD5)
>>> hasher.update('Hello, ')
>>> hasher << 'world' << '!'
>>> hasher.finish()
>>> print hasher.HEX(CRC32)
EBE6C6E6
>>> print hasher.hex(MD5)
6cd3556deb0da54bca060b4c39479839

In this example RHash object is first created for  a  set  of
hashing algorithms. Then, data for hashing is given in chunks
with   methods   update(message)  and  update_file(filepath).
Finally, call finish() to end up all remaining calculations.

To  receive  text represenation of the message digest use one
of the methods hex(), HEX(), base32(), BASE32(), base64() and
BASE64().  The hash() method  outputs  message digest  in its
default format.  Binary  message digest may be obtained  with
raw().  All of these  methods  accept  hash_id  as  argument,
hash_id can be omitted  if the instance of RHash  was created
to compute message digest of a single hash function.

Method  magnet(filepath) will generate magnet link containing
message digests computed by the RHash object.
"""

# public API
__all__ = [
    'ALL', 'CRC32', 'CRC32C', 'MD4', 'MD5', 'SHA1', 'TIGER', 'TTH',
    'BTIH', 'ED2K', 'AICH', 'WHIRLPOOL', 'RIPEMD160', 'GOST94',
    'GOST94_CRYPTOPRO', 'GOST12_256', 'GOST12_512', 'HAS160',
    'SHA224', 'SHA256', 'SHA384', 'SHA512', 'EDONR256', 'EDONR512',
    'SHA3_224', 'SHA3_256', 'SHA3_384', 'SHA3_512', 'BLAKE2S', 'BLAKE2B',
    'SNEFRU128', 'SNEFRU256',
    'RHash', 'hash_msg', 'hash_file', 'make_magnet',
    'hash_for_msg', 'hash_for_file', 'magnet_for_file']

import sys
from ctypes import (
    CDLL, c_char_p, c_int, c_size_t, c_uint, c_void_p, create_string_buffer)

# initialization
if sys.platform == 'win32':
    LIBNAME = 'librhash.dll'
elif sys.platform == 'darwin':
    LIBNAME = 'librhash.0.dylib'
elif sys.platform == 'cygwin':
    LIBNAME = 'cygrhash.dll'
elif sys.platform == 'msys':
    LIBNAME = 'msys-rhash.dll'
else:
    LIBNAME = 'librhash.so.0'
LIBRHASH = CDLL(LIBNAME)
LIBRHASH.rhash_library_init()

# function prototypes
LIBRHASH.rhash_init.argtypes = [c_uint]
LIBRHASH.rhash_init.restype = c_void_p
LIBRHASH.rhash_free.argtypes = [c_void_p]
LIBRHASH.rhash_reset.argtypes = [c_void_p]
LIBRHASH.rhash_update.argtypes = [c_void_p, c_char_p, c_size_t]
LIBRHASH.rhash_final.argtypes = [c_void_p, c_char_p]
LIBRHASH.rhash_print.argtypes = [c_char_p, c_void_p, c_uint, c_int]
LIBRHASH.rhash_print.restype = c_size_t
LIBRHASH.rhash_print_magnet.argtypes = [
    c_char_p, c_char_p, c_void_p, c_uint, c_int]
LIBRHASH.rhash_print_magnet.restype = c_size_t
LIBRHASH.rhash_transmit.argtypes = [c_uint, c_void_p, c_size_t, c_size_t]

# conversion of a string to binary data with Python 2/3 compatibility
if sys.version < '3':
    def _s2b(string):
        """Python 2: just return the string."""
        return string
    def _msg_to_bytes(msg):
        """Convert the msg parameter to a string."""
        if isinstance(msg, str):
            return msg
        return str(msg)
else:
    import codecs
    def _s2b(string):
        """Python 3: convert the string to binary data."""
        return codecs.utf_8_encode(string)[0]
    def _msg_to_bytes(msg):
        """Convert the msg parameter to binary data."""
        if isinstance(msg, bytes):
            return msg
        if isinstance(msg, str):
            return _s2b(msg)
        return _s2b(str(msg))

# hash_id values
CRC32 = 0x01
MD4 = 0x02
MD5 = 0x04
SHA1 = 0x08
TIGER = 0x10
TTH = 0x20
BTIH = 0x40
ED2K = 0x80
AICH = 0x100
WHIRLPOOL = 0x200
RIPEMD160 = 0x400
GOST94 = 0x800
GOST94_CRYPTOPRO = 0x1000
HAS160 = 0x2000
GOST12_256 = 0x4000
GOST12_512 = 0x8000
SHA224 = 0x10000
SHA256 = 0x20000
SHA384 = 0x40000
SHA512 = 0x80000
EDONR256 = 0x100000
EDONR512 = 0x200000
SHA3_224 = 0x0400000
SHA3_256 = 0x0800000
SHA3_384 = 0x1000000
SHA3_512 = 0x2000000
CRC32C   = 0x4000000
SNEFRU128 = 0x08000000
SNEFRU256 = 0x10000000
BLAKE2S = 0x20000000
BLAKE2B = 0x40000000
ALL = 0x7FFFFFFF

#rhash_print values
RHPR_RAW = 1
RHPR_HEX = 2
RHPR_BASE32 = 3
RHPR_BASE64 = 4
RHPR_UPPERCASE = 8
RHPR_NO_MAGNET = 0x20
RHPR_FILESIZE = 0x40

RMSG_SET_AUTOFINAL = 5

class RHash(object):
    """Class to compute message digests and magnet links."""

    def __init__(self, hash_ids):
        """Construct RHash object."""
        if hash_ids == 0:
            self._ctx = None
            raise ValueError('Invalid argument')
        self._ctx = LIBRHASH.rhash_init(hash_ids)
        #switching off the autofinal feature
        LIBRHASH.rhash_transmit(RMSG_SET_AUTOFINAL, self._ctx, 0, 0)

    def __del__(self):
        """Cleanup allocated resources."""
        if self._ctx != None:
            LIBRHASH.rhash_free(self._ctx)

    def reset(self):
        """Reset this object to initial state."""
        LIBRHASH.rhash_reset(self._ctx)
        return self

    def update(self, message):
        """Update this object with new data chunk."""
        data = _msg_to_bytes(message)
        LIBRHASH.rhash_update(self._ctx, data, len(data))
        return self

    def __lshift__(self, message):
        """Update this object with new data chunk."""
        return self.update(message)

    def update_file(self, filepath):
        """Update this object with data from the given file."""
        file = open(filepath, 'rb')
        buf = file.read(8192)
        while len(buf) > 0:
            self.update(buf)
            buf = file.read(8192)
        file.close()
        return self

    def finish(self):
        """Flush buffered data and calculate message digests."""
        LIBRHASH.rhash_final(self._ctx, None)
        return self

    def _print(self, hash_id, flags):
        """Retrieve the message digest in the specified format."""
        buf = create_string_buffer(130)
        size = LIBRHASH.rhash_print(buf, self._ctx, hash_id, flags)
        if (flags & 3) == RHPR_RAW:
            return buf[0:size]
        else:
            return buf[0:size].decode()

    def raw(self, hash_id=0):
        """Return the message digest as raw binary data."""
        return self._print(hash_id, RHPR_RAW)

    def hex(self, hash_id=0):
        """Return the message digest as a hexadecimal lower-case string."""
        return self._print(hash_id, RHPR_HEX)

    def base32(self, hash_id=0):
        """Return the message digest as a Base32 lower-case string."""
        return self._print(hash_id, RHPR_BASE32)

    def base64(self, hash_id=0):
        """Return the message digest as a Base64 string."""
        return self._print(hash_id, RHPR_BASE64)

    # pylint: disable=invalid-name
    def HEX(self, hash_id=0):
        """Return the message digest as a hexadecimal upper-case string."""
        return self._print(hash_id, RHPR_HEX | RHPR_UPPERCASE)

    def BASE32(self, hash_id=0):
        """Return the message digest as a Base32 upper-case string."""
        return self._print(hash_id, RHPR_BASE32 | RHPR_UPPERCASE)
    # pylint: enable=invalid-name

    def magnet(self, filepath):
        """Return magnet link with all message digests computed by this object."""
        size = LIBRHASH.rhash_print_magnet(
            None, _s2b(filepath), self._ctx, ALL, RHPR_FILESIZE)
        buf = create_string_buffer(size)
        LIBRHASH.rhash_print_magnet(
            buf, _s2b(filepath), self._ctx, ALL, RHPR_FILESIZE)
        return buf[0:size-1].decode('utf-8')

    def hash(self, hash_id=0):
        """Return the message digest for the given hash function as a string in the default format."""
        return self._print(hash_id, 0)

    def __str__(self):
        """Return the message digest."""
        return self._print(0, 0)

# simplified interface functions

def hash_msg(message, hash_id):
    """Compute and return the message digest (in its default format) of the message."""
    handle = RHash(hash_id)
    handle.update(message).finish()
    return str(handle)

def hash_file(filepath, hash_id):
    """Compute and return the message digest (in its default format) of the file content."""
    handle = RHash(hash_id)
    handle.update_file(filepath).finish()
    return str(handle)

def make_magnet(filepath, hash_mask):
    """Compute and return the magnet link for the file."""
    handle = RHash(hash_mask)
    handle.update_file(filepath).finish()
    return handle.magnet(filepath)

# depricated functions

def _deprecation(message):
    warnings.warn(message, category=DeprecationWarning, stacklevel=2)

def hash_for_msg(message, hash_id):
    """Depricated function to compute a hash of a message."""
    _deprecation("Call to deprecated function hash_for_msg(), should use hash_msg().")
    return hash_msg(message, hash_id)

def hash_for_file(filepath, hash_id):
    """Depricated function to compute a hash of a file."""
    _deprecation("Call to deprecated function hash_for_file(), should use hash_file().")
    return hash_file(filepath, hash_id)

def magnet_for_file(filepath, hash_mask):
    """Depricated function to compute a magnet link for a file."""
    _deprecation("Call to deprecated function magnet_for_file(), should use make_magnet().")
    return make_magnet(filepath, hash_mask)
