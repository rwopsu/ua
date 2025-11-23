AC_INIT([ua], [1.0.1], [istvan.hernadvolgyi@gmail.com])
AC_CONFIG_SRCDIR([src/ua.cc])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_CHECK_LIB([crypto], [EVP_DigestInit], [], [AC_MSG_ERROR([libcrypto is required])])
AC_CHECK_LIB([ssl], [SSL_new], [], [AC_MSG_ERROR([libssl is required])])

AC_OUTPUT([Makefile])
