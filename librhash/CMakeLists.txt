cmake_minimum_required(VERSION 3.1)
project(librhash LANGUAGES C)

include(GNUInstallDirs)

if(MSVC OR MSVC90 OR MSVC10)
    set(MSVC ON)
    if(BUILD_SHARED_LIBS)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

option(USE_OPENSSL "Enable OpenSSL (optimized hash functions) support")
option(OPENSSL_RUNTIME "Load OpenSSL at runtime if present")

set(SOURCE_FILES    "${CMAKE_CURRENT_SOURCE_DIR}/algorithms.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/byte_order.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/plug_openssl.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_timing.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_torrent.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/aich.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crc32.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/ed2k.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/edonr.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hex.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/md4.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/md5.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha1.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha256.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha512.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha3.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/ripemd-160.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/gost12.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/gost94.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/has160.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/snefru.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tiger.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tiger_sbox.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tth.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/torrent.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/whirlpool.c"
                    "${CMAKE_CURRENT_SOURCE_DIR}/whirlpool_sbox.c")

set(HEADER_FILES    "${CMAKE_CURRENT_SOURCE_DIR}/algorithms.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/byte_order.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/plug_openssl.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_timing.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rhash_torrent.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/aich.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crc32.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/ed2k.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/edonr.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hex.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/md4.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/md5.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha1.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha256.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha512.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/sha3.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/ripemd-160.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/gost12.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/gost94.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/has160.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/snefru.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tiger.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tth.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/torrent.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/ustd.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/util.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/whirlpool.h")

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")

if(USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_OPENSSL)
endif()

if(OPENSSL_RUNTIME)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_DL_LIBS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE OPENSSL_RUNTIME)
endif()

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_DEPRECATE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
                      COMPILE_DEFINITIONS IN_RHASH
                      DEFINE_SYMBOL RHASH_EXPORTS
                      PREFIX ""
                      IMPORT_PREFIX ""
                      VERSION ${RHASH_VERSION}
                      SOVERSION ${RHASH_VERSION_MAJOR})

export(TARGETS ${PROJECT_NAME}
       NAMESPACE ${CMAKE_PROJECT_NAME}::
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake")

install(TARGETS ${PROJECT_NAME}
       EXPORT ${CMAKE_PROJECT_NAME}Config
       RUNTIME DESTINATION bin
       LIBRARY DESTINATION lib
       ARCHIVE DESTINATION lib)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rhash.h" "${CMAKE_CURRENT_SOURCE_DIR}/rhash_torrent.h"
        DESTINATION include)

install(EXPORT ${CMAKE_PROJECT_NAME}Config
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/cmake"
        NAMESPACE ${CMAKE_PROJECT_NAME}::)
