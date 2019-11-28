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

MONO?=mono
CS?=mcs
MONO_PATH?=..

test: Test.exe
	MONO_PATH="$(MONO_PATH)" $(MONO) Test.exe

Test.exe: Test.cs
	$(CS) -r:RHash.dll -lib:.. Test.cs

clean:
	rm -f Test.exe
