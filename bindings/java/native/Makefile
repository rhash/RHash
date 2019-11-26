#!/usr/bin/make -f

# This file is a part of Java Bindings for Librhash
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
# 
# Permission is hereby granted, free of charge,  to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction,  including without limitation the rights
# to  use,  copy,  modify,  merge, publish, distribute, sublicense, and/or sell
# copies  of  the Software,  and  to permit  persons  to whom  the Software  is
# furnished to do so.
# 
# This library  is distributed  in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. Use it at your own risk!


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

