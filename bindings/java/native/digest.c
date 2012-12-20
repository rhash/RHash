/*
 * This file is a part of Java Bindings for Librhash
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

