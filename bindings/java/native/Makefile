#!/usr/bin/make -f

# This file is a part of Java Bindings for Librhash
# Copyright (c) 2011-2012, Sergey Basalaev <sbasalaev@gmail.com>
# Librhash is (c) 2011-2012, Aleksey Kravchenko <rhash.admin@gmail.com>
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


CC = gcc
CFLAGS = -g -O2
LDFLAGS =
LIBRHASH_INC =
LIBRHASH_LD  =
ALLLDFLAGS = $(LDFLAGS) -lrhash $(LIBRHASH_LD)
ALLCFLAGS = $(CFLAGS) $(LIBRHASH_INC) -fPIC

#Platform dependent
JNI_CFLAGS = -I/usr/lib/jvm/default-java/include
LIB_PREFIX = lib
LIB_SUFFIX = .so

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

