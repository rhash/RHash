#!/bin/sed
/^\.IP "--sfv"/ {
i \
.IP "--ansi"\
Use Windows codepage for output.\
.IP "--oem"\
Use DOS (OEM) codepage for output.\
.IP "--utf8"\
Use UTF8 codepage for output.
}
/^\$HOME/s/.*rhashrc/%APPDATA%\\RHash\\rhashrc and %HOMEDRIVE%%HOMEPATH%\\rhashrc/
