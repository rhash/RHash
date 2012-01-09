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
