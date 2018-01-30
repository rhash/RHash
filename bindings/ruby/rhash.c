/*
 * Ruby Bindings for Librhash
 * Copyright (c) 2011-2012, Sergey Basalaev <sbasalaev@gmail.com>
 * Librhash is (c) 2011-2012, Aleksey Kravchenko <rhash.admin@gmail.com>
 * 
 * Permission is hereby granted, free of charge,  to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction,  including without limitation the rights
 * to  use,  copy,  modify,  merge, publish, distribute, sublicense, and/or sell
 * copies  of  the Software,  and  to permit  persons  to whom  the Software  is
 * furnished to do so.
 * 
 * This library  is distributed  in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. Use it at your own risk!
 */

#include <ruby.h>
#include <rhash.h>

/* RHash class. */
static VALUE cRHash;

static void rh_free(rhash ctx) {
	rhash_free(ctx);
}

/**
 * call-seq:
 *   rhash.update(data) -> RHash
 *   rhash << data      -> RHash
 *
 * Updates this <code>RHash</code> with new data chunk.
 */
static VALUE rh_update(VALUE self, VALUE msg) {
	rhash ctx;
	Data_Get_Struct(self, struct rhash_context, ctx);

	if (TYPE(msg) != T_STRING) {
		msg = rb_obj_as_string(msg); /* convert to string */
	}

	rhash_update(ctx, RSTRING_PTR(msg), RSTRING_LEN(msg));
	return self;
}

/* declaring non-static method to fix a warning on an unused function */
VALUE rh_update_file(VALUE self, VALUE file);

/**
 * call-seq:
 *   rhash.update_file(filename) -> RHash
 * 
 * Updates this <code>RHash</code> with data from given file.
 */
VALUE rh_update_file(VALUE self, VALUE file) {
	// this function is actually implemented in pure Ruby below
	// this allows us to handle files in platform-independent way
	return self;
}

/**
 * call-seq:
 *   rhash.finish
 *
 * Finishes calculation for all data buffered by
 * <code>update</code> and stops calculation of hashes.
 */
static VALUE rh_finish(VALUE self) {
	rhash ctx;
	Data_Get_Struct(self, struct rhash_context, ctx);
	rhash_final(ctx, NULL);
	return self;
}

/**
 * call-seq:
 *   rhash.reset
 *
 * Resets this RHash to initial state.
 * The RHash becomes available to process
 * new data chunks.
 */
static VALUE rh_reset(VALUE self) {
	rhash ctx;
	Data_Get_Struct(self, struct rhash_context, ctx);
	rhash_reset(ctx);
	return self;
}

static VALUE rh_print(VALUE self, VALUE type, int flags) {
	char buf[130];
	rhash ctx;
	Data_Get_Struct(self, struct rhash_context, ctx);
	int len = rhash_print(buf, ctx, type == Qnil ? 0 : FIX2INT(type), flags);
	return rb_str_new(buf, len);
}

/**
 * call-seq:
 *   rhash.to_raw(id)
 *   rhash.to_raw
 *
 * Returns value of the RHash digest as raw bytes.
 * If RHash was created with a single hashing algorithm
 * then argument may be omitted.
 */
static VALUE rh_to_raw(int argc, VALUE* argv, VALUE self) {
	VALUE type;
	rb_scan_args(argc, argv, "01", &type);
	return rh_print(self, type, RHPR_RAW);
}

/**
 * call-seq:
 *   rhash.to_hex(id)
 *   rhash.to_hex
 *
 * Returns value of the RHash digest as hexadecimal string.
 * If RHash was created with a single hashing algorithm
 * then argument may be omitted.
 */
static VALUE rh_to_hex(int argc, VALUE* argv, VALUE self) {
	VALUE type;
	rb_scan_args(argc, argv, "01", &type);
	return rh_print(self, type, RHPR_HEX);
}

/**
 * call-seq:
 *   rhash.to_base32(id)
 *   rhash.to_base32
 *
 * Returns value of the RHash digest as base32 string.
 * If RHash was created with a single hashing algorithm
 * then argument may be omitted.
 */
static VALUE rh_to_base32(int argc, VALUE* argv, VALUE self) {
	VALUE type;
	rb_scan_args(argc, argv, "01", &type);
	return rh_print(self, type, RHPR_BASE32);
}

/**
 * call-seq:
 *   rhash.magnet(filepath)
 *   rhash.magnet
 *
 * Returns magnet link with all hashes computed by
 * the RHash object.
 * if filepath is specified, then it is url-encoded
 * and included into the resulting magnet link.
 */
