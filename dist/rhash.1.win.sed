#!/bin/sed
# insert encoding options before sfv
/^\.IP "\\-\\-sfv"/ {
i\
.IP "\\-\\-utf8"\
Use UTF\\-8 encoding for output.\
.IP "\\-\\-win"\
Use Windows codepage for output.\
.IP "\\-\\-dos"\
Use DOS (OEM) codepage for output.
}

/ looks for a config file/ {
a\
on Windows at\
%APPDATA%\\\\RHash\\\\rhashrc, %HOMEDRIVE%%HOMEPATH%\\\\rhashrc, {PROGRAM_DIRECTORY}\\\\rhashrc\
\
and on Linux/Unix
}
