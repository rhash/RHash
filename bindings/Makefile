MAKE = make
RUBY = ruby
PERL = /usr/bin/perl
DESTDIR =
PREFIX = /usr/local
PERL_OPTIMIZE = -O2 -g -Wall

all: configure build test
configure: configure-ruby configure-perl
build: build-java build-perl build-ruby
test: test-java test-perl test-ruby
install: install-ruby install-perl
clean: distclean

configure-perl:
	cd perl && perl Makefile.PL INSTALLDIRS=vendor

configure-ruby:
	$(RUBY) -C ruby extconf.rb

build-java:
	$(MAKE) -C java/ jar jar-symlink jni javadoc

build-perl:
	$(MAKE) -C perl OPTIMIZE="$(PERL_OPTIMIZE)"

build-ruby:
	$(MAKE) -C ruby

test-java:
	$(MAKE) -C java test

test-perl:
	$(MAKE) -C perl test

test-ruby:
	$(RUBY) -C ruby test_rhash.rb

install-ruby:
	$(MAKE) -C ruby install DESTDIR=$(DESTDIR) sitedir=$(DESTDIR)/usr/lib/ruby

install-perl:
	$(MAKE) -C perl install DESTDIR=$(DESTDIR)
#	$(MAKE) -C perl install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) SITEPREFIX=$(PREFIX)

distclean:
	$(MAKE) -C java distclean
	[ ! -f ruby/Makefile ] || $(MAKE) -C ruby distclean
	[ ! -f perl/Makefile ] || $(MAKE) -C perl distclean
