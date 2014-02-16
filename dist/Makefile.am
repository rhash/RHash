SPECFILE = rhash.spec
EXTRA_DIST = $(SPECFILE) $(SPECFILE).in rhash.1.win.sed rhash.1.html MD5.bat magnet.bat rhashrc.sample $(man1_MANS)

man1_MANS = rhash.1

sysconf_DATA = rhashrc

CLEANFILES = rhashrc

rhashrc: rhashrc.sample
	sed -e 's/\x0D//g' $(srcdir)/rhashrc.sample > rhashrc

rhash.1.win.html: rhash.1 rhash.1.win.sed
	sed -f rhash.1.win.sed rhash.1 | rman -fHTML -roff | \
	sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > rhash.1.win.html
#	verify the result
	grep -q "utf8" rhash.1.win.html
	grep -q "APPDATA" rhash.1.win.html

rhash.1.html: rhash.1
	-which rman 2>/dev/null && (rman -fHTML -roff rhash.1 | sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > $@)

rhash.1.txt: rhash.1
	-which groff &>/dev/null && (groff -t -e -mandoc -Tascii rhash.1 | sed -e 's/.\[[0-9]*m//g' > $@)

# rpm packaging
$(SPECFILE): $(SPECFILE).in $(top_srcdir)/configure.ac
	sed -e 's/@VERSION@/$(VERSION)/' $(SPECFILE).in > $(SPECFILE)
