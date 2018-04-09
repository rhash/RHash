cmake_minimum_required(VERSION 3.2.3)
project(rhash)
option(BUILD_SHARED_LIBS "Build shared FLIF encoder/decoder libraries" ON)
option(BUILD_STATIC_LIBS "Build static FLIF encoder/decoder libraries" ON)
option(OPENSSL_RUNTIME "load OpenSSL at runtime if present" ON)

include(GNUInstallDirs)
include(FindPkgConfig)
include(FindOpenSSL)
include(FindGettext)
include(FindIntl)

enable_testing()

set(SOVERSION 0)
set(RHASH_SRC_DIR   ${CMAKE_CURRENT_LIST_DIR}/..)
set(RHASH_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(RHASH_LIB_DIR   ${RHASH_SRC_DIR}/librhash)
set(RHASH_LIB_HEADERS 
	${RHASH_LIB_DIR}/algorithms.h
	${RHASH_LIB_DIR}/byte_order.h
	${RHASH_LIB_DIR}/plug_openssl.h
	${RHASH_LIB_DIR}/rhash.h 
	${RHASH_LIB_DIR}/rhash_timing.h
	${RHASH_LIB_DIR}/rhash_torrent.h
	${RHASH_LIB_DIR}/aich.h
	${RHASH_LIB_DIR}/crc32.h
	${RHASH_LIB_DIR}/ed2k.h
	${RHASH_LIB_DIR}/edonr.h
	${RHASH_LIB_DIR}/hex.h
	${RHASH_LIB_DIR}/md4.h 
	${RHASH_LIB_DIR}/md5.h
	${RHASH_LIB_DIR}/sha1.h
	${RHASH_LIB_DIR}/sha256.h
	${RHASH_LIB_DIR}/sha512.h 
	${RHASH_LIB_DIR}/sha3.h
	${RHASH_LIB_DIR}/ripemd-160.h 
	${RHASH_LIB_DIR}/gost.h
	${RHASH_LIB_DIR}/has160.h
	${RHASH_LIB_DIR}/snefru.h
	${RHASH_LIB_DIR}/tiger.h
	${RHASH_LIB_DIR}/tth.h
	${RHASH_LIB_DIR}/torrent.h
	${RHASH_LIB_DIR}/ustd.h
	${RHASH_LIB_DIR}/util.h
	${RHASH_LIB_DIR}/whirlpool.h)

set(RHASH_LIB_SOURCES 
	${RHASH_LIB_DIR}/algorithms.c
	${RHASH_LIB_DIR}/byte_order.c
	${RHASH_LIB_DIR}/plug_openssl.c
	${RHASH_LIB_DIR}/rhash.c
	${RHASH_LIB_DIR}/rhash_timing.c
	${RHASH_LIB_DIR}/rhash_torrent.c
	${RHASH_LIB_DIR}/aich.c
	${RHASH_LIB_DIR}/crc32.c
	${RHASH_LIB_DIR}/ed2k.c
	${RHASH_LIB_DIR}/edonr.c
	${RHASH_LIB_DIR}/hex.c
	${RHASH_LIB_DIR}/md4.c
	${RHASH_LIB_DIR}/md5.c
	${RHASH_LIB_DIR}/sha1.c
	${RHASH_LIB_DIR}/sha256.c
	${RHASH_LIB_DIR}/sha512.c
	${RHASH_LIB_DIR}/sha3.c
	${RHASH_LIB_DIR}/ripemd-160.c
	${RHASH_LIB_DIR}/gost.c
	${RHASH_LIB_DIR}/has160.c
	${RHASH_LIB_DIR}/snefru.c
	${RHASH_LIB_DIR}/tiger.c
	${RHASH_LIB_DIR}/tiger_sbox.c
	${RHASH_LIB_DIR}/tth.c
	${RHASH_LIB_DIR}/torrent.c
	${RHASH_LIB_DIR}/whirlpool.c
	${RHASH_LIB_DIR}/whirlpool_sbox.c)
set(RHASH_LIB_PUBLIC_HEADERS 
	${RHASH_LIB_DIR}/rhash.h
	${RHASH_LIB_DIR}/rhash_torrent.h)
set(RHASH_LIB_TEXT_SOURCES 
        ${RHASH_LIB_DIR}/test_hashes.c
        ${RHASH_LIB_DIR}/test_hashes.h)
set(RHASH_LIB_DEPEND_LIBS "")
set(RHASH_EXE_HEADERS 
	${RHASH_SRC_DIR}/calc_sums.h 
	${RHASH_SRC_DIR}/hash_print.h
	${RHASH_SRC_DIR}/common_func.h
	${RHASH_SRC_DIR}/hash_update.h
	${RHASH_SRC_DIR}/file.h
	${RHASH_SRC_DIR}/file_mask.h 
	${RHASH_SRC_DIR}/file_set.h 
	${RHASH_SRC_DIR}/find_file.h 
	${RHASH_SRC_DIR}/hash_check.h 
	${RHASH_SRC_DIR}/output.h 
	${RHASH_SRC_DIR}/parse_cmdline.h 
	${RHASH_SRC_DIR}/rhash_main.h 
	${RHASH_SRC_DIR}/win_utils.h 
	${RHASH_SRC_DIR}/platform.h 
	${RHASH_SRC_DIR}/version.h)
set(WIN_BAT_FILES 
	${RHASH_SRC_DIR}/dist/MD5.bat
	${RHASH_SRC_DIR}/dist/magnet.bat)
set(WIN_DIST_FILES ${WIN_BAT_FILES}
	${RHASH_SRC_DIR}/dist/rhashrc.sample)  
set(RHASH_EXE_SOURCES 
	${RHASH_SRC_DIR}/calc_sums.c 
	${RHASH_SRC_DIR}/hash_print.c
	${RHASH_SRC_DIR}/common_func.c
	${RHASH_SRC_DIR}/hash_update.c
	${RHASH_SRC_DIR}/file.c 
	${RHASH_SRC_DIR}/file_mask.c 
	${RHASH_SRC_DIR}/file_set.c 
	${RHASH_SRC_DIR}/find_file.c 
	${RHASH_SRC_DIR}/hash_check.c 
	${RHASH_SRC_DIR}/output.c 
	${RHASH_SRC_DIR}/parse_cmdline.c 
	${RHASH_SRC_DIR}/rhash_main.c 
	${RHASH_SRC_DIR}/win_utils.c)
set(RHASH_PO_DIR ${RHASH_SRC_DIR}/po)
set(I18N_FILES
  ${RHASH_PO_DIR}/ca.po
  ${RHASH_PO_DIR}/de.po
  ${RHASH_PO_DIR}/en_AU.po
  ${RHASH_PO_DIR}/es.po 
  ${RHASH_PO_DIR}/fr.po 
  ${RHASH_PO_DIR}/gl.po 
  ${RHASH_PO_DIR}/it.po
  ${RHASH_PO_DIR}/ro.po
  ${RHASH_PO_DIR}/ru.po)
set(SOURCE_RHASH_DIST_DIR ${RHASH_SRC_DIR}/dist)
set(BUILD_RHASH_DIST_DIR ${RHASH_BUILD_DIR}/dist)
set(SPECFILE     ${BUILD_RHASH_DIST_DIR}/rhash.spec)
set(LIBRHASH_PC  ${BUILD_RHASH_DIST_DIR}/librhash.pc)
set(DEFAULT_SYMLINKS 
        sfv-hash
        tiger-hash
        tth-hash
        whirlpool-hash
        has160-hash
        gost-hash
        edonr256-hash
        edonr512-hash
        ed2k-link
        magnet-link)
file(MAKE_DIRECTORY ${BUILD_RHASH_DIST_DIR})

#obtain version number from version.h

# from jsoncpp CMakeLists.txt
# Extract major, minor, patch from version text
# Parse a version string "X.Y.Z" and outputs
# version parts in ${OUPUT_PREFIX}_MAJOR, _MINOR, _PATCH.
# If parse succeeds then ${OUPUT_PREFIX}_FOUND is TRUE.
# Set variable named ${VAR_NAME} to value ${VALUE}
FUNCTION(set_using_dynamic_name VAR_NAME VALUE)
    SET( "${VAR_NAME}" "${VALUE}" PARENT_SCOPE)
ENDFUNCTION()

MACRO(jsoncpp_parse_version VERSION_TEXT OUPUT_PREFIX)
    SET(VERSION_REGEX "[0-9]+\\.[0-9]+\\.[0-9]+(-[a-zA-Z0-9_]+)?")
    IF( ${VERSION_TEXT} MATCHES ${VERSION_REGEX} )
        STRING(REGEX MATCHALL "[0-9]+|-([A-Za-z0-9_]+)" VERSION_PARTS ${VERSION_TEXT})
        LIST(GET VERSION_PARTS 0 ${OUPUT_PREFIX}_MAJOR)
        LIST(GET VERSION_PARTS 1 ${OUPUT_PREFIX}_MINOR)
        LIST(GET VERSION_PARTS 2 ${OUPUT_PREFIX}_PATCH)
        set_using_dynamic_name( "${OUPUT_PREFIX}_FOUND" TRUE )
    ELSE( ${VERSION_TEXT} MATCHES ${VERSION_REGEX} )
        set_using_dynamic_name( "${OUPUT_PREFIX}_FOUND" FALSE )
    ENDIF()
ENDMACRO()

file(READ ${RHASH_SRC_DIR}/version.h VERSION_CONTENTS)
jsoncpp_parse_version( ${VERSION_CONTENTS} RHASH_VERSION )
IF(NOT RHASH_VERSION_FOUND)
    MESSAGE(FATAL_ERROR "Failed to parse version string properly. Expect X.Y.Z")
endif(NOT RHASH_VERSION_FOUND)

message(STATUS "RHash version:           ${RHASH_VERSION_MAJOR}.${RHASH_VERSION_MINOR}.${RHASH_VERSION_PATCH}")

SET(CPACK_PACKAGE_VERSION_MAJOR "${RHASH_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${RHASH_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${RHASH_VERSION_PATCH}")
include(CPack)

#library and program libary settings
set(ADDITIONAL_LIBS "")
set(ADDITIONAL_DIRS "")
set(DEFINITIONS_FOR_ALL_TARGETS "")

message(STATUS "OpenSSL Crypto Includes: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL Crypto Library:  ${OPENSSL_CRYPTO_LIBRARY}")
if (OPENSSL_FOUND)
  list(APPEND ADDITIONAL_DIRS ${OPENSSL_INCLUDE_DIR})
  if (OPENSSL_RUNTIME)
    set(DEFINITIONS_FOR_ALL_TARGETS ${DEFINITIONS_FOR_ALL_TARGETS} OPENSSL_RUNTIME)
else()
    list(APPEND ADDITIONAL_LIBS ${OPENSSL_CRYPTO_LIBRARY})
    set(DEFINITIONS_FOR_ALL_TARGETS ${DEFINITIONS_FOR_ALL_TARGETS} USE_OPENSSL)
	list(APPEND RHASH_LIB_DEPEND_LIBS ${OPENSSL_CRYPTO_LIBRARY})
  endif(OPENSSL_RUNTIME)
endif (OPENSSL_FOUND)

if (Intl_FOUND)
  list(APPEND ADDITIONAL_DIRS ${Intl_INCLUDE_DIR})
  list(APPEND ADDITIONAL_LIBS ${Intl_LIBRARY})
endif(Intl_FOUND )
message(STATUS "Intl Includes:           ${Intl_INCLUDE_DIR}")
message(STATUS "Intl Library:            ${Intl_LIBRARY}")

add_executable(rhash_exe ${RHASH_EXE_HEADERS} ${RHASH_EXE_SOURCES})
target_include_directories(rhash_exe PRIVATE ${ADDITIONAL_DIRS})
target_link_libraries(rhash_exe PUBLIC ${ADDITIONAL_LIBS}) 
set_target_properties(rhash_exe PROPERTIES OUTPUT_NAME rhash)
install(TARGETS rhash_exe
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      PUBLIC_HEADER DESTINATION include)
message(STATUS "CMAKE_EXECUTABLE_SUFFIX  ${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_target(symlinks)
get_target_property(RASH_TARGET_NAME rhash_exe OUTPUT_NAME)

foreach(CURSYM ${DEFAULT_SYMLINKS})
 # exe file symlink
  add_custom_command(OUTPUT ${RHASH_BUILD_DIR}/${CURSYM}${CMAKE_EXECUTABLE_SUFFIX}
    DEPENDS rhash_exe
    COMMAND ${CMAKE_COMMAND} -E copy ${RHASH_BUILD_DIR}/${RASH_TARGET_NAME}${CMAKE_EXECUTABLE_SUFFIX} ${RHASH_BUILD_DIR}/${CURSYM}${CMAKE_EXECUTABLE_SUFFIX})
  add_custom_target(exe_${CURSYM} ALL DEPENDS ${RHASH_BUILD_DIR}/${CURSYM}${CMAKE_EXECUTABLE_SUFFIX})
  install(FILES ${RHASH_BUILD_DIR}/${CURSYM}${CMAKE_EXECUTABLE_SUFFIX} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
# man file symlinks
  add_custom_command(OUTPUT ${RHASH_BUILD_DIR}/${CURSYM}.1
    COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_RHASH_DIST_DIR}/${RASH_TARGET_NAME}.1 ${RHASH_BUILD_DIR}/${CURSYM}.1)
  add_custom_target(man_${CURSYM}.1 ALL DEPENDS ${RHASH_BUILD_DIR}/${CURSYM}.1)
  install(FILES ${RHASH_BUILD_DIR}/${CURSYM}.1 DESTINATION ${CMAKE_INSTALL_FULL_MANDIR})
endforeach(CURSYM)

set(RHASH_LIB_DEPEND_LIBS "${RHASH_LIB_DEPEND_LIBS};${CMAKE_DL_LIBS}")

if(BUILD_SHARED_LIBS)
    add_library(rhash_lib SHARED ${RHASH_LIB_SOURCES})
    set_target_properties(rhash_lib PROPERTIES OUTPUT_NAME rhash)
	if (WIN32 OR CYGWIN OR MSYS OR WINCE)
      set_target_properties(rhash_lib PROPERTIES PUBLIC_HEADER "${RHASH_LIB_PUBLIC_HEADERS}")   
	else()
      set_target_properties(rhash_lib PROPERTIES PUBLIC_HEADER "${RHASH_LIB_PUBLIC_HEADERS}" SOVERSION ${SOVERSION})   
	endif (WIN32 OR CYGWIN OR MSYS OR WINCE)
    target_compile_definitions(rhash_lib PRIVATE ${DEFINITIONS_FOR_ALL_TARGETS} RHASH_EXPORTS )
    target_link_libraries(rhash_lib PUBLIC ${ADDITIONAL_LIBS} ${CMAKE_DL_LIBS}) 
	install(TARGETS rhash_lib
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      PUBLIC_HEADER DESTINATION include)
    add_executable(test_exe ${RHASH_LIB_TEXT_SOURCES} )
    target_link_libraries(test_exe PUBLIC rhash_lib)
    set_target_properties(test_exe PROPERTIES OUTPUT_NAME test_shared)
    target_link_libraries(rhash_exe PUBLIC rhash_lib)
endif(BUILD_SHARED_LIBS)

if(BUILD_STATIC_LIBS)
    add_library(rhash_lib_static STATIC ${RHASH_LIB_SOURCES})
    set_target_properties(rhash_lib_static PROPERTIES OUTPUT_NAME rhash)
    if (WIN32 OR CYGWIN OR MSYS OR WINCE)
           set_target_properties(rhash_lib_static PROPERTIES PUBLIC_HEADER "${RHASH_LIB_PUBLIC_HEADERS}")   
    else()
           set_target_properties(rhash_lib_static PROPERTIES PUBLIC_HEADER "${RHASH_LIB_PUBLIC_HEADERS}" SOVERSION ${SOVERSION})   
    endif (WIN32 OR CYGWIN OR MSYS OR WINCE)
    target_compile_definitions(rhash_lib_static PRIVATE ${DEFINITIONS_FOR_ALL_TARGETS})
    target_link_libraries(rhash_lib_static PUBLIC "${RHASH_LIB_DEPEND_LIBS}")
    install(TARGETS rhash_lib_static
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      PUBLIC_HEADER DESTINATION include)
    if (NOT(BUILD_SHARED_LIBS))
       target_link_libraries(rhash_exe PUBLIC rhash_lib_static)
    else()
       target_link_libraries(rhash_exe PUBLIC rhash_lib)
    endif(NOT(BUILD_SHARED_LIBS))
    add_executable(test_exe_static ${RHASH_LIB_TEXT_SOURCES})
    target_link_libraries(test_exe_static PUBLIC rhash_lib_static)
    set_target_properties(test_exe_static PROPERTIES OUTPUT_NAME test_static)
endif(BUILD_STATIC_LIBS)

#Make pkg-config
set(PC_EXC "${CMAKE_INSTALL_PREFIX}")
set(PC_INC "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(PC_LIB "${CMAKE_INSTALL_FULL_LIBDIR}")
if("${PC_EXC}" STREQUAL "${CMAKE_INSTALL_PREFIX}")
    set(PC_EXC "\$\{prefix\}")
endif()
if ("${PC_INC}" STREQUAL "${CMAKE_INSTALL_PREFIX}/include")
    set(PC_INC "\$\{prefix\}/include")
endif()
if ("${PC_LIB}" STREQUAL "${CMAKE_INSTALL_PREFIX}/lib")
    set(PC_LIB "\$\{exec_prefix\}/lib")
endif()
  
# This stuff is necessary to ensure that the dependency list
# in our .pc file is in the proper format
set(PKGCONF_RHASH_DEPS "")
foreach(RHASH_LIB_DEP ${RHASH_LIB_DEPEND_LIBS})
  set(PKGCONF_RHASH_DEPS "${PKGCONF_RHASH_DEPS} -l${RHASH_LIB_DEP}")
endforeach(RHASH_LIB_DEP)

set(PKGCONFIG_CONTENTS "prefix=${CMAKE_INSTALL_PREFIX}\r
exec_prefix=${PC_EXC}\r
libdir=${PC_LIB}\r
includedir=${PC_INC}\r
\r
Name: librash\r
Description: LibRHash shared library\r
Version: ${RHASH_VERSION_MAJOR}.${RHASH_VERSION_MINOR}.${RHASH_VERSION_PATCH}\r
Cflags: -I\$\{includedir\}\r
Libs: -L\${libdir\} -lrhash\r
Libs.private: ${PKGCONF_RHASH_DEPS}\r
")
file(WRITE ${LIBRHASH_PC} ${PKGCONFIG_CONTENTS})
install(FILES ${LIBRHASH_PC} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  
if(GETTEXT_FOUND)
  foreach(PO_FILE ${I18N_FILES})
    get_filename_component(CUR_LANG ${PO_FILE} NAME_WE)
    GETTEXT_PROCESS_PO_FILES(${CUR_LANG} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_FULL_LOCALEDIR} PO_FILES ${PO_FILE})
  endforeach(PO_FILE)
endif(GETTEXT_FOUND)

install(FILES ${SOURCE_RHASH_DIST_DIR}/rhash.1 DESTINATION ${CMAKE_INSTALL_MANDIR})

find_program(SED sed)
if(SED)
  message(STATUS "SED                      ${SED}")
  add_custom_command(POST_BUILD OUTPUT ${BUILD_RHASH_DIST_DIR}/rhash.1.win 
    COMMENT "Creating rhash.1.win" 
    COMMAND ${CMAKE_COMMAND} -D SED=${SED} -DSED_FILE=${SOURCE_RHASH_DIST_DIR}/rhash.1.win.sed -D MAN_FILE=${SOURCE_RHASH_DIST_DIR}/rhash.1 -DWIN_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.win -P ${CMAKE_CURRENT_LIST_DIR}/make_win_man.cmake)
  install(FILES ${BUILD_RHASH_DIST_DIR}/rhash.1.win DESTINATION ${CMAKE_INSTALL_MANDIR})
endif(SED)

find_program(RMAN rman)
if(RMAN)
  message(STATUS "Polyglotman (rman)       ${RMAN}")
  add_custom_command(POST_BUILD OUTPUT ${BUILD_RHASH_DIST_DIR}/rhash.1.html COMMENT "Creating rhash.1.html" 
    COMMAND ${CMAKE_COMMAND} -D RMAN=${RMAN} -D MAN_FILE=${SOURCE_RHASH_DIST_DIR}/rhash.1 -DHTML_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.html -P ${CMAKE_CURRENT_LIST_DIR}/make_html.cmake)
  install(FILES ${BUILD_RHASH_DIST_DIR}/rhash.1.html DESTINATION ${CMAKE_INSTALL_DOCDIR}/html)
  if(SED)
    add_custom_command(POST_BUILD OUTPUT ${BUILD_RHASH_DIST_DIR}/rhash.1.win.html 
      DEPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.win 
      COMMENT "Creating rhash.1.win.html" 
      COMMAND ${CMAKE_COMMAND} -D RMAN=${RMAN} -D MAN_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.win -DHTML_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.win.html -P ${CMAKE_CURRENT_LIST_DIR}/make_html.cmake)
    install(FILES ${BUILD_RHASH_DIST_DIR}/rhash.1.win.html DESTINATION ${CMAKE_INSTALL_DOCDIR}/html)
	add_custom_target(html_doc ALL DEPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.html ${BUILD_RHASH_DIST_DIR}/rhash.1.win.html)
  else()
    add_custom_target(html_doc ALL DEPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.html)
  endif(SED)  
endif(RMAN)

find_program(GROFF groff)
if(GROFF)
#dist/rhash.1.txt: dist/rhash.1
#	-which groff &>/dev/null && (groff -t -e -mandoc -Tascii dist/rhash.1 | sed -e 's/.\[[0-9]*m//g' > $@)
  message(STATUS "groff                    ${GROFF}")
  add_custom_command(POST_BUILD OUTPUT ${BUILD_RHASH_DIST_DIR}/rhash.1.txt COMMENT "Creating rhash.1.txt" 
    COMMAND ${CMAKE_COMMAND} -D GROFF=${GROFF} -D MAN_FILE=${SOURCE_RHASH_DIST_DIR}/rhash.1 -DTXT_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.txt -P ${CMAKE_CURRENT_LIST_DIR}/make_txt.cmake)
  install(FILES ${BUILD_RHASH_DIST_DIR}/rhash.1.txt DESTINATION ${CMAKE_INSTALL_DOCDIR}/txt)
  if(SED)
    add_custom_command(POST_BUILD OUTPUT ${BUILD_RHASH_DIST_DIR}/rhash.1.win.txt D
      EPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.win COMMENT "Creating rhash.1.win.txt" 
      COMMAND ${CMAKE_COMMAND} -D GROFF=${GROFF} -D MAN_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.win -DTXT_FILE=${BUILD_RHASH_DIST_DIR}/rhash.1.win.txt -P ${CMAKE_CURRENT_LIST_DIR}/make_txt.cmake)
    install(FILES ${BUILD_RHASH_DIST_DIR}/rhash.1.win.txt DESTINATION ${CMAKE_INSTALL_DOCDIR}/txt)
    add_custom_target(txt_doc ALL DEPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.txt ${BUILD_RHASH_DIST_DIR}/rhash.1.win.txt)
  else()
    add_custom_target(txt_doc ALL DEPENDS ${BUILD_RHASH_DIST_DIR}/rhash.1.txt ${BUILD_RHASH_DIST_DIR}/rhash.1.win.txt)
  endif(SED)
endif(GROFF)

# WIN_DIST_FILES
foreach (CURDIST ${WIN_DIST_FILES})
  install(FILES ${CURDIST} DESTINATION ${CMAKE_INSTALL_FULL_DOCDIR})
endforeach(CURDIST)
# Win32 .bat files
if(WIN32)
  foreach (CURDIST ${WIN_BAT_FILES})
    install(FILES ${CURDIST} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
  endforeach(CURDIST)
endif(WIN32)

# bindings
add_custom_command(PRE_BUILD OUTPUT bindings
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/../bindings ${BUILD_RHASH_DIST_DIR}/bindings)
add_custom_command(PRE_BUILD OUTPUT bindings/version.properties
  COMMAND ${CMAKE_COMMAND} -D VERSION=${RHASH_VERSION_MAJOR}.${RHASH_VERSION_MINOR}.${RHASH_VERSION_PATCH} 
    -DVERSION_PROP_FILE=${BUILD_RHASH_DIST_DIR}/bindings/version.properties 
    -P ${CMAKE_CURRENT_LIST_DIR}/make_version_properties.cmake)
add_custom_target(bindings_dir ALL DEPENDS bindings bindings/version.properties)

