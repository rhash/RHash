ACLOCAL_AMFLAGS=-I m4
bin_PROGRAMS=rhash

SUBDIRS=dist librhash tests

AM_CPPFLAGS = -DSYSCONFDIR=\"${sysconfdir}\"

rhash_SOURCES=calc_sums.c hash_print.c common_func.c hash_update.c file_mask.c file_set.c find_file.c hash_check.c output.c parse_cmdline.c rhash_main.c win_utils.c
rhash_INCLUDES=-I$(top_srcdir)/librhash
noinst_HEADERS=calc_sums.h hash_print.h common_func.h hash_update.h file_mask.h file_set.h find_file.h hash_check.h output.h parse_cmdline.h rhash_main.h win_utils.h version.h
rhash_LDADD=$(top_builddir)/librhash/librhash.la

BINLINKS = sfv-hash tiger-hash tth-hash whirlpool-hash has160-hash gost-hash ed2k-link magnet-link

EXTRA_DIST = win32/dirent.h win32/stdint.h win32/unistd.h win32/platform-dependent.h \
  win32/vc-2010/rhash.vcxproj

install-exec-hook:
	cd $(DESTDIR)$(bindir) && for i in $(BINLINKS); do \
		ln $(bin_PROGRAMS) $$i; \
	done

uninstall-hook:
	cd $(DESTDIR)$(bindir) && for i in $(BINLINKS); do \
		rm -f $$i; \
	done

# check version
check-version: version.h
	grep -q '\* === Version $(VERSION) ===' ChangeLog
	grep -q '^#define VERSION "$(VERSION)"' version.h
	[ ! -d bindings -o bindings/version.properties -nt Makefile ] || \
		echo "version=$(VERSION)" > bindings/version.properties
	[ -s dist/rhash.1.html ]

cpp-doc:
	cppdoc_cmd -title=RHash -company=Animegorodok -classdir=classdoc -module="cppdoc-standard" -overwrite -extensions="c,h" -languages="c=cpp,h=cpp" -generate-deprecations-list=false $(rhash_SOURCES) $(rhash_INCLUDES) $(noinst_HEADERS) ./Documentation/CppDoc/index.html

permissions:
	find . dist librhash po win32 win32/vc-2010 -maxdepth 1 -type f -exec chmod -x '{}' \;
	chmod +x tests/test_rhash.sh

update-po:
	xgettext *.c -k_ -cTRANSLATORS -o po/rhash.pot \
		--msgid-bugs-address='Aleksey <rhash.admin@gmail.com>' --package-name='RHash'
	for f in po/*.po; do \
		msgmerge -U $$f po/rhash.pot; \
	done
