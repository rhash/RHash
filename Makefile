
include config.mak

HEADERS = calc_sums.h hash_print.h common_func.h hash_update.h file.h file_mask.h file_set.h find_file.h hash_check.h output.h parse_cmdline.h rhash_main.h win_utils.h platform.h version.h
SOURCES = calc_sums.c hash_print.c common_func.c hash_update.c file.c file_mask.c file_set.c find_file.c hash_check.c output.c parse_cmdline.c rhash_main.c win_utils.c
OBJECTS = $(SOURCES:.c=.o)
WIN_DIST_FILES = dist/MD5.bat dist/magnet.bat dist/rhashrc.sample
OTHER_FILES = configure Makefile ChangeLog INSTALL.md COPYING README \
  dist/rhash.spec.in dist/rhash.1 dist/rhash.1.win.sed \
  tests/test_rhash.sh tests/test1K.data win32/vc-2010/rhash.vcxproj
LIBRHASH_FILES  = librhash/algorithms.c librhash/algorithms.h \
  librhash/byte_order.c librhash/byte_order.h librhash/plug_openssl.c librhash/plug_openssl.h \
  librhash/rhash.c librhash/rhash.h librhash/rhash_torrent.c librhash/rhash_torrent.h \
  librhash/rhash_timing.c librhash/rhash_timing.h \
  librhash/aich.c librhash/aich.h librhash/crc32.c librhash/crc32.h \
  librhash/ed2k.c librhash/ed2k.h librhash/edonr.c librhash/edonr.h \
  librhash/gost.c librhash/gost.h librhash/has160.c librhash/has160.h \
  librhash/hex.c librhash/hex.h librhash/md4.c librhash/md4.h \
  librhash/md5.c librhash/md5.h librhash/ripemd-160.c librhash/ripemd-160.h \
  librhash/sha1.c librhash/sha1.h librhash/sha3.c librhash/sha3.h \
  librhash/sha256.c librhash/sha256.h librhash/sha512.c librhash/sha512.h \
  librhash/snefru.c librhash/snefru.h librhash/tiger.c librhash/tiger.h \
  librhash/tiger_sbox.c librhash/tth.c librhash/tth.h librhash/whirlpool.c \
  librhash/whirlpool.h librhash/whirlpool_sbox.c librhash/test_hashes.c \
  librhash/test_hashes.h librhash/torrent.h librhash/torrent.c librhash/ustd.h \
  librhash/util.h librhash/Makefile
I18N_FILES  = po/ca.po po/de.po po/en_AU.po po/es.po po/fr.po po/gl.po po/it.po po/ro.po po/ru.po
ALL_FILES   = $(SOURCES) $(HEADERS) $(LIBRHASH_FILES) $(OTHER_FILES) $(WIN_DIST_FILES) $(I18N_FILES)
SPECFILE    = dist/rhash.spec
LIBRHASH_PC = dist/librhash.pc
RHASH_NAME     = rhash
RHASH_BINARY   = rhash$(EXEC_EXT)
CONFDIR_MACRO  = -DSYSCONFDIR=\"$(SYSCONFDIR)\"
RPMTOP  = rpms
RPMDIRS = SOURCES SPECS BUILD SRPMS RPMS
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA    = $(INSTALL) -m 644

all: $(BUILD_TARGETS)
install: build-install-binary install-data install-symlinks $(EXTRA_INSTALL)
build-static: $(RHASH_STATIC)
build-shared: $(RHASH_SHARED)
lib-shared: $(LIBRHASH_SHARED)
lib-static: $(LIBRHASH_STATIC)
install-data: install-man install-conf
uninstall: uninstall-binary uninstall-data uninstall-symlinks uninstall-lib uninstall-gmo uninstall-pkg-config

config.mak:
	echo "Run the ./configure script first" && false

# creating archives
WIN_SUFFIX     = win32
PACKAGE_NAME   = $(RHASH_NAME)-$(VERSION)
ARCHIVE_BZIP   = $(PACKAGE_NAME)-src.tar.bz2
ARCHIVE_GZIP   = $(PACKAGE_NAME)-src.tar.gz
ARCHIVE_FULL   = $(PACKAGE_NAME)-full-src.tar.gz
ARCHIVE_DEB_GZ = $(RHASH_NAME)_$(VERSION).orig.tar.gz
ARCHIVE_7Z     = $(PACKAGE_NAME)-src.tar.7z
ARCHIVE_ZIP    = $(PACKAGE_NAME)-$(WIN_SUFFIX).zip
WIN_ZIP_DIR    = RHash-$(VERSION)-$(WIN_SUFFIX)
dist: gzip gzip-bindings
dist-full: gzip-full
win-dist: zip
zip: $(ARCHIVE_ZIP)
dgz: check $(ARCHIVE_DEB_GZ)

