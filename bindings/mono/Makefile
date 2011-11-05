#!/usr/bin/make -f

CS=gmcs
MDOC=mdoc
SOURCES=AssemblyInfo.cs Bindings.cs Hasher.cs HashType.cs

all: assembly assemble-doc html

assembly: RHash.dll RHash.dll.mdb

RHash.dll RHash.dll.mdb: $(SOURCES)
	$(CS) -target:library -define:UNIX -out:RHash.dll -debug -keyfile:RHash.snk $(SOURCES)

update-doc: RHash.dll
	$(MDOC) update RHash.dll -o doc

assemble-doc: RHash.tree RHash.zip

RHash.tree RHash.zip:
	$(MDOC) assemble -o RHash doc

html:
	$(MDOC) export-html -o html doc

clean:
	rm -f RHash.dll RHash.dll.mdb
	rm -f RHash.tree RHash.zip
	rm -rf html
