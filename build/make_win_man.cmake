# make_win_man.cmake
# Do something like this:

#sed -f dist/rhash.1.win.sed dist/rhash.1 > dist/rhash.1.win

#Parameters:
# SED - fully-qualified name to sed
# SED_FILE = fully-qualified name execute "sed"
# MAN_FILE = fully-qualified name for MAN file to convert
# WIN_FILE = fully-qualified name for output .HTML file

if (NOT SED)
  message(FATAL_ERROR "SED parameter is empty")
endif(NOT SED)
if (NOT WIN_FILE)
  message(FATAL_ERROR "WIN_FILE parameter is empty")
endif(NOT WIN_FILE)
if (NOT MAN_FILE)
  message(FATAL_ERROR "MAN_FILE parameter is empty")
endif(NOT MAN_FILE)
if (NOT SED_FILE)
  message(FATAL_ERROR "SED_FILE parameter is empty")
endif(NOT SED_FILE)

get_filename_component(DEST_WIN_DIR ${WIN_FILE} DIRECTORY)
get_filename_component(DEST_WIN_FN ${MAN_FILE} NAME)
message(STATUS ${DEST_WIN_DIR})
message(STATUS ${DEST_WIN_FN})
file(COPY ${MAN_FILE} DESTINATION ${DEST_WIN_DIR})
file(RENAME ${DEST_WIN_DIR}/${DEST_WIN_FN} ${WIN_FILE})
execute_process(COMMAND ${SED} -f ${SED_FILE} -i ${WIN_FILE})
