/*
 * This file is a part of Java Bindings for Librhash
 *
 * Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
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

/* This is convenient structure to hold message digest. */

#ifndef DIGEST_H
#define DIGEST_H

typedef struct {
	int hash_id;
	size_t hash_len;
	unsigned char *hash_data;
} DigestStruct;

typedef DigestStruct* Digest;

/**
 * Frees memory occupated by Digest.
 * @param  obj  object to free
 */
void freeDigest(Digest obj);

/**
 * Compares two Digest instances.
 * @param  obj1  first object to compare
 * @param  obj2  second object to compare
 * @return  1 if objects are equal, 0 otherwise
 */
int compareDigests(Digest obj1, Digest obj2);

/**
 * Calculates hashcode for Digest.
 * @param  obj  object to calculate hash code
 */
int hashcodeForDigest(Digest obj);

#endif /* DIGEST_H */
