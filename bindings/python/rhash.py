# Python Bindings for Librhash
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
# Librhash is (c) 2011, Alexey S Kravchenko <rhash.admin@gmail.com>
# 
# Permission is hereby granted, free of charge,  to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction,  including without limitation the rights
# to  use,  copy,  modify,  merge, publish, distribute, sublicense, and/or sell
# copies  of  the Software,  and  to permit  persons  to whom  the Software  is
# furnished to do so.
# 
# This library  is distributed  in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. Use it at your own risk!

'''Python bindings for librhash

Librhash is a library for computing and verifying hash sums
that supports many hashing algorithms. This module provides
class for incremental hashing, sample usage of it  you  can
see from the following example:

>>> hasher = RHash(CRC32, MD5)
>>> hasher.update('Hello, ')
>>> hasher.update('world!')
>>> hasher.finish()
>>> print hasher.HEX(CRC32)
EBE6C6E6
>>> print hasher.hex(MD5)
6cd3556deb0da54bca060b4c39479839

In this example RHash object is first created for a set  of
hashing  algorithms.  Supported values are CRC32, MD4, MD5,
SHA1, TIGER, TTH, BTIH, ED2K, AICH,  WHIRLPOOL,  RIPEMD160,
GOST, GOST_CRYPTOPRO, HAS160, SNEFRU128, SNEFRU256, SHA224,
SHA256,  SHA384,  SHA512, EDONR256 and EDONR512. Or you can
just use RHash(ALL) to process all of them at once.

Next, data for hashing is  given  in  chunks  with  methods
update(message)  and  update_file(filename).  Finally, call
finish() to end up all remaining calculations.

To receive text represenation of the message digest use one
of  methods  hex(), HEX(), base32(), BASE32(), base64() and
BASE64(). Binary message digest may be obtained with raw().
All of these methods accept algorithm value as argument. It
may be omitted if RHash was created  to  compute  hash  for
only a single hashing algorithm.
'''

# public API
__all__ = ['ALL', 'CRC32', 'MD4', 'MD5', 'SHA1', 'TIGER', 'TTH',
	'BTIH', 'ED2K', 'AICH', 'WHIRLPOOL', 'RIPEMD160', 'GOST',
	'GOST_CRYPTOPRO', 'HAS160', 'SNEFRU128', 'SNEFRU256',
	'SHA224', 'SHA256', 'SHA384', 'SHA512', 'EDONR256', 'EDONR512',
	'RHash']

from ctypes import *

# initialization
librhash = CDLL('librhash.so.1.2')
librhash.rhash_library_init()

# hash_id values
CRC32 = 0x01
MD4   = 0x02
MD5   = 0x04
SHA1  = 0x08
TIGER = 0x10
TTH   = 0x20
BTIH  = 0x40
ED2K  = 0x80
AICH  = 0x100
WHIRLPOOL = 0x200
RIPEMD160 = 0x400
GOST      = 0x800
GOST_CRYPTOPRO = 0x1000
HAS160    = 0x2000
SNEFRU128 = 0x4000
SNEFRU256 = 0x8000
SHA224    = 0x10000
SHA256    = 0x20000
SHA384    = 0x40000
SHA512    = 0x80000
EDONR256  = 0x100000
EDONR512  = 0x200000
ALL       = EDONR512*2 - 1


#rhash_print values
RHPR_RAW    = 1
RHPR_HEX    = 2
RHPR_BASE32 = 3
RHPR_BASE64 = 4
RHPR_UPPERCASE = 8


class RHash(object):
	'Incremental hasher'
	
	def __init__(self, *hash_ids):
		flags = 0
		for hash_id in hash_ids:
			flags |= hash_id
		if flags == 0:
			raise ValueError('Invalid argument')
		self._ctx = librhash.rhash_init(flags)
	
	def __del__(self):
		librhash.rhash_free(self._ctx)
	
	def reset(self):
		librhash.rhash_reset(self._ctx)
	
	def update(self, message):
		message = str(message)
		librhash.rhash_update(self._ctx, message, len(message))
	
	def update_file(self, filename):
		f = open(filename, 'rb')
		buf = f.read(8192)
		while len(buf) > 0:
			self.update(buf)
			buf = f.read(8192)
		f.close()
	
	def finish(self):
		librhash.rhash_final(self._ctx, None)
	
	def _print(self, hash_id, flags):
		buf = create_string_buffer(130)
		ln = librhash.rhash_print(buf, self._ctx, hash_id, flags)
		return str(buf[0:ln])
	
	def raw(self, hash_id = 0):
		return self._print(hash_id, RHPR_RAW)
	
	def hex(self, hash_id = 0):
		return self._print(hash_id, RHPR_HEX)
	
	def HEX(self, hash_id = 0):
		return self._print(hash_id, RHPR_HEX | RHPR_UPPERCASE)
	
	def base32(self, hash_id = 0):
		return self._print(hash_id, RHPR_BASE32)
	
	def BASE32(self, hash_id = 0):
		return self._print(hash_id, RHPR_BASE32 | RHPR_UPPERCASE)
	
	def base64(self, hash_id = 0):
		return self._print(hash_id, RHPR_BASE64)
	
	def BASE64(self, hash_id = 0):
		return self._print(hash_id, RHPR_BASE64 | RHPR_UPPERCASE)

