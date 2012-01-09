# Python Bindings for Librhash
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
# Librhash is (c) 2011, Aleksey Kravchenko <rhash.admin@gmail.com>
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

Librhash  is  a library for computing and verifying hash sums
that supports many hashing algorithms. The  simplest  way  to
calculate  hash  of  a message  or  a file is by using one of
the functions:

hash_for_message(message, hash_id)
hash_for_file(filename, hash_id)
magnet_for_file(filename, hash_ids)

Here  hash_id  is one of the constants CRC32, MD4, MD5, SHA1,
TIGER, TTH, BTIH, ED2K,  AICH,  WHIRLPOOL,  RIPEMD160,  GOST,
GOST_CRYPTOPRO, HAS160, SNEFRU128, SNEFRU256, SHA224, SHA256,
SHA384, SHA512, EDONR256 or EDONR512. The first two functions
will  return  the  default text representation of the message
digest they compute. The latter will return magnet  link  for
the  file.  In  this  function  you  can  OR-combine  several
hash_ids, like

>>> print magnet_for_file('rhash.py', CRC32 | MD5)
magnet:?xl=6041&dn=rhash.py&xt=urn:crc32:f5866f7a&xt=urn:md5:f88e6c1620361da9d04e2a2a1c788059

Next, this module provides a class to calculate several hashes
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
with   methods   update(message)  and  update_file(filename).
Finally, call finish() to end up all remaining calculations.

To  receive  text represenation of the message digest use one
of the methods hex(), HEX(), base32(), BASE32(), base64() and
BASE64().  Binary  message digest may be obtained with raw().
All of these methods accept hash_id as argument.  It  may  be
omitted  if  RHash  was  created  to  compute hash for only a
single hashing algorithm.

Method magnet(filename) will generate magnet link with  all
hashes computed by the RHash object.
'''

# public API
__all__ = ['ALL', 'CRC32', 'MD4', 'MD5', 'SHA1', 'TIGER', 'TTH',
	'BTIH', 'ED2K', 'AICH', 'WHIRLPOOL', 'RIPEMD160', 'GOST',
	'GOST_CRYPTOPRO', 'HAS160', 'SNEFRU128', 'SNEFRU256',
	'SHA224', 'SHA256', 'SHA384', 'SHA512', 'EDONR256', 'EDONR512',
	'RHash', 'hash_for_msg', 'hash_for_file', 'magnet_for_file']

import sys
from ctypes import *

# initialization
if sys.platform == 'win32':
	libname = 'librhash.dll'
else:
	libname = 'librhash.so.0'
librhash = CDLL(libname)
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
RHPR_NO_MAGNET = 0x20
RHPR_FILESIZE  = 0x40

class RHash(object):
	'Incremental hasher'
	
	def __init__(self, hash_ids):
		if hash_ids == 0:
			self._ctx = None
			raise ValueError('Invalid argument')
		self._ctx = librhash.rhash_init(hash_ids)
		#switching off autofinal feature
		librhash.rhash_transmit(5, self._ctx, 0, 0)

	def __del__(self):
		if self._ctx != None:
			librhash.rhash_free(self._ctx)
	
	def reset(self):
		librhash.rhash_reset(self._ctx)
	
	def update(self, message):
		message = str(message)
		librhash.rhash_update(self._ctx, message, len(message))
		return self
		
	def __lshift__(self, message):
		return self.update(message)
	
	def update_file(self, filename):
		f = open(filename, 'rb')
		buf = f.read(8192)
		while len(buf) > 0:
			self.update(buf)
			buf = f.read(8192)
		f.close()
		return self
	
	def finish(self):
		librhash.rhash_final(self._ctx, None)
	
	def _print(self, hash_id, flags):
		buf = create_string_buffer(130)
		ln = librhash.rhash_print(buf, self._ctx, hash_id, flags)
		return buf[0:ln]
	
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
	
	def magnet(self, filepath):
		ln = librhash.rhash_print_magnet(None, filepath, self._ctx, ALL, RHPR_FILESIZE)
		buf = create_string_buffer(ln)
		librhash.rhash_print_magnet(buf, filepath, self._ctx, ALL, RHPR_FILESIZE)
		return buf[0:ln-1]
	
	def __str__(self, hash_id = 0):
		return self._print(hash_id, 0)

def hash_for_msg(message, hash_id):
	h = RHash(hash_id)
	h.update(message).finish()
	return str(h)

def hash_for_file(filename, hash_id):
	h = RHash(hash_id)
	h.update_file(filename).finish()
	return str(h)	

def magnet_for_file(filename, hash_mask):
	h = RHash(hash_mask)
	h.update_file(filename).finish()
	return h.magnet(filename)
