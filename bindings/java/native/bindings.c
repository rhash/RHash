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
#include "digest.h"

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
	// creating and populating Digest
	Digest obj = malloc(sizeof(DigestStruct));
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
 * Method:    rhash_print_bytes
 * Signature: (JI)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_sf_rhash_Bindings_rhash_1print_1bytes
(JNIEnv *env, jclass clz, jlong ptr, jint flags) {
	Digest obj = (Digest)ptr;
	unsigned char output[130];
	int len = rhash_print_bytes(output, obj->hash_data, obj->hash_len, flags);
	jbyteArray arr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, arr, 0, len, output);
	return arr;
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
 * Method:    rhash_get_digest_size
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_sf_rhash_Bindings_rhash_1get_1digest_1size
(JNIEnv *env, jclass clz, jint hash_id) {
	return rhash_get_digest_size(hash_id);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_init
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_org_sf_rhash_Bindings_rhash_1init
(JNIEnv *env, jclass clz, jint hash_flags) {
	return (jlong)rhash_init(hash_flags);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_update
 * Signature: (J[BII)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_rhash_1update
(JNIEnv *env, jclass clz, jlong context, jbyteArray data, jint ofs, jint len) {
	void* msg = malloc(len);
	(*env)->GetByteArrayRegion(env, data, ofs, len, msg);
	rhash_update((rhash)context, msg, len);
	free(msg);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_final
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_rhash_1final
(JNIEnv *env, jclass clz, jlong context) {
	rhash_final((rhash)context, NULL);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_reset
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_rhash_1reset
(JNIEnv *env, jclass clz, jlong context) {
	rhash_reset((rhash)context);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_print
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_org_sf_rhash_Bindings_rhash_1print
(JNIEnv *env, jclass clz, jlong context, jint hash_id) {
	Digest obj = malloc(sizeof(DigestStruct));
	obj->hash_len  = rhash_get_digest_size(hash_id);
	obj->hash_data = calloc(obj->hash_len, sizeof(unsigned char));
	rhash_print(obj->hash_data, (rhash)context, hash_id, RHPR_RAW);
	return (jlong)obj;
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    rhash_free
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_rhash_1free
(JNIEnv *env, jclass clz, jlong context) {
	rhash_free((rhash)context);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    compareDigests
 * Signature: (JJ)Z
 */
JNIEXPORT jboolean JNICALL Java_org_sf_rhash_Bindings_compareDigests
(JNIEnv *env, jclass clz, jlong ptr1, jlong ptr2) {
	return compareDigests((Digest)ptr1, (Digest)ptr2);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    hashcodeForDigest
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_sf_rhash_Bindings_hashcodeForDigest
(JNIEnv *env, jclass clz, jlong ptr) {
	return hashcodeForDigest((Digest)ptr);
}

/*
 * Class:     org_sf_rhash_Bindings
 * Method:    freeDigest
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_sf_rhash_Bindings_freeDigest
(JNIEnv *env, jclass clz, jlong ptr) {
	freeDigest((Digest)ptr);
}

