#!/usr/bin/make -f

# This file is a part of Java Bindings for Librhash
#
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
# OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
# PERFORMANCE OF THIS SOFTWARE.

CC ?= gcc
JAVAC ?= javac
CFLAGS ?= -g -O2
LDFLAGS ?=
LIBRHASH_INC ?=
LIBRHASH_LD  ?=
ALLLDFLAGS = $(LDFLAGS) -lrhash $(LIBRHASH_LD)
ALLCFLAGS = $(CFLAGS) $(LIBRHASH_INC) -fPIC

#Platform dependent
JAVAC_PATH = $(shell readlink -f $(shell which $(JAVAC)))
JDK_HOME ?=$(realpath $(dir $(JAVAC_PATH))/..)
JNI_CFLAGS1 = $(shell test -d "$(JDK_HOME)/include" && echo -I$(JDK_HOME)/include)
JNI_CFLAGS2 = $(shell test -d "$(JDK_HOME)/include/linux" && echo -I$(JDK_HOME)/include/linux)
JNI_CFLAGS ?= $(JNI_CFLAGS1) $(JNI_CFLAGS2)
LIB_PREFIX ?= lib
LIB_SUFFIX ?= .so

OBJECTS = bindings.o digest.o
LIBRARY = $(LIB_PREFIX)rhash-jni$(LIB_SUFFIX)

all: $(LIBRARY)

bindings.o: bindings.c bindings.h
	$(CC) $(ALLCFLAGS) $(JNI_CFLAGS) -c $< -o $@

digest.o: digest.c digest.h
	$(CC) $(ALLCFLAGS) -c $< -o $@

$(LIBRARY): $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS) $(ALLLDFLAGS)

clean:
	rm -f *.o $(LIBRARY)

