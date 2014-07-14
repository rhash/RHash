/* php_rhash.c */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rhash.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_streams.h"
#include "php_rhash.h"

#define PHP_RHASH_VERSION "1.2.9"

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO(arginfo_rhash_count, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_get_digest_size, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_is_base32, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_get_name, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_msg, 0)
	ZEND_ARG_INFO(0, hash_id)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_file, 0)
	ZEND_ARG_INFO(0, hash_id)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_magnet_func, 0)
	ZEND_ARG_INFO(0, hash_id)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ The table of global functions
*/
static zend_function_entry rhash_functions[] = {
	PHP_FE(rhash_count, arginfo_rhash_count)
	PHP_FE(rhash_get_digest_size, arginfo_rhash_get_digest_size)
	PHP_FE(rhash_is_base32, arginfo_rhash_is_base32)
	PHP_FE(rhash_get_name, arginfo_rhash_get_name)
	PHP_FE(rhash_msg, arginfo_rhash_msg)
	PHP_FE(rhash_file, arginfo_rhash_file)
	PHP_FE(rhash_magnet, arginfo_rhash_magnet_func)
	PHP_FE_END
};
/* }}} */


/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash__construct, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_update, 0)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_update_stream, 0, 0, 1)
	ZEND_ARG_INFO(0, handle)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_update_file, 0, 0, 1)
	ZEND_ARG_INFO(0, path)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_final, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_reset, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_rhash_hashed_length, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_hash, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_raw, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_hex, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_base32, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_base64, 0, 0, 0)
	ZEND_ARG_INFO(0, hash_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rhash_magnet, 0, 0, 0)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ The table of the RHash class methods
