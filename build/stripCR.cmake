#stripCR.cmake
#
#Strips carriage returns char(13) and writes file to DEST_FILE
#
#SOURCE_FILE = fully-qualified name of source file
#DEST_FILE = fully-qualified name of destination file

file(READ ${SOURCE_FILE} FILE_CONTENTS)
string(REGEX REPLACE FILE_CONTENTS "\r" "" "${FILE_CONTENTS}")
file(WRITE ${DEST_FILE} "${FILE_CONTENTS}")