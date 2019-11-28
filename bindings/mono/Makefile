#!/usr/bin/make -f

# This file is a part of Mono Bindings for Librhash
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

CS?=mcs
MDOC?=mdoc
SOURCES=AssemblyInfo.cs Bindings.cs Hasher.cs HashType.cs

all: assembly assemble-doc html

assembly: RHash.dll RHash.dll.mdb

RHash.dll RHash.dll.mdb: $(SOURCES)
	$(CS) -target:library -out:RHash.dll -debug -keyfile:RHash.snk $(SOURCES)

update-doc: RHash.dll
	$(MDOC) update RHash.dll -o doc

assemble-doc: RHash.tree RHash.zip

RHash.tree RHash.zip:
	$(MDOC) assemble -o RHash doc

html:
	$(MDOC) export-html -o html doc

test: RHash.dll
	+$(MAKE) -C test

clean:
	rm -f RHash.dll RHash.dll.mdb
	rm -f RHash.tree RHash.zip
	rm -rf html
	+$(MAKE) -C test clean

.PHONY : clean html test
