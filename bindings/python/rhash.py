# This file is a part of Java Bindings for Librhash
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

librhash is a library for computing and verifying hash sums.
'''

# public API
__all__ = ['CRC32', 'MD4', 'MD5', 'SHA1', 'TIGER', 'TTH',
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


#rhash_print values
RHPR_RAW    = 1
RHPR_HEX    = 2
RHPR_BASE32 = 3
RHPR_BASE64 = 4
RHPR_UPPERCASE = 8


class RHash(object):
	def __init__(self, hash_flags):
		self._ctx = librhash.rhash_init(hash_flags)
		self._flags = hash_flags
		if self._ctx == 0:
			raise ValueError('Invalid argument')
	
	def __del__(self):
		librhash.rhash_free(self._ctx)
	
	def reset(self):
		librhash.rhash_reset(self._ctx)
	
	def update(self, message):
		message = str(message)
		librhash.rhash_update(self._ctx, message, len(message))
	
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
	
	def HEX(self, hash_id = 0)
		return self._print(hash_id, RHPR_HEX | RHPR_UPPERCASE)
	
	def base32(self, hash_id = 0)
		return self._print(hash_id, RHPR_BASE32)
	
	def BASE32(self, hash_id = 0)
		return self._print(hash_id, RHPR_BASE32 | RHPR_UPPERCASE)
	
	def base64(self, hash_id = 0)
		return self._print(hash_id, RHPR_BASE64)
	
	def BASE64(self, hash_id = 0)
		return self._print(hash_id, RHPR_BASE64 | RHPR_UPPERCASE)

