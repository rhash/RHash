dnl
dnl config.m4 for RHash extension
dnl

PHP_ARG_WITH(rhash, for RHash support,
[  --with-rhash[=DIR]        Include RHash support.])
dnl                          DIR is the RHash install prefix [BUNDLED]], yes, no)

if test "$PHP_RHASH" != "no"; then
  if test -r $PHP_RHASH/include/rhash/rhash.h; then
    RHASH_DIR=$PHP_RHASH
  else
    AC_MSG_CHECKING(for RHash in default path)
    for i in /usr/local /usr; do
      if test -r $i/include/rhash/rhash.h; then
        RHASH_DIR=$i
        AC_MSG_RESULT(found in $i)
        break
      fi
    done
  fi

  if test -z "$RHASH_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(Please reinstall the librhash -
    rhash.h should be in <rhash-dir>/include/rhash/)
  fi

  dnl PHP_RHASH_CFLAGS="-I@ext_builddir@/lib"

  AC_DEFINE(HAVE_RHASH, 1, [Whether you have RHash])
  PHP_ADD_INCLUDE($RHASH_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH(rhash, $RHASH_DIR/lib/librhash.so, RHASH_SHARED_LIBADD)
  PHP_NEW_EXTENSION(rhash, php_rhash.c, $ext_shared)
  PHP_SUBST(RHASH_SHARED_LIBADD)
  dnl  PHP_NEW_EXTENSION(rhash, php_rhash.c, $PHP_RHASH_CFLAGS)
fi
