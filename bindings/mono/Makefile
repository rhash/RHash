#!/usr/bin/make -f

# This file is a part of Mono Bindings for Librhash
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
