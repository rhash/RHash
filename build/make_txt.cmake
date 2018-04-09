# make_txt.cmake
# Do something like this:

#dist/rhash.1.txt: dist/rhash.1
#	-which groff &>/dev/null && (groff -t -e -mandoc -Tascii dist/rhash.1 | sed -e 's/.\[[0-9]*m//g' > $@)

#Parameters:
# GROFF - fully-qualified name to Polyglot program (or rman)
# MAN_FILE = fully-qualified name for MAN file to convert
# TXT_FILE = fully-qualified name for output .TXT file
if (NOT TXT_FILE)
  message(FATAL_ERROR "TXT_FILE parameter is empty")
endif(NOT TXT_FILE)

if(GROFF)
#  file(TO_NATIVE_PATH ${MAN_FILE} NATIVE_MAN_FILE)

#groff will output warnings such as:
# invalid input character code `13'
  execute_process(COMMAND ${GROFF}  -t -e -mandoc -Tascii ${MAN_FILE} OUTPUT_VARIABLE FILE_CONTENTS ERROR_VARIABLE ERROR_CONTENTS)
  string(REGEX REPLACE FILE_CONTENTS ".[[0-9]*m" "" "${FILE_CONTENTS}")
  file(WRITE ${TXT_FILE} "${FILE_CONTENTS}")
endif(GROFF)