build-install-binary: $(BUILD_TARGETS)
	+$(MAKE) install-binary

mkdir-bin:
	$(INSTALL) -d $(BINDIR)

# install binary without (re-)compilation
install-binary: mkdir-bin
	$(INSTALL_PROGRAM) $(RHASH_BINARY) $(BINDIR)/$(RHASH_BINARY)

install-man:
	$(INSTALL) -d $(MANDIR)/man1
	$(INSTALL_DATA) dist/rhash.1 $(MANDIR)/man1/rhash.1

install-conf:
	$(INSTALL) -d $(SYSCONFDIR)
	tr -d \\r < dist/rhashrc.sample > rc.tmp && $(INSTALL_DATA) rc.tmp $(SYSCONFDIR)/rhashrc
	rm -f rc.tmp

# dependencies should be properly set, otherwise 'make -j<n>' can fail
install-symlinks: mkdir-bin install-man install-binary
	cd $(BINDIR) && for f in $(SYMLINKS); do $(LN_S) $(RHASH_BINARY) $$f$(EXEC_EXT); done
	cd $(MANDIR)/man1 && for f in $(SYMLINKS); do $(LN_S) rhash.1 $$f.1; done

install-pkg-config:
	$(INSTALL) -d $(PKGCONFIGDIR)
	$(INSTALL_DATA) $(LIBRHASH_PC) $(PKGCONFIGDIR)/

uninstall-binary:
	rm -f $(BINDIR)/$(RHASH_BINARY)

uninstall-data:
	rm -f $(MANDIR)/man1/rhash.1

uninstall-symlinks:
	for f in $(SYMLINKS); do rm -f $(BINDIR)/$$f$(EXEC_EXT) $(MANDIR)/man1/$$f.1; done

uninstall-pkg-config:
	rm -f $(PKGCONFIGDIR)/librhash.pc

uninstall-lib:
	+cd librhash && $(MAKE) uninstall-lib

install-lib-static: $(LIBRHASH_STATIC) install-lib-headers
	+cd librhash && $(MAKE) install-lib-static

install-lib-shared: $(LIBRHASH_SHARED)
	+cd librhash && $(MAKE) install-lib-shared

install-lib-headers:
	+cd librhash && $(MAKE) install-lib-headers

install-lib-so-link:
	+cd librhash && $(MAKE) install-so-link

$(LIBRHASH_SHARED): $(LIBRHASH_FILES)
	+cd librhash && $(MAKE) lib-shared

$(LIBRHASH_STATIC): $(LIBRHASH_FILES)
	+cd librhash && $(MAKE) lib-static

test-lib: test-lib-$(BUILD_TYPE)
test-lib-static: $(LIBRHASH_STATIC)
	+cd librhash && $(MAKE) test-static

test-lib-shared: $(LIBRHASH_SHARED)
	+cd librhash && $(MAKE) test-shared

test-libs: $(LIBRHASH_STATIC) $(LIBRHASH_SHARED)
	+cd librhash && $(MAKE) test-static test-shared

test: test-$(BUILD_TYPE)
test-static: $(RHASH_STATIC)
	chmod +x tests/test_rhash.sh
	tests/test_rhash.sh ./$(RHASH_STATIC)

test-shared: $(RHASH_SHARED)
	chmod +x tests/test_rhash.sh
	tests/test_rhash.sh --shared ./$(RHASH_SHARED)

print-info: lib-$(BUILD_TYPE)
	+cd librhash && $(MAKE) print-info

# check that source tree is consistent
check:
	grep -q '\* === Version $(VERSION) ===' ChangeLog
	grep -q '^#define VERSION "$(VERSION)"' version.h
	test ! -f bindings/version.properties || grep -q '^version=$(VERSION)$$' bindings/version.properties

$(RHASH_STATIC): $(OBJECTS) $(LIBRHASH_STATIC)
	$(CC) $(OBJECTS) $(LIBRHASH_STATIC) $(BIN_STATIC_LDFLAGS) -o $@

