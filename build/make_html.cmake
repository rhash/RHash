# make_html.cmake
# Do something like this:

#dist/rhash.1.html: dist/rhash.1
#	-which rman 2>/dev/null && (rman -fHTML -roff dist/rhash.1 | sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' > $@)

#Parameters:
# RMAN - fully-qualified name to Polyglot program (or rman)
# MAN_FILE = fully-qualified name for MAN file to convert
# HTML_FILE = fully-qualified name for output .HTML file
if (NOT HTML_FILE)
  message(FATAL_ERROR "HTML_FILE parameter is empty")
endif(NOT HTML_FILE)

if(RMAN)
  file(TO_NATIVE_PATH ${MAN_FILE} NATIVE_MAN_FILE)
  execute_process(COMMAND ${RMAN} -fHTML -roff ${NATIVE_MAN_FILE} OUTPUT_VARIABLE FILE_CONTENTS)
#I'm not entirely clear what this does:
#sed -e '/<BODY/s/\(bgcolor=\)"[^"]*"/\1"white"/i' 
#
#But I'm not sure if that is working at all.
#
#But it's probably a good idea to remove the bgcolor attribute and value from this.
  string(REGEX REPLACE FILE_CONTENTS "<body bgcolor=.*>" "<body>" "${FILE_CONTENTS}")
  file(WRITE ${HTML_FILE} "${FILE_CONTENTS}")
endif(RMAN)
