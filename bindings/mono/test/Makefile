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

MONO?=mono
CS?=mcs
MONO_PATH?=..

test: Test.exe
	MONO_PATH="$(MONO_PATH)" $(MONO) Test.exe

Test.exe: Test.cs
	$(CS) -r:RHash.dll -lib:.. Test.cs

clean:
	rm -f Test.exe