$(RHASH_SHARED): $(OBJECTS) $(LIBRHASH_SHARED)
	$(CC) $(OBJECTS) $(LIBRHASH_SHARED) $(LDFLAGS) -o $@

# NOTE: dependences were generated by 'gcc -Ilibrhash -MM *.c'
# we are using plain old makefile style to support BSD make
calc_sums.o: calc_sums.c platform.h calc_sums.h common_func.h \
 hash_check.h file.h hash_print.h output.h parse_cmdline.h rhash_main.h \
 win_utils.h librhash/rhash.h librhash/rhash_torrent.h
	$(CC) -c $(CFLAGS) $< -o $@

common_func.o: common_func.c common_func.h parse_cmdline.h version.h \
 win_utils.h
	$(CC) -c $(CFLAGS) $< -o $@

file.o: file.c file.h common_func.h win_utils.h
	$(CC) -c $(CFLAGS) $< -o $@

file_mask.o: file_mask.c file_mask.h common_func.h
	$(CC) -c $(CFLAGS) $< -o $@

file_set.o: file_set.c file_set.h calc_sums.h common_func.h hash_check.h \
 hash_print.h output.h parse_cmdline.h rhash_main.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

find_file.o: find_file.c platform.h find_file.h common_func.h file.h \
 output.h win_utils.h
	$(CC) -c $(CFLAGS) $< -o $@

hash_check.o: hash_check.c hash_check.h hash_print.h common_func.h \
 output.h parse_cmdline.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

hash_print.o: hash_print.c hash_print.h calc_sums.h common_func.h \
 hash_check.h file.h parse_cmdline.h win_utils.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

hash_update.o: hash_update.c common_func.h calc_sums.h hash_check.h \
 file.h file_set.h file_mask.h hash_print.h hash_update.h output.h \
 parse_cmdline.h rhash_main.h win_utils.h
	$(CC) -c $(CFLAGS) $< -o $@

output.o: output.c platform.h output.h calc_sums.h common_func.h \
 hash_check.h parse_cmdline.h rhash_main.h win_utils.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

parse_cmdline.o: parse_cmdline.c parse_cmdline.h common_func.h \
 file_mask.h find_file.h file.h hash_print.h output.h rhash_main.h \
 win_utils.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

rhash_main.o: rhash_main.c rhash_main.h calc_sums.h common_func.h \
 hash_check.h file_mask.h find_file.h file.h hash_print.h hash_update.h \
 parse_cmdline.h output.h win_utils.h librhash/rhash.h
	$(CC) -c $(CFLAGS) $< -o $@

win_utils.o: win_utils.c win_utils.h common_func.h file.h parse_cmdline.h \
 rhash_main.h
	$(CC) -c $(CFLAGS) $< -o $@

dist/rhash.1.win.html: dist/rhash.1 dist/rhash.1.win.sed
	sed -f dist/rhash.1.win.sed dist/rhash.1 | rman -fHTML -roff | \
	sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > dist/rhash.1.win.html
#	verify the result
	grep -q "utf8" dist/rhash.1.win.html
	grep -q "APPDATA" dist/rhash.1.win.html

dist/rhash.1.html: dist/rhash.1
	-which rman 2>/dev/null && (rman -fHTML -roff dist/rhash.1 | sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > $@)

dist/rhash.1.txt: dist/rhash.1
	-which groff &>/dev/null && (groff -t -e -mandoc -Tascii dist/rhash.1 | sed -e 's/.\[[0-9]*m//g' > $@)

permissions:
	find . dist librhash po win32 win32/vc-2010 -maxdepth 1 -type f -exec chmod -x '{}' \;
	chmod +x configure tests/test_rhash.sh

copy-dist: $(ALL_FILES) permissions
	rm -rf $(PACKAGE_NAME)
	mkdir $(PACKAGE_NAME)
	cp -rl --parents $(ALL_FILES) $(PACKAGE_NAME)/

gzip: check
	+$(MAKE) copy-dist
	tar czf $(ARCHIVE_GZIP) --owner=root --group=root $(PACKAGE_NAME)/
	rm -rf $(PACKAGE_NAME)

gzip-bindings:
	+cd bindings && $(MAKE) gzip ARCHIVE_GZIP=../rhash-bindings-$(VERSION)-src.tar.gz

