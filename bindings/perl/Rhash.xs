#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <rhash/rhash.h>

/* helper macros and functions */
#define BASE32_LENGTH(size) (((size) * 8 + 4) / 5)
#define BASE64_LENGTH(size) ((((size) + 2) / 3) * 4)

void verify_single_bit_hash_id(unsigned hash_id, CV* cv)
{
	const char* error;
	const GV *gv;
	const char *func_name;

	if(0 == (hash_id & RHASH_ALL_HASHES)) {
		error = "%s: unknown hash hash_id = %d";
	} else if(0 != (hash_id & (hash_id - 1))) {
		error = "%s: hash_id is not a single bit: %d";
	} else {
		return; /* success */
	}

	gv = CvGV(cv);
	func_name = (gv ? GvNAME(gv) : "Rhash");
	croak(error, func_name, hash_id);
}

/* allocate a perl string scalar variable, containing buf_size+1 bytes */
SV * allocate_string_buffer(STRLEN buf_size)
{
	SV * sv = newSV(buf_size); /* allocates (buf_size + 1) bytes */
	SvPOK_only(sv);
	SvCUR_set(sv, buf_size);
	return sv;
}

MODULE = Rhash             PACKAGE = Rhash

##############################################################################
# Initialize LibRHash in the module bootstrap function

BOOT:
	rhash_library_init();

##############################################################################
# perl bindings for Hi-level functions

SV *
rhash_msg_raw(hash_id, message)
		unsigned	hash_id
	PROTOTYPE: $$
	PREINIT:
		STRLEN length;
		char out[264];
		int res;
	INPUT:
		char* message = SvPV(ST(1), length);
	CODE:
		verify_single_bit_hash_id(hash_id, cv);
		res = rhash_msg(hash_id, message, length, out);
		if(res < 0) {
			croak("%s: %s", "rhash_msg_raw", strerror(errno));
		}
		RETVAL = newSVpv(out, rhash_get_digest_size(hash_id));
	OUTPUT:
		RETVAL

SV *
rhash_file_raw(hash_id, filepath)
		unsigned hash_id
		char * filepath
	PROTOTYPE: $$
	PREINIT:
		int res;
		char out[264];
	CODE:
		verify_single_bit_hash_id(hash_id, cv);
		res = rhash_file(hash_id, filepath, out);
		if(res < 0) {
			croak("%s: %s: %s", "rhash_file", filepath, strerror(errno));
		}
		RETVAL = newSVpv(out, rhash_get_digest_size(hash_id));
	OUTPUT:
		RETVAL

##############################################################################
# perl bindings for Low-level functions

rhash_context *
rhash_init(hash_id)
		unsigned hash_id
	PROTOTYPE: $

int
rhash_update(ctx, message)
		rhash_context * ctx
	PROTOTYPE: $$
	PREINIT:
		STRLEN length;
	INPUT:
		char* message = SvPV(ST(1), length);
	CODE:
		RETVAL = rhash_update(ctx, message, length);
	OUTPUT:
		RETVAL

int
rhash_final(ctx)
		rhash_context * ctx
	PROTOTYPE: $
	CODE:
		RETVAL = rhash_final(ctx, 0);
	OUTPUT:
		RETVAL

void
rhash_reset(ctx)
		rhash_context * ctx
	PROTOTYPE: $

void
rhash_free(ctx)
		rhash_context * ctx
	PROTOTYPE: $

SV *
rhash_print(ctx, hash_id, flags = 0)
		rhash_context * ctx
		unsigned hash_id
		int flags
	PROTOTYPE: $$;$
	PREINIT:
		int len;
		char out[264];
	CODE:
		if(hash_id != 0) verify_single_bit_hash_id(hash_id, cv);

		len = rhash_print(out, ctx, hash_id, flags);

		/* set exact length to support raw output (RHPR_RAW) */
		RETVAL = newSVpv(out, len);
	OUTPUT:
		RETVAL

unsigned
rhash_get_hash_id(ctx)
		rhash_context * ctx
	PROTOTYPE: $
	CODE:
		RETVAL = ctx->hash_id;
	OUTPUT:
		RETVAL

uint64_t
rhash_get_hashed_length(ctx)
		rhash_context * ctx
	PROTOTYPE: $
	CODE:
		RETVAL = ctx->msg_size;
	OUTPUT:
		RETVAL

##############################################################################
# Hash information functions