*/
zend_function_entry rhash_methods[] = {
	PHP_ME(RHash,  __construct,     arginfo_rhash__construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(RHash,  update,          arginfo_rhash_update, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  update_stream,   arginfo_rhash_update_stream, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  update_file,     arginfo_rhash_update_file, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  final,           arginfo_rhash_final, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  reset,           arginfo_rhash_reset, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  hashed_length,   arginfo_rhash_hashed_length, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  hash,            arginfo_rhash_hash, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  raw,             arginfo_rhash_raw, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  hex,             arginfo_rhash_hex, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  base32,          arginfo_rhash_base32, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  base64,          arginfo_rhash_base64, ZEND_ACC_PUBLIC)
	PHP_ME(RHash,  magnet,          arginfo_rhash_magnet, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */

/* RHash class wrapper */
typedef struct rhash_object {
	zend_object std;
	rhash rhash;
} rhash_object;

zend_class_entry *rhash_ce;
zend_object_handlers rhash_object_handlers;
static zend_object_value rhash_create_handler(zend_class_entry *type TSRMLS_DC);
static void rhash_free_storage(void *object TSRMLS_DC);

/* {{{ Module struct
*/
zend_module_entry rhash_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"rhash",                   /* extension name */
	rhash_functions,           /* function list */
	PHP_MINIT(rhash),          /* process startup */
	PHP_MSHUTDOWN(rhash),      /* process shutdown */
	NULL,
	NULL,
	PHP_MINFO(rhash),          /* extension info */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_RHASH_VERSION,         /* extension version */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_RHASH
ZEND_GET_MODULE(rhash)
#endif

#define REGISTER_RHASH_CONSTANT(c) REGISTER_LONG_CONSTANT(#c, c, CONST_CS | CONST_PERSISTENT)
#define RHASH_ALL RHASH_ALL_HASHES

/* {{{ PHP_MINIT_FUNCTION(rhash) */
PHP_MINIT_FUNCTION(rhash)
{
	zend_class_entry ce;
	rhash_library_init(); /* initialize LibRHash */

	/* register RHash class, its methods and handlers */
	INIT_CLASS_ENTRY(ce, "RHash", rhash_methods);
	rhash_ce = zend_register_internal_class(&ce TSRMLS_CC);
	rhash_ce->create_object = rhash_create_handler;
	memcpy(&rhash_object_handlers,
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	rhash_object_handlers.clone_obj = NULL;

	REGISTER_RHASH_CONSTANT(RHASH_CRC32);
	REGISTER_RHASH_CONSTANT(RHASH_MD4);
	REGISTER_RHASH_CONSTANT(RHASH_MD5);
	REGISTER_RHASH_CONSTANT(RHASH_SHA1);
	REGISTER_RHASH_CONSTANT(RHASH_TIGER);
	REGISTER_RHASH_CONSTANT(RHASH_TTH);
	REGISTER_RHASH_CONSTANT(RHASH_BTIH);
	REGISTER_RHASH_CONSTANT(RHASH_ED2K);
	REGISTER_RHASH_CONSTANT(RHASH_AICH);
	REGISTER_RHASH_CONSTANT(RHASH_WHIRLPOOL);
	REGISTER_RHASH_CONSTANT(RHASH_RIPEMD160);
	REGISTER_RHASH_CONSTANT(RHASH_GOST);
	REGISTER_RHASH_CONSTANT(RHASH_GOST_CRYPTOPRO);
	REGISTER_RHASH_CONSTANT(RHASH_HAS160);
	REGISTER_RHASH_CONSTANT(RHASH_SNEFRU128);
	REGISTER_RHASH_CONSTANT(RHASH_SNEFRU256);
	REGISTER_RHASH_CONSTANT(RHASH_SHA224);
	REGISTER_RHASH_CONSTANT(RHASH_SHA256);
	REGISTER_RHASH_CONSTANT(RHASH_SHA384);
	REGISTER_RHASH_CONSTANT(RHASH_SHA512);
	REGISTER_RHASH_CONSTANT(RHASH_EDONR256);
	REGISTER_RHASH_CONSTANT(RHASH_EDONR512);
	REGISTER_RHASH_CONSTANT(RHASH_SHA3_224);
	REGISTER_RHASH_CONSTANT(RHASH_SHA3_256);
	REGISTER_RHASH_CONSTANT(RHASH_SHA3_384);
	REGISTER_RHASH_CONSTANT(RHASH_SHA3_512);
	REGISTER_RHASH_CONSTANT(RHASH_ALL);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION(rhash) */
PHP_MSHUTDOWN_FUNCTION(rhash)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION(rhash) */
PHP_MINFO_FUNCTION(rhash)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "rhash support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* Global functions */

/* {{{ proto int rhash_count()
   Returns the number of supported hash functions */
PHP_FUNCTION(rhash_count) {
	RETURN_LONG(rhash_count());
}
/* }}} */

/* {{{ proto int rhash_get_digest_size(int hash_id)
   Returns the size in bytes of message digest of the specified hash function */
PHP_FUNCTION(rhash_get_digest_size) {
	long hash_id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &hash_id) == FAILURE) {
		RETURN_FALSE;
	}
	RETURN_LONG(rhash_get_digest_size((unsigned)hash_id));
}
/* }}} */

/* {{{ proto boolean rhash_is_base32(int hash_id)
   Returns true if default format of message digest is base32 and false if it's hexadecimal */
PHP_FUNCTION(rhash_is_base32) {
	long hash_id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &hash_id) == FAILURE) {
		RETURN_FALSE;
	}
	RETURN_BOOL(rhash_is_base32((unsigned)hash_id));
}
/* }}} */

/* {{{ proto string rhash_get_name(int hash_id)
   Returns the name of the specified hash function */
PHP_FUNCTION(rhash_get_name) {
	long hash_id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &hash_id) == FAILURE) {
		RETURN_FALSE;
	}
	RETURN_STRING(rhash_get_name((unsigned)hash_id), 1);
}
/* }}} */

/* {{{ proto string rhash_msg(int hash_id, string message)
   Returns message digest for the message string */
PHP_FUNCTION(rhash_msg) {
	long hash_id = 0;
	char *s;
	int s_len;
	int length;
	rhash context = NULL;
	char buffer[130];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &hash_id, &s, &s_len) == FAILURE) {
		RETURN_NULL();
	}
	if (!hash_id) hash_id = RHASH_ALL_HASHES;

	if (!(context = rhash_init(hash_id))) {
		RETURN_NULL();
	}

	rhash_update(context, s, s_len);
	rhash_final(context, 0);
	length = rhash_print(buffer, context, hash_id, 0);
	rhash_free(context);
	RETURN_STRINGL(buffer, length, 1);
}

