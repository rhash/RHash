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

#include <rhash/rhash.h>
#include <stdlib.h>

#include "bindings.h"
#include "HashObject.h"

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_library_init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_rhash_1library_1init
(JNIEnv *env, jclass clz) {
	rhash_library_init();
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_count
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_sf_rhash_Bindings_rhash_1count
(JNIEnv *env, jclass clz) {
	return rhash_count();
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_msg
 * Signature: (I[BII)J
 */
JNIEXPORT jlong JNICALL Java_org_sf_rhash_Bindings_rhash_1msg
(JNIEnv *env, jclass clz, jint hash_id, jbyteArray buf, jint ofs, jint len) {
	// reading data
	void* msg = malloc(len);
	(*env)->GetByteArrayRegion(env, buf, ofs, len, msg);
	// creating and populating HashObject
	HashObject obj = malloc(sizeof(HashStruct));
	obj->hash_len  = rhash_get_digest_size(hash_id);
	obj->hash_data = calloc(obj->hash_len, sizeof(unsigned char));
	rhash_msg(hash_id, msg, len, obj->hash_data);
	//cleaning
	free(msg);
	//returning
	return (jlong)obj;
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_file
 * Signature: (ILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_sf_rhash_Bindings_rhash_1file
(JNIEnv *env, jclass clz, jint hash_id, jstring filepath) {
	// reading filename
#ifdef _WIN32
	const wchar_t *path = (*env)->GetStringChars(env, filepath, NULL);
#else /* _WIN32 */
	const char *path = (*env)->GetStringUTFChars(env, filepath, NULL);
#endif /* _WIN32 */
	if (path == NULL) {
		return NULL; /* OutOfMemoryError is thrown */
	}
	// creating and populating HashObject
	HashObject obj = malloc(sizeof(HashStruct));
	obj->hash_len  = rhash_get_digest_size(hash_id);
	obj->hash_data = calloc(obj->hash_len, sizeof(unsigned char));
#ifdef _WIN32
	if ( rhash_wfile(hash_id, path, obj->hash_data) != 0) {
#else /* _WIN32 */
	if ( rhash_file(hash_id, path, obj->hash_data) != 0) {
#endif /* _WIN32 */
		freeHashObject(obj);
		obj = NULL;
		//throwing IOException
		jclass exclz = (*env)->FindClass(env, "java/io/IOException");
		if (exclz != NULL) {
			(*env)->ThrowNew(env, exclz, "Error reading file");
		}
		(*env)->DeleteLocalRef(env, exclz);
	}
#ifdef _WIN32
	(*env)->ReleaseStringChars(env, filepath, path);
#else /* _WIN32 */
	(*env)->ReleaseStringUTFChars(env, filepath, path);
#endif /* _WIN32 */
	return (jlong)obj;
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_print_bytes
 * Signature: (JI)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_sf_rhash_Bindings_rhash_1print_1bytes
(JNIEnv *env, jclass clz, jlong ptr, jint flags) {
	HashObject obj = (HashObject)ptr;
	unsigned char output[130];
	int len = rhash_print_bytes(output, obj->hash_data, obj->hash_len, flags);
	jbyteArray arr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, arr, 0, len, output);
	return arr;
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    freeHashObject
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_freeHashObject
(JNIEnv *env, jclass clz, jlong ptr) {
	freeHashObject((HashObject)ptr);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    cloneHashObject
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_sf_rhash_Bindings_cloneHashObject
(JNIEnv *env, jclass clz, jlong ptr) {
	HashObject obj = (HashObject)ptr;
	HashObject newobj = malloc(sizeof(HashStruct));
	newobj->hash_id = obj->hash_id;
	newobj->hash_len = obj->hash_len;
	newobj->hash_data = calloc(obj->hash_len, sizeof(unsigned char));
	memcpy(newobj->hash_data, obj->hash_data, obj->hash_len);
	return (jlong)newobj;
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_is_base32
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_sf_rhash_Bindings_rhash_1is_1base32
(JNIEnv *env, jclass clz, jint hash_id) {
	return rhash_is_base32(hash_id);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    compareHashObjects
 * Signature: (JJ)Z
 */
JNIEXPORT jboolean JNICALL Java_org_sf_rhash_Bindings_compareHashObjects
(JNIEnv *env, jclass clz, jlong ptr1, jlong ptr2) {
	return compareHashObjects((HashObject)ptr1, (HashObject)ptr2);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    hashcodeForHashObject
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_sf_rhash_Bindings_hashcodeForHashObject
(JNIEnv *env, jclass clz, jlong ptr) {
	return hashcodeForHashObject((HashObject)ptr);
}