int
count()
	CODE:
		RETVAL = rhash_count();
	OUTPUT:
		RETVAL

int
is_base32(hash_id)
		unsigned hash_id
	PROTOTYPE: $
	CODE:
		RETVAL = rhash_is_base32(hash_id);
	OUTPUT:
		RETVAL

int
get_digest_size(hash_id)
		unsigned hash_id
	PROTOTYPE: $
	CODE:
		RETVAL = rhash_get_digest_size(hash_id);
	OUTPUT:
		RETVAL

int
get_hash_length(hash_id)
		unsigned hash_id
	PROTOTYPE: $
	CODE:
		RETVAL = rhash_get_hash_length(hash_id);
	OUTPUT:
		RETVAL

const char *
get_name(hash_id)
		unsigned hash_id
	PROTOTYPE: $
	CODE:
		RETVAL = rhash_get_name(hash_id);
	OUTPUT:
		RETVAL

##############################################################################
# Hash printing functions

##############################################################################
# Hash conversion functions

SV *
raw2hex(bytes)
	PROTOTYPE: $
	PREINIT:
		STRLEN size;
	INPUT:
		unsigned char * bytes = SvPV(ST(0), size);
	CODE:
		RETVAL = allocate_string_buffer(size * 2);
		rhash_print_bytes(SvPVX(RETVAL), bytes, size, RHPR_HEX);
	OUTPUT:
		RETVAL

SV *
raw2base32(bytes)
	PROTOTYPE: $
	PREINIT:
		STRLEN size;
	INPUT:
		unsigned char * bytes = SvPV(ST(0), size);
	CODE:
		RETVAL = allocate_string_buffer(BASE32_LENGTH(size));
		rhash_print_bytes(SvPVX(RETVAL), bytes, size, RHPR_BASE32);
	OUTPUT:
		RETVAL

SV *
raw2base64(bytes)
	PROTOTYPE: $
	PREINIT:
		STRLEN size;
	INPUT:
		unsigned char * bytes = SvPV(ST(0), size);
	CODE:
		RETVAL = allocate_string_buffer(BASE64_LENGTH(size));
		rhash_print_bytes(SvPVX(RETVAL), bytes, size, RHPR_BASE64);
	OUTPUT:
		RETVAL

# rhash_print_bytes should not be used directly
#SV *
#rhash_print_bytes(bytes, flags)
#	PROTOTYPE: $;$
#	PREINIT:
#		STRLEN size;
#	INPUT:
#		unsigned char * bytes = SvPV(ST(0), size);
#		int flags
#	CODE:
#		RETVAL = allocate_string_buffer(size * 2);
#		rhash_print_bytes(SvPVX(RETVAL), bytes, size, flags);
#	OUTPUT:
#		RETVAL

#rhash_uptr_t
#rhash_transmit(msg_id, dst, ldata, rdata)
#	unsigned msg_id
#	void * dst
#	rhash_uptr_t ldata
#	rhash_uptr_t rdata

##############################################################################
# BTIH / BitTorrent support functions

void
rhash_bt_add_filename(ctx, filename, filesize)
		rhash_context * ctx
		char * filename
		uint64_t filesize
	PROTOTYPE: $$$
	CODE:
		rhash_transmit(RMSG_BT_ADD_FILE, ctx, RHASH_STR2UPTR(filename), (rhash_uptr_t)&filesize);

void
rhash_bt_set_piece_length(ctx, piece_length)
		rhash_context * ctx
		unsigned piece_length
	PROTOTYPE: $$
	CODE:
		rhash_transmit(RMSG_BT_SET_PIECE_LENGTH, ctx, RHASH_STR2UPTR(piece_length), 0);

void
rhash_bt_set_private(ctx)
		rhash_context * ctx
	PROTOTYPE: $
	CODE:
		rhash_transmit(RMSG_BT_SET_OPTIONS, ctx, RHASH_BT_OPT_PRIVATE, 0);

SV *
rhash_bt_get_torrent_text(ctx)
		rhash_context * ctx
	PROTOTYPE: $
	PREINIT:
		size_t len;
		char *text;
	CODE:
		len = rhash_transmit(RMSG_BT_GET_TEXT, ctx, RHASH_STR2UPTR(&text), 0);
		if(len == RHASH_ERROR) {
			XSRETURN_UNDEF;
		}
		RETVAL = newSVpv(text, len);
	OUTPUT:
		RETVAL
