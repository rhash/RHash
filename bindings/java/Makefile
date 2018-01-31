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


JAVA = java
JAVAC = javac
JAVADOC = javadoc
JAVAH = javah
JAR = jar
JUNIT_CLASSPATH = /usr/share/java/junit4.jar

VERSION = $(shell sed -ne 's/^version=\(.*\)/\1/p' ../version.properties)

JAVASRC_DIR = src
CSRC_DIR    = native
DEST_DIR    = dist
CLASSES_DIR = classes
JAVADOC_DIR = javadoc
TEST_DIR    = test

JAVA_FILES := $(shell find $(JAVASRC_DIR) -name '*.java')
JNI_FILES := $(shell find $(CSRC_DIR) -name '*.c' -o -name '*.h')
LIB_PREFIX = lib
LIB_SUFFIX = .so
JNI_LIBRARY = $(LIB_PREFIX)rhash-jni$(LIB_SUFFIX)
JARFILE = $(DEST_DIR)/rhash-$(VERSION).jar
JAVADOC_FILE = $(DEST_DIR)/rhash-$(VERSION)-javadoc.zip
JAVADOC_API_URL = http://download.oracle.com/javase/6/docs/api/


all: build-binary javadoc
build-binary: jar jar-symlink jni
jar: $(JARFILE)
jni: $(DEST_DIR)/$(JNI_LIBRARY)
zip-javadoc: $(JAVADOC_FILE)

jar-symlink:
	mkdir -p $(DEST_DIR)
	ln -fs rhash-$(VERSION).jar $(DEST_DIR)/rhash.jar

$(JARFILE): $(JAVA_FILES)
	+$(MAKE) compile-classes
	mkdir -p $(DEST_DIR)
	$(JAR) cf $(JARFILE) -C $(CLASSES_DIR) org/

compile-classes:
	mkdir -p $(CLASSES_DIR)
	$(JAVAC) -d $(CLASSES_DIR) -sourcepath $(JAVASRC_DIR) $(JAVA_FILES)

update-header: compile-classes
	$(JAVAH) -o $(CSRC_DIR)/bindings.h -classpath $(CLASSES_DIR)  org.sf.rhash.Bindings

$(DEST_DIR)/$(JNI_LIBRARY): $(JNI_FILES)
	+$(MAKE) -C $(CSRC_DIR)
	mkdir -p $(DEST_DIR)
	cp $(CSRC_DIR)/$(JNI_LIBRARY) $(DEST_DIR)

javadoc: clean-javadoc
	$(JAVADOC) -windowtitle 'RHash' \
	           -sourcepath $(JAVASRC_DIR) \
	           -subpackages org \
	           -d $(JAVADOC_DIR) \
	           -link $(JAVADOC_API_URL)

$(JAVADOC_FILE): $(JAVA_FILES)
	+$(MAKE) javadoc
	jar -cMf $(JAVADOC_FILE) $(JAVADOC_DIR)

$(TEST_DIR)/RHashTest.class: $(JARFILE) $(TEST_DIR)/RHashTest.java
	$(JAVAC) -classpath $(JARFILE):$(JUNIT_CLASSPATH) $(TEST_DIR)/RHashTest.java

test: $(TEST_DIR)/RHashTest.class jni
	$(JAVA) -classpath $(TEST_DIR):$(JARFILE):$(JUNIT_CLASSPATH) -Djava.library.path=$(DEST_DIR) junit.textui.TestRunner RHashTest

clean: clean-javadoc clean-jni clean-classes clean-test

clean-javadoc:
	rm -rf $(JAVADOC_DIR)

clean-classes:
	rm -rf $(CLASSES_DIR)

clean-jni:
	+$(MAKE) -C $(CSRC_DIR) clean

clean-test:
	rm -f $(TEST_DIR)/*.class

distclean: clean
	rm -rf $(DEST_DIR)

.PHONY: jar jni javadoc test clean clean-javadoc distclean
