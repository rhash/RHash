# autoconf/automake base variables
AC_PREREQ([2.69])
AC_INIT([RHash],[1.3.1],[rhash.admin@gmail.com])
AC_CONFIG_SRCDIR([rhash_main.c])
AC_CONFIG_HEADERS([version.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([support])

AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

# checks for compilers and other tools
AC_PROG_CC
AM_PROG_AR
AC_EXEEXT
LT_INIT

AX_CHECK_OPENSSL([AC_DEFINE([OPENSSL_RUNTIME], [1], [OpenSSL support])])

# TODO: if gettext is found, add -DUSE_GETTEXT to cflags

# output files
AC_CONFIG_FILES([Makefile])
#AC_CONFIG_FILES([bindings/Makefile])
AC_CONFIG_FILES([dist/Makefile])
AC_CONFIG_FILES([librhash/Makefile])
#AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT
