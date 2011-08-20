# Sample usage:
# compile with debug info: make CFLAGS=-g
# compile for pentiumpro: make CFLAGS="-O2 -DNDEBUG -march=i586 -mcpu=pentiumpro -fomit-frame-pointer"
# create rpm with statically linked program: make rpm ADDLDFLAGS="-static -s -Wl,--gc-sections"
CC      = gcc
VERSION = 1.2.7
PREFIX  = /usr/local
# using OPTFLAGS/OPTLDFLAGS for compatibilty with old scripts using this makefile
OPTFLAGS = -O2 -DNDEBUG -fomit-frame-pointer -ffunction-sections -fdata-sections
OPTLDFLAGS =
CFLAGS = $(OPTFLAGS)
LDFLAGS = $(OPTLDFLAGS)
ADDCFLAGS =
ADDLDFLAGS =
ALLCFLAGS = -pipe $(CFLAGS) $(ADDCFLAGS) \
  -Wall -W -Wstrict-prototypes -Wnested-externs -Winline -Wpointer-arith \
  -Wbad-function-cast -Wmissing-prototypes -Wmissing-declarations
LDLIBRHASH = -Llibrhash -lrhash
ALLLDFLAGS = $(LDLIBRHASH) $(LDFLAGS) $(ADDLDFLAGS)
HEADERS = calc_sums.h crc_print.h common_func.h crc_update.h file_mask.h file_set.h find_file.h hash_check.h output.h parse_cmdline.h rhash_main.h win_utils.h version.h
SOURCES = calc_sums.c crc_print.c common_func.c crc_update.c file_mask.c file_set.c find_file.c hash_check.c output.c parse_cmdline.c rhash_main.c win_utils.c
OBJECTS = calc_sums.o crc_print.o common_func.o crc_update.o file_mask.o file_set.o find_file.o hash_check.o output.o parse_cmdline.o rhash_main.o win_utils.o
OUTDIR   =
PROGNAME = rhash
TARGET   = $(OUTDIR)$(PROGNAME)
SYMLINKS = sfv-hash tiger-hash tth-hash whirlpool-hash has160-hash gost-hash ed2k-link magnet-link
SPECFILE = dist/rhash.spec
LIN_DIST_FILES = Makefile ChangeLog INSTALL COPYING README \
  $(SPECFILE) $(SPECFILE).in $(SOURCES) $(HEADERS) tests/test_rhash.sh \
  dist/rhash.1 dist/rhash.1.win.sed dist/rhash.1.html
WIN_DIST_FILES = dist/MD5.bat dist/magnet.bat dist/rhashrc.sample
WIN_SRC_FILES  = win32/dirent.h win32/stdint.h win32/unistd.h win32/platform-dependent.h \
  win32/vc-2010/rhash.vcxproj
LIBRHASH_FILES  = librhash/algorithms.c librhash/algorithms.h \
  librhash/byte_order.c librhash/byte_order.h librhash/timing.c librhash/timing.h \
  librhash/plug_openssl.c librhash/plug_openssl.h librhash/rhash.c librhash/rhash.h \
  librhash/aich.c librhash/aich.h librhash/crc32.c librhash/crc32.h \
  librhash/ed2k.c librhash/ed2k.h librhash/edonr.c librhash/edonr.h \
  librhash/gost.c librhash/gost.h librhash/has160.c librhash/has160.h \
  librhash/hex.c librhash/hex.h librhash/md4.c librhash/md4.h librhash/md5.c librhash/md5.h \
  librhash/ripemd-160.c librhash/ripemd-160.h librhash/sha1.c librhash/sha1.h \
  librhash/sha256.c librhash/sha256.h librhash/sha512.c librhash/sha512.h \
  librhash/snefru.c librhash/snefru.h librhash/tiger.c librhash/tiger.h \
  librhash/tiger_sbox.c librhash/tth.c librhash/tth.h librhash/whirlpool.c \
  librhash/whirlpool.h librhash/whirlpool_sbox.c librhash/test_hashes.c \
  librhash/test_hashes.h librhash/torrent.h librhash/torrent.c \
  librhash/util.c librhash/util.h librhash/config.h librhash/Makefile
DIST_FILES     = $(LIN_DIST_FILES) $(LIBRHASH_FILES) $(WIN_DIST_FILES) $(WIN_SRC_FILES)
WIN_SUFFIX     = win32
ARCHIVE_BZIP   = rhash-$(VERSION)-src.tar.bz2
ARCHIVE_GZIP   = rhash-$(VERSION)-src.tar.gz
ARCHIVE_DEB_GZ = ../rhash_$(VERSION).orig.tar.gz
ARCHIVE_7Z     = rhash-$(VERSION)-src.tar.7z
ARCHIVE_ZIP    = rhash-$(VERSION)-$(WIN_SUFFIX).zip
WIN_ZIP_DIR    = RHash-$(VERSION)-$(WIN_SUFFIX)
DESTDIR =
BINDIR  = $(PREFIX)/bin
MANDIR  = $(PREFIX)/share/man
RPMTOP  = rpms
RPMDIRS = SOURCES SPECS BUILD SRPMS RPMS
LIBRHASH = librhash/librhash.a
# Set variables according to GNU coding standard
INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA    = $(INSTALL) -m 644

all: $(TARGET)
install: install-program install-symlinks
uninstall: uninstall-program uninstall-symlinks

# creating archives
dist: gzip
gzip: check $(ARCHIVE_GZIP)
bzip: check $(ARCHIVE_BZIP)
7z:   check $(ARCHIVE_7Z)
zip : $(ARCHIVE_ZIP)
win-dist : $(ARCHIVE_ZIP)

install-program: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)/man1 $(DESTDIR)/etc
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(BINDIR)
	$(INSTALL_DATA) dist/rhash.1 $(DESTDIR)$(MANDIR)/man1/rhash.1
	sed -e 's/\x0D//g' dist/rhashrc.sample > rhashrc && $(INSTALL_DATA) rhashrc $(DESTDIR)/etc/rhashrc
	rm -f rhashrc

# dependencies should be properly set, otherwise 'make -j<n>' can run install targets in parallel
install-symlinks: install-program
	for f in $(SYMLINKS); do ln -s rhash $(DESTDIR)$(BINDIR)/$$f; done
	cd $(DESTDIR)$(MANDIR)/man1 && for f in $(SYMLINKS); do ln -s rhash.1* $$f.1; done

uninstall-program:
	rm -f $(DESTDIR)$(BINDIR)/$(PROGNAME)
	rm -f $(DESTDIR)$(MANDIR)/man1/rhash.1

uninstall-symlinks:
	for f in $(SYMLINKS); do rm -f $(DESTDIR)$(BINDIR)/$$f; done

install-lib-static:
	cd librhash && make install-lib-static

install-lib-shared:
	cd librhash && make install-lib-shared

lib-static: $(LIBRHASH)

lib-shared :
	cd librhash && make lib-shared

test-hashes:
	cd librhash && make test

test: $(TARGET) test-hashes
	chmod +x tests/test_$(PROGNAME).sh
	tests/test_$(PROGNAME).sh

version.h : Makefile
	echo "#define VERSION \"$(VERSION)\"" > version.h

check: version.h
# check version
	grep -q '\* === Version $(VERSION) ===' ChangeLog
	grep -q '^#define VERSION "$(VERSION)"' version.h
	[ -s dist/rhash.1.html ]

$(LIBRHASH): $(LIBRHASH_FILES)
	cd librhash && make lib-static

$(TARGET): $(OBJECTS) $(LIBRHASH)
	$(CC) $(OBJECTS) -o $(TARGET) $(ALLLDFLAGS)

# NOTE: dependences were generated by 'gcc -Ilibrhash -MM *.c'
# we are using plain old makefile style to support BSD make
calc_sums.o: calc_sums.c common_func.h librhash/util.h librhash/hex.h \
 librhash/rhash.h librhash/timing.h parse_cmdline.h rhash_main.h \
 file_set.h calc_sums.h hash_check.h crc_print.h output.h win_utils.h \
 version.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

common_func.o: common_func.c common_func.h librhash/util.h librhash/hex.h \
 win_utils.h parse_cmdline.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

crc_print.o: crc_print.c common_func.h librhash/util.h librhash/hex.h \
 librhash/byte_order.h calc_sums.h librhash/rhash.h hash_check.h \
 parse_cmdline.h crc_print.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

crc_update.o: crc_update.c common_func.h librhash/util.h \
 librhash/timing.h win_utils.h parse_cmdline.h output.h rhash_main.h \
 file_set.h calc_sums.h librhash/rhash.h hash_check.h file_mask.h \
 crc_update.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

file_mask.o: file_mask.c common_func.h librhash/util.h file_mask.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

file_set.o: file_set.c librhash/hex.h librhash/crc32.h common_func.h \
 librhash/util.h crc_print.h parse_cmdline.h rhash_main.h output.h \
 file_set.h calc_sums.h librhash/rhash.h hash_check.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

find_file.o: find_file.c common_func.h librhash/util.h win_utils.h \
 find_file.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

hash_check.o: hash_check.c librhash/hex.h librhash/byte_order.h \
 librhash/rhash.h output.h parse_cmdline.h crc_print.h hash_check.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

output.o: output.c common_func.h librhash/util.h librhash/rhash.h \
 calc_sums.h hash_check.h parse_cmdline.h rhash_main.h output.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

parse_cmdline.o: parse_cmdline.c common_func.h librhash/util.h \
 librhash/rhash.h librhash/plug_openssl.h win_utils.h file_mask.h \
 crc_print.h output.h rhash_main.h version.h parse_cmdline.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