/* Calculate hash for a php stream. Returns SUCCESS or FAILURE. */
static int _php_rhash_stream(INTERNAL_FUNCTION_PARAMETERS, rhash context, php_stream *stream, long start, long size)
{
	char data[8192];
	if (context == NULL) {
		rhash_object *obj = (rhash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
		if ((context = obj->rhash) == NULL) return FAILURE;
	}

	if (start >= 0) {
		if(php_stream_seek(stream, start, SEEK_SET) < 0) return FAILURE;
	}

	if (size >= 0) {
		for(; size > 0; size -= 8192) {
			int res = php_stream_read(stream, data, (size < 8192 ? size : 8192));
			if (res < 0) return FAILURE;
			rhash_update(context, data, res);
		}
	} else {
		int res;
		while((res = php_stream_read(stream, data, 8192)) > 0) {
			rhash_update(context, data, res);
		}
		if (res < 0) return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

/* Calculate hash of the given file or its part. Returns SUCCESS or FAILURE. */
static int _php_rhash_file(INTERNAL_FUNCTION_PARAMETERS, rhash context, char* path, long start, long size)
{
	int res;
	php_stream *stream = php_stream_open_wrapper(path, "rb", 0, 0);
	if (stream == NULL) return FAILURE;

	res = _php_rhash_stream(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, stream, start, size);
	php_stream_close(stream);
	return res;
}
/* }}} */

/* {{{ proto string rhash_msg(int hash_id, string path)
   Computes and returns message digest for a file. Returns NULL on failure. */
PHP_FUNCTION(rhash_file) {
	long hash_id = 0;
	char *path;
	int path_len;
	rhash context = NULL;
	char buffer[130];
	int buffer_length;
	int res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &hash_id, &path, &path_len) == FAILURE) {
		RETURN_NULL();
	}
	if (!hash_id || !(context = rhash_init(hash_id))) {
		RETURN_NULL()
	}
	res = _php_rhash_file(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, path, -1, -1);
	rhash_final(context, 0);
	buffer_length = rhash_print(buffer, context, hash_id, 0);
	rhash_free(context);

	/* return NULL on failure */
	if (res != SUCCESS) {
		RETURN_NULL();
	}
	RETURN_STRINGL(buffer, buffer_length, 1);
}
/* }}} */

/* {{{ proto string rhash_magnet(int hash_id, string path)
   Computes and returns magnet link for a file. Returns NULL on failure. */
PHP_FUNCTION(rhash_magnet) {
	long hash_id = 0;
	char *path;
	int path_len;
	rhash context = NULL;
	char* buffer;
	size_t buffer_size;
	int res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &hash_id, &path, &path_len) == FAILURE) {
		RETURN_NULL();
	}
	if (!hash_id || !(context = rhash_init(hash_id))) {
		RETURN_NULL();
	}
	res = _php_rhash_file(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, path, -1, -1);
	if (res != SUCCESS) RETURN_NULL();
	rhash_final(context, 0);

	buffer_size = rhash_print_magnet(0, path, context, hash_id, RHPR_FILESIZE);
	buffer = (char*)emalloc(buffer_size);
	if (!buffer) {
		rhash_free(context);
		RETURN_NULL();
	}

	rhash_print_magnet(buffer, path, context, hash_id, RHPR_FILESIZE);
	rhash_free(context);
	RETURN_STRINGL(buffer, buffer_size - 1, 0);
}
/* }}} */

/* Free the memory allocated by rhash_object */
static void rhash_free_storage(void *object TSRMLS_DC)
{
	rhash_object *obj = (rhash_object *)object;
	rhash_free(obj->rhash);

	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

/* Allocate memory for new rhash_object */
static zend_object_value rhash_create_handler(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	rhash_object *obj = (rhash_object *)emalloc(sizeof(rhash_object));
	memset(obj, 0, sizeof(rhash_object));
	obj->std.ce = class_type;

	ALLOC_HASHTABLE(obj->std.properties);
	zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(obj->std.properties, &class_type->default_properties,
		(copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#else
	(void)tmp;
	object_properties_init(&obj->std, class_type);
#endif
	retval.handle = zend_objects_store_put(obj, NULL,
		rhash_free_storage, NULL TSRMLS_CC);
	retval.handlers = &rhash_object_handlers;

	return retval;
}

/* RHash class methods */

/* {{{ proto RHash::__construct([int hash_id])
   Creates new RHash object */
PHP_METHOD(RHash, __construct)
{
	long hash_id = 0;
	rhash context = NULL;
	rhash_object *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &hash_id) == FAILURE) {
		RETURN_NULL();
	}
	if (!hash_id) hash_id = RHASH_ALL_HASHES;
	if (!(context = rhash_init(hash_id))) {
		RETURN_NULL();
	}
	rhash_set_autofinal(context, 0);
	obj = (rhash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	obj->rhash = context;
}
/* }}} */

/* {{{ proto RHash RHash::update(string message)
   Updates RHash object with new data chunk and returns $this */
PHP_METHOD(RHash, update)
{
	char *s;
	int s_len;
	zval *object = getThis();
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(object TSRMLS_CC);
	
	if (obj->rhash == NULL ||
		zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &s, &s_len) == FAILURE) {
		RETURN_FALSE;
	}
	rhash_update(obj->rhash, s, s_len);
	Z_ADDREF(*object);
	*return_value = *object;
}
/* }}} */

/* {{{ proto boolean RHash::update_stream(resource handle[, int start[, int size]])
   Returns true if successfully calculated hashes for a (part of) stream, false on error */
PHP_METHOD(RHash, update_stream)
{
	zval *handle;
	int res;
	long start = -1, size = -1;
	php_stream *stream;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|ll", &handle, &start, &size) == FAILURE) {
		RETURN_FALSE;
	}
	php_stream_from_zval_no_verify(stream, &handle);
	if (stream == NULL) RETURN_FALSE;
	res = _php_rhash_stream(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, stream, start, size);
	RETURN_BOOL(res == SUCCESS);
}
/* }}} */

