#include <ruby.h>
#include <rhash/rhash.h>

/* RHash class. */
static VALUE cRHash;

static void rh_free(rhash ctx) {
	rhash_free(ctx);
}

static VALUE rh_update(VALUE self, VALUE msg) {
	rhash ctx;
	Data_Get_Struct(self, rhash, ctx);
	
	int len;
	char *data = rb_str2cstr(msg, &len);
	
	rhash_update(ctx, data, len);
	return self;
}

static VALUE rh_finish(VALUE self) {
	rhash ctx;
	Data_Get_Struct(self, rhash, ctx);
	rhash_final(ctx, NULL);
	return Qnil;
}

static VALUE rh_reset(VALUE self) {
	rhash ctx;
	Data_Get_Struct(self, rhash, ctx);
	rhash_reset(ctx);
	return Qnil;
}

static VALUE rh_print(VALUE self, VALUE type, int flags) {
	char buf[130];
	rhash ctx;
	Data_Get_Struct(self, rhash, ctx);
	int len = rhash_print(buf, ctx, FIX2INT(type), flags);
	return rb_str_new(buf, len);
}

static VALUE rh_to_raw(VALUE self, VALUE type) {
	return rh_print(self, type, RHPR_RAW);
}

static VALUE rh_to_hex(VALUE self, VALUE type) {
	return rh_print(self, type, RHPR_HEX);
}

static VALUE rh_to_base32(VALUE self, VALUE type) {
	return rh_print(self, type, RHPR_BASE32);
}

static VALUE rh_to_base64(VALUE self, VALUE type) {
	return rh_print(self, type, RHPR_BASE64);
}

static VALUE rh_init(int argc, VALUE *argv, VALUE self) {
	return self;
}

VALUE rh_new(int argc, VALUE* argv, VALUE clz) {
	int flags = 0, i;
	for (i=0; i<argc; i++) {
		flags |= FIX2INT(argv[i]);
	}
	if (!flags) flags = RHASH_ALL_HASHES;
	rhash ctx = rhash_init(flags);
	VALUE newobj = Data_Wrap_Struct(clz, NULL, rh_free, ctx);
	rb_obj_call_init(newobj, argc, argv);
	return newobj;
}

void Init_rhash() {
	rhash_library_init();
	
	cRHash = rb_define_class("RHash", rb_cObject);
	
	rb_define_singleton_method(cRHash, "new", rh_new, -1);
	
	rb_define_method(cRHash, "initialize", rh_init,  -1);
	rb_define_method(cRHash, "update",     rh_update, 1);
	rb_define_method(cRHash, "<<",         rh_update, 1);
	rb_define_method(cRHash, "finish",     rh_finish, 0);
	rb_define_method(cRHash, "reset",      rh_reset,  0);
	rb_define_method(cRHash, "to_raw",     rh_to_raw, 1);
	rb_define_method(cRHash, "to_hex",     rh_to_hex, 1);
	rb_define_method(cRHash, "to_base32",  rh_to_base32, 1);
	rb_define_method(cRHash, "to_base64",  rh_to_base64, 1);
	
	rb_eval_string(
"class RHash \n\
  def update_file(filename) \n\
    f = File.open filename, 'rb' \n\
    f.each_char {|c| update c.to_s} \n\
    f.close \n\
    self \n\
  end \n\
end");
	
	rb_define_const(cRHash, "CRC32",     INT2FIX(RHASH_CRC32));
	rb_define_const(cRHash, "MD4",       INT2FIX(RHASH_MD4));
	rb_define_const(cRHash, "MD5",       INT2FIX(RHASH_MD5));
	rb_define_const(cRHash, "SHA1",      INT2FIX(RHASH_SHA1));
	rb_define_const(cRHash, "TIGER",     INT2FIX(RHASH_TIGER));
	rb_define_const(cRHash, "TTH",       INT2FIX(RHASH_TTH));
	rb_define_const(cRHash, "BTIH",      INT2FIX(RHASH_BTIH));
	rb_define_const(cRHash, "ED2K",      INT2FIX(RHASH_ED2K));
	rb_define_const(cRHash, "AICH",      INT2FIX(RHASH_AICH));
	rb_define_const(cRHash, "WHIRLPOOL", INT2FIX(RHASH_WHIRLPOOL));
	rb_define_const(cRHash, "RIPEMD160", INT2FIX(RHASH_RIPEMD160));
	rb_define_const(cRHash, "GOST",      INT2FIX(RHASH_GOST));
	rb_define_const(cRHash, "GOST_CRYPTOPRO", INT2FIX(RHASH_GOST_CRYPTOPRO));
	rb_define_const(cRHash, "HAS160",    INT2FIX(RHASH_HAS160));
	rb_define_const(cRHash, "SNEFRU128", INT2FIX(RHASH_SNEFRU128));
	rb_define_const(cRHash, "SNEFRU256", INT2FIX(RHASH_SNEFRU256));
	rb_define_const(cRHash, "SHA224",    INT2FIX(RHASH_SHA224));
	rb_define_const(cRHash, "SHA256",    INT2FIX(RHASH_SHA256));
	rb_define_const(cRHash, "SHA384",    INT2FIX(RHASH_SHA384));
	rb_define_const(cRHash, "SHA512",    INT2FIX(RHASH_SHA512));
	rb_define_const(cRHash, "EDONR256",  INT2FIX(RHASH_EDONR256));
	rb_define_const(cRHash, "EDONR512",  INT2FIX(RHASH_EDONR512));
	rb_define_const(cRHash, "ALL",       INT2FIX(RHASH_ALL_HASHES));
}

