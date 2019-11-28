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

#include <stdlib.h>
#include <string.h>

#include "digest.h"

void freeDigest(Digest obj) {
	free(obj->hash_data);
	free(obj);
}

int compareDigests(Digest o1, Digest o2) {
	if (o1->hash_len != o2->hash_len) return 0;
	return memcmp(o1->hash_data, o2->hash_data, o1->hash_len) == 0;
}

int hashcodeForDigest(Digest obj) {
	int hash = 123321, i;
	for (i = 0; i < obj->hash_len; i++) {
		switch (i % 3) {
			case 0: hash ^= obj->hash_data[i];
			case 1: hash ^= obj->hash_data[i] << 8;
			case 2: hash ^= obj->hash_data[i] << 16;
			case 3: hash ^= obj->hash_data[i] << 24;
		}
	}
	return hash ^ (obj->hash_id + obj->hash_len);
}

