/*
 * This file is a part of PHP Bindings for Librhash
 *
 * Copyright (c) 2012, Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef PHP_RHASH_H
#define PHP_RHASH_H

extern zend_module_entry rhash_module_entry;
#define phpext_rhash_ptr &rhash_module_entry

PHP_MINIT_FUNCTION(rhash);
PHP_MSHUTDOWN_FUNCTION(rhash);
PHP_MINFO_FUNCTION(rhash);

PHP_FUNCTION(rhash_count);
PHP_FUNCTION(rhash_get_digest_size);
PHP_FUNCTION(rhash_get_name);
PHP_FUNCTION(rhash_is_base32);
PHP_FUNCTION(rhash_msg);
PHP_FUNCTION(rhash_file);
PHP_FUNCTION(rhash_magnet);

PHP_METHOD(RHash, __construct);
PHP_METHOD(RHash, update);
PHP_METHOD(RHash, update_stream);
PHP_METHOD(RHash, update_file);
PHP_METHOD(RHash, final);
PHP_METHOD(RHash, reset);
PHP_METHOD(RHash, hashed_length);

PHP_METHOD(RHash, hash);
PHP_METHOD(RHash, raw);
PHP_METHOD(RHash, hex);
PHP_METHOD(RHash, base32);
PHP_METHOD(RHash, base64);
PHP_METHOD(RHash, magnet);

#endif /* PHP_RHASH_H */
