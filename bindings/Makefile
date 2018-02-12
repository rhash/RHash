VERSION := $(shell sed -ne 's/^version=\(.*\)/\1/p' version.properties)
BINDINGS = java mono perl python ruby php
FILES = Makefile version.properties ChangeLog COPYING
DESTDIR =
PREFIX = /usr/local
RUBY ?= ruby
PYTHON ?= python
PERL ?= perl
CP = cp -l --parents
PERL_OPTIMIZE = -O2 -g -Wall
ARCHIVE_GZIP = rhash-bindings-$(VERSION)-src.tar.gz
COPYDIR = rhash-bindings-$(VERSION)

all: configure build test
clean: distclean

configure: $(patsubst %, configure-%, $(filter perl ruby php, $(BINDINGS)))
build: $(patsubst %, build-%, $(BINDINGS))
test: $(patsubst %, test-%, $(BINDINGS))
install: $(patsubst %, install-%, $(filter perl ruby php, $(BINDINGS)))

configure-perl: perl/Makefile
configure-ruby: ruby/Makefile
configure-php: php/Makefile

perl/Makefile: perl/Makefile.PL
	cd perl && USE_SYSTEM_LIBRHASH=1 $(PERL) Makefile.PL INSTALLDIRS=vendor

php/Makefile: php/config.m4
	cd php && phpize && ./configure --with-rhash

ruby/Makefile: ruby/extconf.rb
	$(RUBY) -C ruby extconf.rb

build-java:
	+$(MAKE) -C java build-binary

build-perl: configure-perl
	+$(MAKE) -C perl OPTIMIZE="$(PERL_OPTIMIZE)"

build-php: configure-php
	+$(MAKE) -C php

build-python:
# not required

build-ruby: configure-ruby
	+$(MAKE) -C ruby

build-mono:
	+$(MAKE) -C mono

test-java:
	+$(MAKE) -C java test

test-perl:
	+$(MAKE) -C perl test

test-php:
	+$(MAKE) -C php test TEST_PHP_ARGS=-q

test-ruby:
	$(RUBY) -C ruby -I. test_rhash.rb

test-mono:
	+$(MAKE) -C mono test

test-python:
	$(PYTHON) python/test_rhash.py

install-ruby:
# clear MAKEFLAGS to overcome ruby1.8 mkmf concurrency bug
	+MAKEFLAGS= $(MAKE) -C ruby install DESTDIR=$(DESTDIR) sitedir=$(DESTDIR)/usr/lib/ruby

install-perl:
	+$(MAKE) -C perl install DESTDIR=$(DESTDIR)

install-php:
	+$(MAKE) -C php install INSTALL_ROOT=$(DESTDIR)

copy-dist:
	mkdir -p $(COPYDIR)
	find java ruby python -type f -regex '.*\(\.\([hc]\|java\|py\|rb\|txt\)\|Makefile\)' -exec $(CP) '{}' $(COPYDIR)/ \;
	find mono -type f -regex '.*\(\.\([hc]\|cs\|xml\|txt\|snk\|sln\|csproj\|config\)\|Makefile\)' -exec $(CP) '{}' "$(COPYDIR)/" \;
	$(CP) $(shell sed -e 's/\([^ ]*\).*/perl\/\1/' perl/MANIFEST) "$(COPYDIR)/"
	find php -type f -regex '.*\.\(m4\|c\|h\|phpt\)' -exec $(CP) '{}' "$(COPYDIR)/" \;
	$(CP) $(FILES) "$(COPYDIR)/"

gzip: distclean
	rm -rf "$(COPYDIR)" $(ARCHIVE_GZIP)
	+$(MAKE) copy-dist
	tar -czf $(ARCHIVE_GZIP) --owner=root:0 --group=root:0 "$(COPYDIR)"
	rm -rf "$(COPYDIR)"

PERL_PKG_VER = $(shell [ -f perl/Rhash.pm ] && sed -ne "s/^our \+.VERSION *= *'\([0-9\.]*\)';/\1/p;" perl/Rhash.pm)
PERL_PKG = Crypt-RHash-$(PERL_PKG_VER)

cpan:
	[ -f ../librhash/rhash.h ]
	echo "$(PERL_PKG_VER)" | grep -q '^[0-9\.]\+$$'
	rm -rf $(PERL_PKG)/ $(PERL_PKG).tar.gz
	mkdir -p $(PERL_PKG)/librhash/
	grep -q / perl/MANIFEST && mkdir -p `sed -ne '/\//s/\([^\/]*\/\).*/$(PERL_PKG)\/\1/p' perl/MANIFEST | sort -u`
	sed -e 's/^\([^ ]*\)\( .*\)\?/cp perl\/\1 $(PERL_PKG)\/\1/' perl/MANIFEST > _cpy.sh
	sh _cpy.sh && rm -f _cpy.sh
	cp ../librhash/*.[hc] $(PERL_PKG)/librhash/
	cp perl/MANIFEST $(PERL_PKG)/
	find $(PERL_PKG)/librhash/ -type f -printf "librhash/%f\n" | sort >> $(PERL_PKG)/MANIFEST
	tar -czf $(PERL_PKG).tar.gz --owner=root:0 --group=root:0 $(PERL_PKG)/
	rm -rf $(PERL_PKG)/

distclean:
	+$(MAKE) -C java distclean
	+$(MAKE) -C mono clean
	+[ ! -f ruby/Makefile ] || $(MAKE) -C ruby distclean
	+[ ! -f perl/Makefile ] || $(MAKE) -C perl distclean
	[ ! -f php/configure ] || (cd php && phpize --clean)
	rm -f python/*.pyc
