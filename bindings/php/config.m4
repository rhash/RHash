dnl
dnl config.m4 for RHash extension
dnl

PHP_ARG_WITH(rhash, for RHash support,
[  --with-rhash[=DIR]        Include RHash support.])

if test "$PHP_RHASH" != "no"; then
  if test -r $PHP_RHASH/include/rhash.h; then
    RHASH_INCLUDE_DIR=$PHP_RHASH/include
    RHASH_LIB_DIR=$PHP_RHASH/lib
  elif test -r $PHP_RHASH/rhash.h; then
    RHASH_INCLUDE_DIR=$PHP_RHASH
    RHASH_LIB_DIR=$PHP_RHASH
  else
    AC_MSG_CHECKING(for RHash in default path)
    for i in /usr/local /usr; do
      if test -r $i/include/rhash.h; then
        RHASH_INCLUDE_DIR=$i/include
        RHASH_LIB_DIR=$i/lib
        AC_MSG_RESULT(found at $i)
        break
      fi
    done
  fi

  if test -z "$RHASH_INCLUDE_DIR" -a -r ../../librhash/rhash.h; then
    RHASH_INCLUDE_DIR=$(pwd)/../../librhash
    RHASH_LIB_DIR=$RHASH_INCLUDE_DIR
    AC_MSG_RESULT(found at $RHASH_INCLUDE_DIR)
  fi

  if test -z "$RHASH_INCLUDE_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(Please reinstall the librhash -
    rhash.h should be in <rhash-dir>/include/)
  fi

  AC_DEFINE(HAVE_RHASH, 1, [Whether you have RHash])
  PHP_ADD_INCLUDE($RHASH_INCLUDE_DIR)
  PHP_ADD_LIBRARY_WITH_PATH(rhash, $RHASH_LIB_DIR, RHASH_SHARED_LIBADD)
  PHP_NEW_EXTENSION(rhash, php_rhash.c, $ext_shared)
  PHP_SUBST(RHASH_SHARED_LIBADD)
fi