/* {{{ proto boolean RHash::update_file(string path[, int start[, int size]])
   Returns true if successfully calculated hashes for a (part of) file, false on error */
PHP_METHOD(RHash, update_file)
{
	char *path;
	int len;
	long start = -1, size = -1;
	int res = zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p|ll", &path, &len, &start, &size);
	if (res == SUCCESS) {
		res = _php_rhash_file(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, path, start, size);
	}
	RETURN_BOOL(res == SUCCESS);
}
/* }}} */

/* {{{ proto RHash RHash::final()
   Finalizes calculation for all hashed data and returns $this */
PHP_METHOD(RHash, final)
{
	zval *object = getThis();
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(object TSRMLS_CC);
	if (obj->rhash == NULL) RETURN_FALSE;
	rhash_final(obj->rhash, NULL);
	Z_ADDREF(*object);
	*return_value = *object;
}
/* }}} */

/* {{{ proto RHash RHash::reset()
   Resets RHash object to initial state and returns $this */
PHP_METHOD(RHash, reset)
{
	zval *object = getThis();
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(object TSRMLS_CC);
	if (obj->rhash == NULL) RETURN_FALSE;
	rhash_reset(obj->rhash);
	Z_ADDREF(*object);
	*return_value = *object;
}
/* }}} */

/* {{{ proto int RHash::hashed_length()
   Returns length in bytes of the hashed data */
PHP_METHOD(RHash, hashed_length)
{
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (obj->rhash == NULL) RETURN_FALSE;
	RETURN_LONG((long)obj->rhash->msg_size);
}
/* }}} */

/* {{{ _php_get_hash(RHash this_class[, int hash_id], int print_flags)
   Returns calculated hash in the specified format */
static void _php_get_hash(INTERNAL_FUNCTION_PARAMETERS, int print_flags)
{
	long hash_id = 0;
	char buffer[130];
	int length;
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (obj->rhash == NULL ||
		zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &hash_id) == FAILURE) {
		RETURN_FALSE;
	}
	length = rhash_print(buffer, obj->rhash, hash_id, print_flags);
	RETURN_STRINGL(buffer, length, 1)
}
/* }}} */

/* {{{ proto string RHash::hash([int hash_id])
   Returns hash value in default format */
PHP_METHOD(RHash, hash)
{
	_php_get_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string RHash::raw([int hash_id])
   Returns hash value as raw bytes */
PHP_METHOD(RHash, raw)
{
	_php_get_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, RHPR_RAW);
}
/* }}} */

/* {{{ proto string RHash::hex([int hash_id])
   Returns hash value as hexadecimal string */
PHP_METHOD(RHash, hex)
{
	_php_get_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, RHPR_HEX);
}
/* }}} */

/* {{{ proto string RHash::base32([int hash_id])
   Returns hash value as base32 string */
PHP_METHOD(RHash, base32)
{
	_php_get_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, RHPR_BASE32);
}
/* }}} */

/* {{{ proto string RHash::base64([int hash_id])
   Returns hash value as base64 string */
PHP_METHOD(RHash, base64)
{
	_php_get_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, RHPR_BASE64);
}
/* }}} */

/* {{{ proto string RHash::magnet([string filename])
   Returns magnet link with all hashes computed by the RHash object */
PHP_METHOD(RHash, magnet)
{
	char *s = 0;
	int s_len;
	size_t buf_size;
	char *buffer;
	rhash_object *obj = (rhash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (obj->rhash == NULL ||
		zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &s, &s_len) == FAILURE) {
		RETURN_FALSE;
	}

	buf_size = rhash_print_magnet(0, s, obj->rhash, RHASH_ALL_HASHES, RHPR_FILESIZE);
	buffer = (char*)emalloc(buf_size);
	if (!buffer) RETURN_FALSE;

	rhash_print_magnet(buffer, s, obj->rhash, RHASH_ALL_HASHES, RHPR_FILESIZE);
	RETURN_STRINGL(buffer, buf_size - 1, 0);
}
/* }}} */
