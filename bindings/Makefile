MAKE = make
RUBY = ruby
PYTHON = python
PERL = /usr/bin/perl
DESTDIR =
PREFIX = /usr/local
PERL_OPTIMIZE = -O2 -g -Wall
VERSION := $(shell sed -ne 's/^version=\(.*\)/\1/p' version.properties)
FILES = Makefile version.properties
BINDINGS = java mono perl python ruby
ARCHIVE_GZIP = rhash-bindings-$(VERSION)-src.tar.gz
COPYDIR = rhash-bindings-$(VERSION)
CP = cp -l --parents

all: configure build test
clean: distclean

configure: $(patsubst %, configure-%, $(filter perl ruby, $(BINDINGS)))
build: $(patsubst %, build-%, $(BINDINGS))
test: $(patsubst %, test-%, $(BINDINGS))
install: $(patsubst %, install-%, $(filter perl ruby, $(BINDINGS)))

configure-perl: perl/Makefile
configure-ruby: ruby/Makefile
configure-php: php/Makefile

perl/Makefile: perl/Makefile.PL
	cd perl && perl Makefile.PL INSTALLDIRS=vendor

php/Makefile: php/config.m4
	cd php && phpize && ./configure --with-rhash

ruby/Makefile: ruby/extconf.rb
	$(RUBY) -C ruby extconf.rb

build-java:
	$(MAKE) -C java

build-perl: configure-perl
	$(MAKE) -C perl OPTIMIZE="$(PERL_OPTIMIZE)"

build-php: configure-php
	$(MAKE) -C php

build-python:
# not required

build-ruby: configure-ruby
	$(MAKE) -C ruby

build-mono:
	$(MAKE) -C mono

test-java:
	$(MAKE) -C java test

test-perl:
	$(MAKE) -C perl test

test-php:
#	$(MAKE) -C php test

test-ruby:
	$(RUBY) -C ruby test_rhash.rb

test-mono:
	$(MAKE) -C mono test

test-python:
	$(PYTHON) python/test_rhash.py

install-ruby:
	$(MAKE) -C ruby install DESTDIR=$(DESTDIR) sitedir=$(DESTDIR)/usr/lib/ruby

install-perl:
	$(MAKE) -C perl install DESTDIR=$(DESTDIR)
#	$(MAKE) -C perl install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) SITEPREFIX=$(PREFIX)

copy-dist:
	mkdir -p $(COPYDIR)
	find java ruby python -type f -regex '.*\(\.\([hc]\|java\|py\|rb\|txt\)\|Makefile\)' -exec $(CP) '{}' $(COPYDIR)/ \;
	find perl -type f -regex '.*\(\.\(t\|PL\|pm\|xs\)\|MANIFEST\|typemap\)' -exec $(CP) '{}' "$(COPYDIR)/" \;
	find mono -type f -regex '.*\(\.\([hc]\|cs\|xml\|txt\|snk\|sln\|csproj\)\|Makefile\)' -exec $(CP) '{}' "$(COPYDIR)/" \;
	$(CP) $(FILES) "$(COPYDIR)/"

gzip: distclean
	rm -rf "$(COPYDIR)" $(ARCHIVE_GZIP)
	make copy-dist
	tar -czf $(ARCHIVE_GZIP) "$(COPYDIR)"
	rm -rf "$(COPYDIR)"

distclean:
	$(MAKE) -C java distclean
	$(MAKE) -C mono clean
	[ ! -f ruby/Makefile ] || $(MAKE) -C ruby distclean
	[ ! -f perl/Makefile ] || $(MAKE) -C perl distclean
	[ ! -f php/configure ] || (cd php && phpize --clean)
	rm -f python/*.pyc