rhash_main.o: rhash_main.c common_func.h librhash/util.h \
 librhash/timing.h win_utils.h find_file.h file_set.h calc_sums.h \
 librhash/rhash.h hash_check.h crc_update.h file_mask.h crc_print.h \
 parse_cmdline.h output.h version.h rhash_main.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

win_utils.o: win_utils.c common_func.h librhash/util.h parse_cmdline.h \
 rhash_main.h win_utils.h
	$(CC) -c $(ALLCFLAGS) $< -o $@

dist/rhash.1.win.html: dist/rhash.1 dist/rhash.1.win.sed
	sed -f dist/rhash.1.win.sed dist/rhash.1 | rman -fHTML -roff | \
	sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > dist/rhash.1.win.html
#	verify the result
	grep -q "utf8" dist/rhash.1.win.html
	grep -q "APPDATA" dist/rhash.1.win.html

dist/rhash.1.html: dist/rhash.1
	-which rman &>/dev/null && (rman -fHTML -roff dist/rhash.1 | sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > $@)

dist/rhash.1.txt: dist/rhash.1
	-which groff &>/dev/null && (groff -t -e -mandoc -Tascii dist/rhash.1 | sed -e 's/.\[[0-9]*m//g' > $@)

cpp-doc:
	cppdoc_cmd -title=RHash -company=Animegorodok -classdir=classdoc -module="cppdoc-standard" -overwrite -extensions="c,h" -languages="c=cpp,h=cpp" -generate-deprecations-list=false $(SOURCES) $(HEADERS) ./Documentation/CppDoc/index.html

permissions:
	chmod -x $(DIST_FILES)
	chmod +x tests/test_$(PROGNAME).sh

$(ARCHIVE_GZIP): $(DIST_FILES)
	make permissions
	rm -rf $(PROGNAME)-$(VERSION)
	mkdir $(PROGNAME)-$(VERSION)
	cp -rl --parents $(DIST_FILES) $(PROGNAME)-$(VERSION)/
	tar czf $(ARCHIVE_GZIP) $(PROGNAME)-$(VERSION)/
	rm -rf $(PROGNAME)-$(VERSION)

$(ARCHIVE_BZIP): $(DIST_FILES)
	make permissions
	rm -rf $(PROGNAME)-$(VERSION)
	mkdir $(PROGNAME)-$(VERSION)
	cp -rl --parents $(DIST_FILES) $(PROGNAME)-$(VERSION)/
	tar cjf $(ARCHIVE_BZIP) $(PROGNAME)-$(VERSION)/
	rm -rf $(PROGNAME)-$(VERSION)

$(ARCHIVE_7Z): $(DIST_FILES)
	make permissions
	rm -rf $(PROGNAME)-$(VERSION)
	mkdir $(PROGNAME)-$(VERSION)
	cp -rl --parents $(DIST_FILES) $(PROGNAME)-$(VERSION)/
	tar cf - $(PROGNAME)-$(VERSION)/ | 7zr a -si $(ARCHIVE_7Z)
	rm -rf $(PROGNAME)-$(VERSION)

$(ARCHIVE_ZIP): $(WIN_DIST_FILES) dist/rhash.1.win.html
	[ -s dist/rhash.1.win.html -a -x $(TARGET) ]
	-rm -rf $(WIN_ZIP_DIR)
	mkdir $(WIN_ZIP_DIR)
	cp $(TARGET).exe ChangeLog $(WIN_DIST_FILES) $(WIN_ZIP_DIR)/
	cp dist/rhash.1.win.html $(WIN_ZIP_DIR)/rhash-doc.html
	-[ -f $(OUTDIR)libeay32.dll ] && cp $(OUTDIR)libeay32.dll $(WIN_ZIP_DIR)/
	zip -9r $(ARCHIVE_ZIP) $(WIN_ZIP_DIR)
	rm -rf $(WIN_ZIP_DIR)

$(ARCHIVE_DEB_GZ) : $(DIST_FILES)
	make $(ARCHIVE_GZIP)
	mv -f $(ARCHIVE_GZIP) $(ARCHIVE_DEB_GZ)

# rpm packaging
$(SPECFILE): $(SPECFILE).in Makefile
	sed -e 's/@VERSION@/$(VERSION)/' $(SPECFILE).in > $(SPECFILE)

rpm: gzip
	-for i in $(RPMDIRS); do mkdir -p $(RPMTOP)/$$i; done
	cp -f $(ARCHIVE_GZIP) $(RPMTOP)/SOURCES
	rpmbuild -ba --clean --define "_topdir `pwd`/$(RPMTOP)" $(SPECFILE)
	mv -f `find $(RPMTOP) -name "*rhash*-$(VERSION)*.rpm"` .
	rm -rf $(RPMTOP)

distclean: clean

clean:
	cd librhash && make clean
	rm -f *.o $(TARGET)