gzip-full: check clean-bindings
	+$(MAKE) copy-dist
	+cd bindings && $(MAKE) copy-dist COPYDIR=../$(PACKAGE_NAME)/bindings
	tar czf $(ARCHIVE_FULL) --owner=root:0 --group=root:0 $(PACKAGE_NAME)/
	rm -rf $(PACKAGE_NAME)

bzip: check
	+$(MAKE) copy-dist
	tar cjf $(ARCHIVE_BZIP) --owner=root:0 --group=root:0 $(PACKAGE_NAME)/
	rm -rf $(PACKAGE_NAME)

7z: check
	+$(MAKE) copy-dist
	tar cf - --owner=root:0 --group=root:0 $(PACKAGE_NAME)/ | 7zr a -si $(ARCHIVE_7Z)
	rm -rf $(PACKAGE_NAME)

$(ARCHIVE_ZIP): $(WIN_DIST_FILES) dist/rhash.1.win.html
	test -s dist/rhash.1.win.html && test -x $(RHASH_BINARY)
	-rm -rf $(WIN_ZIP_DIR)
	mkdir $(WIN_ZIP_DIR)
	cp $(RHASH_BINARY) ChangeLog $(WIN_DIST_FILES) $(WIN_ZIP_DIR)/
	cp dist/rhash.1.win.html $(WIN_ZIP_DIR)/rhash-doc.html
	zip -9r $(ARCHIVE_ZIP) $(WIN_ZIP_DIR)
	rm -rf $(WIN_ZIP_DIR)

$(ARCHIVE_DEB_GZ) : $(ALL_FILES)
	+$(MAKE) $(ARCHIVE_GZIP)
	mv -f $(ARCHIVE_GZIP) $(ARCHIVE_DEB_GZ)

# rpm packaging
$(SPECFILE): $(SPECFILE).in config.mak
	sed -e 's/@VERSION@/$(VERSION)/' $(SPECFILE).in > $(SPECFILE)

rpm: gzip
	-for i in $(RPMDIRS); do mkdir -p $(RPMTOP)/$$i; done
	cp -f $(ARCHIVE_GZIP) $(RPMTOP)/SOURCES
	rpmbuild -ba --clean --define "_topdir `pwd`/$(RPMTOP)" $(SPECFILE)
	mv -f `find $(RPMTOP) -name "*rhash*-$(VERSION)*.rpm"` .
	rm -rf $(RPMTOP)

clean-bindings:
	+cd bindings && $(MAKE) clean

clean-local:
	rm -f *.o $(RHASH_SHARED) $(RHASH_STATIC)
	rm -f po/*.gmo po/*.po~

distclean: clean-local
	rm -f config.log config.mak $(SPECFILE) $(LIBRHASH_PC)
	+cd librhash && $(MAKE) distclean

clean: clean-local
	+cd librhash && $(MAKE) clean

update-po:
	xgettext *.c -k_ -cTRANSLATORS -o po/rhash.pot \
		--msgid-bugs-address='Aleksey <rhash.admin@gmail.com>' --package-name='RHash'
	for f in $(I18N_FILES); do \
		msgmerge -U $$f po/rhash.pot; \
	done

compile-gmo:
	for f in $(I18N_FILES); do \
		g=`basename $$f .po`; \
		msgfmt -o po/$$g.gmo $$f; \
	done

install-gmo: compile-gmo
	for f in $(I18N_FILES); do \
		l=`basename $$f .po`; \
		$(INSTALL) -d $(LOCALEDIR)/$$l/LC_MESSAGES; \
		$(INSTALL_DATA) po/$$l.gmo $(LOCALEDIR)/$$l/LC_MESSAGES/rhash.mo; \
	done

uninstall-gmo:
	for f in $(I18N_FILES); do \
		rm -f $(LOCALEDIR)/`basename $$f .po`/LC_MESSAGES/rhash.mo; \
	done

.PHONY: all build-shared build-static lib-shared lib-static clean clean-bindings distclean clean-local \
	test test-shared test-static test-lib test-libs test-lib-shared test-lib-static \
	install build-install-binary install-binary install-lib-shared install-lib-static \
	install-lib-headers install-lib-so-link install-conf install-data install-gmo install-man \
	install-symlinks install-pkg-config uninstall-gmo uninstall-pkg-config \
	uninstall uninstall-binary uninstall-data uninstall-lib uninstall-symlinks \
	print-info check copy-dist update-po compile-gmo mkdir-bin permissions \
	bzip dgz dist dist-full gzip gzip-bindings gzip-full rpm win-dist zip
