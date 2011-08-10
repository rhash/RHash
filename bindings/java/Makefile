#!/usr/bin/make -f

# This file is a part of Java Bindings for Librhash
# Copyright (c) 2011, Sergey Basalaev <sbasalaev@gmail.com>
# Librhash is (c) 2011, Alexey S Kravchenko <rhash.admin@gmail.com>
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


MAKE = make
JAVA = java
JAVAC = javac
JAVADOC = javadoc
JAVAH = javah
JAR = jar

VERSION = $(shell sed -e 's/\(.*\)"\(.*\)"\(.*\)/\2/' ../../version.h)

JAVASRC_DIR = src
CSRC_DIR    = native
DEST_DIR    = dist
CLASSES_DIR = classes
JAVADOC_DIR = javadoc
TEST_DIR    = test

JARFILE = $(DEST_DIR)/rhash-$(VERSION).jar

all: jar jni test

jar: compile-classes
	mkdir -p $(DEST_DIR)
	$(JAR) cf $(JARFILE) -C $(CLASSES_DIR) org/

compile-classes:
	mkdir -p $(CLASSES_DIR)
	$(JAVAC) -d $(CLASSES_DIR) -sourcepath $(JAVASRC_DIR) `find ${JAVASRC_DIR} -name '*.java'`

update-header: compile-classes
	$(JAVAH) -o $(CSRC_DIR)/bindings.h -classpath $(CLASSES_DIR)  org.sf.rhash.Bindings

jni:
	$(MAKE) -C $(CSRC_DIR)
	mkdir -p $(DEST_DIR)
	cp $(CSRC_DIR)/*rhash-jni* $(DEST_DIR)

javadoc: clean-javadoc
	$(JAVADOC) -windowtitle 'RHash' \
	           -sourcepath src \
	           -subpackages org \
	           -d $(JAVADOC_DIR) \
	           -link http://java.sun.com/j2se/1.5.0/docs/api

test: jar jni
	$(JAVAC) -classpath $(JARFILE) $(TEST_DIR)/Test.java
	$(JAVA) -classpath $(TEST_DIR):$(JARFILE) -Djava.library.path=$(DEST_DIR) Test

clean: clean-javadoc clean-jni clean-classes clean-test

clean-javadoc:
	rm -rf $(JAVADOC_DIR)

clean-classes:
	rm -rf $(CLASSES_DIR)

clean-jni:
	$(MAKE) -C $(CSRC_DIR) clean

clean-test:
	rm -f $(TEST_DIR)/*.class

distclean: clean
	rm -rf $(DEST_DIR)

.PHONY: jar jni javadoc clean clean-javadoc distclean