static VALUE rh_magnet(int argc, VALUE* argv, VALUE self) {
	VALUE value;
	const char* filepath = 0;
	char* buf;
	size_t buf_size;
	rhash ctx;

	Data_Get_Struct(self, struct rhash_context, ctx);

	rb_scan_args(argc, argv, "01", &value);
	if (value != Qnil) {
		if (TYPE(value) != T_STRING) value = rb_obj_as_string(value);
		filepath = RSTRING_PTR(value);
	}

	buf_size = rhash_print_magnet(0, filepath, ctx, RHASH_ALL_HASHES, RHPR_FILESIZE);
	buf = (char*)malloc(buf_size);
	if (!buf) return Qnil;

	rhash_print_magnet(buf, filepath, ctx, RHASH_ALL_HASHES, RHPR_FILESIZE);
	value = rb_str_new2(buf);
	free(buf);
	return value;
}

/**
 * call-seq:
 *   rhash.to_base64(id)
 *   rhash.to_base64
 *
 * Returns value of the RHash digest as base64 string.
 * If RHash was created with a single hashing algorithm
 * then argument may be omitted.
 */
static VALUE rh_to_base64(int argc, VALUE* argv, VALUE self) {
	VALUE type;
	rb_scan_args(argc, argv, "01", &type);
	return rh_print(self, type, RHPR_BASE64);
}

/**
 * call-seq:
 *   rhash.to_s(id)
 *   rhash.to_s
 *
 * Returns value of the RHash digest for given algorithm
 * as string in default format. If RHash was created with
 * a single hashing algorithm then argument may be omitted.
 */
static VALUE rh_to_s(int argc, VALUE* argv, VALUE self) {
	VALUE type;
	rb_scan_args(argc, argv, "01", &type);
	return rh_print(self, type, 0);
}

/**
 * call-seq:
 *   RHash.base32?(id) -> true or false
 *
 * Returns true if default format for given hash algorithm is
 * base32 and false if it is hexadecimal.
 */
static VALUE rh_is_base32(VALUE self, VALUE type) {
	return rhash_is_base32(FIX2INT(type)) ? Qtrue : Qfalse;
}

static VALUE rh_init(int argc, VALUE *argv, VALUE self) {
	return self;
}

/**
 * call-seq:
 *   RHash.new(id, ...)
 *
 * Creates RHash object to calculate hashes for given algorithms.
 * Parameters should be constants defined in this class.
 */
VALUE rh_new(int argc, VALUE* argv, VALUE clz) {
	int flags = 0, i;
	for (i=0; i<argc; i++) {
		flags |= FIX2INT(argv[i]);
	}
	if (!flags) flags = RHASH_ALL_HASHES;
	rhash ctx = rhash_init(flags);
	rhash_set_autofinal(ctx, 0);
	VALUE newobj = Data_Wrap_Struct(clz, NULL, rh_free, ctx);
	rb_obj_call_init(newobj, argc, argv);
	return newobj;
}

/**
 * Librhash is a library for computing and verifying hash sums
 * that supports many hashing algorithms. This module provides
 * class for incremental hashing that utilizes the library.
 * Sample usage of it you can see from the following example:
 * 
 *   hasher = RHash.new(RHash::CRC32, RHash::MD5)
 *   hasher.update('Hello, ')
 *   hasher << 'world' << '!'
 *   hasher.finish
 *   puts hasher.to_hex RHash::CRC32
 *   puts hasher.to_base32 RHash::MD5
 *
 * which produces
 *
 *   ebe6c6e6
 *   ntjvk3plbwsuxsqgbngdsr4yhe
 *
 * In this example <code>RHash</code> object is first created
 * for a set of hashing algorithms.
 *
 * Next, data for hashing is  given  in  chunks  with  methods
 * <code>update</code> and <code>update_file</code>. Finally,
 * call <code>finish</code> to end up all remaining calculations.
 *
 * To receive text represenation of the message digest use one
 * of methods <code>to_hex</code>, <code>to_base32</code> and
 * <code>to_base64</code>. Binary message digest may be obtained
 * with <code>to_raw</code>. All of these methods accept algorithm
 * value as argument. It may be omitted if <code>RHash</code> was
 * created to compute hash for only a single hashing algorithm.
 */
