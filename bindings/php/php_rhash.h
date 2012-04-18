#ifndef PHP_RHASH_H
#define PHP_RHASH_H

#include <rhash/rhash.h>

extern zend_module_entry rhash_module_entry;
#define phpext_rhash_ptr &rhash_module_entry

PHP_MINIT_FUNCTION(rhash);
PHP_MSHUTDOWN_FUNCTION(rhash);
PHP_MINFO_FUNCTION(rhash);

PHP_FUNCTION(rhash_count);
PHP_FUNCTION(rhash_get_digest_size);
PHP_FUNCTION(rhash_get_hash_length);
PHP_FUNCTION(rhash_is_base32);
PHP_FUNCTION(rhash_get_name);
PHP_FUNCTION(rhash_get_magnet_name);

PHP_METHOD(RHash, __construct);
PHP_METHOD(RHash, update);
PHP_METHOD(RHash, update_fd);
PHP_METHOD(RHash, update_file);
PHP_METHOD(RHash, final);
PHP_METHOD(RHash, reset);
PHP_METHOD(RHash, hashed_length);
PHP_METHOD(RHash, hash_id);

PHP_METHOD(RHash, hash);
PHP_METHOD(RHash, raw);
PHP_METHOD(RHash, hex);
PHP_METHOD(RHash, base32);
PHP_METHOD(RHash, base64);
PHP_METHOD(RHash, magnet);

#endif /* PHP_RHASH_H */
