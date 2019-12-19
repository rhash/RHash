cmake_minimum_required(VERSION 3.1)
project(rhash)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.h" versionfile)
string(REGEX MATCH "#define VERSION \"([0-9]*)\.([0-9]*)\.([0-9]*)\"" _ ${versionfile})
set(RHASH_VERSION_MAJOR ${CMAKE_MATCH_1})
set(RHASH_VERSION_MINOR ${CMAKE_MATCH_2})
set(RHASH_VERSION_PATCH ${CMAKE_MATCH_3})
set(RHASH_VERSION "${RHASH_VERSION_MAJOR}.${RHASH_VERSION_MINOR}.${RHASH_VERSION_PATCH}")

option(USE_GETTEXT "Enable gettext (localization) support")

set(SOURCE_FILES    "${CMAKE_CURRENT_SOURCE_DIR}/calc_sums.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_print.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/common_func.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_update.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file_mask.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file_set.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/find_file.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_check.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/output.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/parse_cmdline.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_main.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/win_utils.c")

set(HEADER_FILES    "${CMAKE_CURRENT_SOURCE_DIR}/calc_sums.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_print.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/common_func.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_update.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file_mask.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/file_set.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/find_file.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hash_check.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/output.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/parse_cmdline.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_main.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/win_utils.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/platform.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/version.h")

add_subdirectory("librhash")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME} librhash)

if (USE_GETTEXT)
    find_package(Intl REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${Intl_LIBRARIES})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${Intl_INCLUDE_DIRS})
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_GETTEXT)
endif()

install(TARGETS ${CMAKE_PROJECT_NAME}
       RUNTIME DESTINATION bin)