void Init_rhash() {
	rhash_library_init();
	
	cRHash = rb_define_class("RHash", rb_cObject);
	
	rb_define_singleton_method(cRHash, "new", rh_new, -1);
	rb_define_singleton_method(cRHash, "base32?", rh_is_base32, 1);
	
	rb_define_method(cRHash, "initialize", rh_init,  -1);
	rb_define_method(cRHash, "update",     rh_update, 1);
	rb_define_method(cRHash, "<<",         rh_update, 1);
	rb_define_method(cRHash, "finish",     rh_finish, 0);
	rb_define_method(cRHash, "reset",      rh_reset,  0);
	rb_define_method(cRHash, "to_raw",     rh_to_raw, -1);
	rb_define_method(cRHash, "to_hex",     rh_to_hex, -1);
	rb_define_method(cRHash, "to_base32",  rh_to_base32, -1);
	rb_define_method(cRHash, "to_base64",  rh_to_base64, -1);
	rb_define_method(cRHash, "to_s",       rh_to_s, -1);
	rb_define_method(cRHash, "magnet",     rh_magnet, -1);
	
	rb_eval_string(
"class RHash \n\
  def update_file(filename) \n\
    f = File.open(filename, 'rb') \n\
    while block = f.read(4096) \n\
      self.update(block) \n\
    end \n\
    f.close \n\
    self \n\
  end \n\
end\n\
\n\
def RHash.hash_for_msg(msg, hash_id)\n\
  RHash.new(hash_id).update(msg).finish.to_s\n\
end\n\
\n\
def RHash.hash_for_file(filename, hash_id)\n\
  RHash.new(hash_id).update_file(filename).finish.to_s\n\
end\n\
\n\
def RHash.magnet_for_file(filename, *hash_ids)\n\
  RHash.new(*hash_ids).update_file(filename).finish.magnet(filename)\n\
end");
	
	/** CRC32 checksum. */
	rb_define_const(cRHash, "CRC32",     INT2FIX(RHASH_CRC32));
	/** MD4 hash. */
	rb_define_const(cRHash, "MD4",       INT2FIX(RHASH_MD4));
	/** MD5 hash. */
	rb_define_const(cRHash, "MD5",       INT2FIX(RHASH_MD5));
	/** SHA-1 hash. */
	rb_define_const(cRHash, "SHA1",      INT2FIX(RHASH_SHA1));
	/** Tiger hash. */
	rb_define_const(cRHash, "TIGER",     INT2FIX(RHASH_TIGER));
	/** Tiger tree hash */
	rb_define_const(cRHash, "TTH",       INT2FIX(RHASH_TTH));
	/** BitTorrent info hash. */
	rb_define_const(cRHash, "BTIH",      INT2FIX(RHASH_BTIH));
	/** EDonkey 2000 hash. */
	rb_define_const(cRHash, "ED2K",      INT2FIX(RHASH_ED2K));
	/** eMule AICH. */
	rb_define_const(cRHash, "AICH",      INT2FIX(RHASH_AICH));
	/** Whirlpool hash. */
	rb_define_const(cRHash, "WHIRLPOOL", INT2FIX(RHASH_WHIRLPOOL));
	/** RIPEMD-160 hash. */
	rb_define_const(cRHash, "RIPEMD160", INT2FIX(RHASH_RIPEMD160));
	/** GOST R 34.11-94. */
	rb_define_const(cRHash, "GOST",      INT2FIX(RHASH_GOST));
	/** GOST R 34.11-94. */
	rb_define_const(cRHash, "GOST_CRYPTOPRO", INT2FIX(RHASH_GOST_CRYPTOPRO));
	/** HAS-160 hash. */
	rb_define_const(cRHash, "HAS160",    INT2FIX(RHASH_HAS160));
	/** Snefru-128 hash. */
	rb_define_const(cRHash, "SNEFRU128", INT2FIX(RHASH_SNEFRU128));
	/** Snefru-256 hash. */
	rb_define_const(cRHash, "SNEFRU256", INT2FIX(RHASH_SNEFRU256));
	/** SHA-224 hash. */
	rb_define_const(cRHash, "SHA224",    INT2FIX(RHASH_SHA224));
	/** SHA-256 hash. */
	rb_define_const(cRHash, "SHA256",    INT2FIX(RHASH_SHA256));
	/** SHA-384 hash. */
	rb_define_const(cRHash, "SHA384",    INT2FIX(RHASH_SHA384));
	/** SHA-512 hash. */
	rb_define_const(cRHash, "SHA512",    INT2FIX(RHASH_SHA512));
	/** EDON-R 256. */
	rb_define_const(cRHash, "EDONR256",  INT2FIX(RHASH_EDONR256));
	/** EDON-R 512. */
	rb_define_const(cRHash, "EDONR512",  INT2FIX(RHASH_EDONR512));
	/** SHA3-224 hash. */
	rb_define_const(cRHash, "SHA3_224",    INT2FIX(RHASH_SHA3_224));
	/** SHA3-256 hash. */
	rb_define_const(cRHash, "SHA3_256",    INT2FIX(RHASH_SHA3_256));
	/** SHA3-384 hash. */
	rb_define_const(cRHash, "SHA3_384",    INT2FIX(RHASH_SHA3_384));
	/** SHA3-512 hash. */
	rb_define_const(cRHash, "SHA3_512",    INT2FIX(RHASH_SHA3_512));
	/** Create RHash with this parameter to compute hashes for all available algorithms. */
	rb_define_const(cRHash, "ALL",       INT2FIX(RHASH_ALL_HASHES));
}

