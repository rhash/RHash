/*
 * This file is a part of Java Bindings for Librhash
 * Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
 * Librhash is (c) 2011, Alexey S Kravchenko <rhash.admin@gmail.com>
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

#ifndef HashObject_H
#define HashObject_H

typedef struct {
	int hash_id;
	size_t hash_len;
	unsigned char* hash_data;
} HashStruct;

typedef HashStruct *HashObject;

/**
 * Frees memory occupated by HashObject.
 * @param  obj  object to free
 */
void freeHashObject(HashObject obj);

/**
 * Allocates new HashObject that is exact copy of parameter.
 * @param  obj  object to clone
 */
HashObject cloneHashObject(HashObject obj);

/**
 * Compares two HashObject instances.
 * @param  obj1  first object to compare
 * @param  obj2  second object to compare
 * @return  1 if objects are equal, 0 otherwise
 */
int compareHashObjects(HashObject obj1, HashObject obj2);

/**
 * Calculates hashcode for HashObject.
 * @param  obj  object to calculate hash code
 */
int hashcodeForHashObject(HashObject obj);

#endif /* HashObject_H */
