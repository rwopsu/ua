AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CXXFLAGS = -O3 -I src -Wall
AM_CFLAGS = -O3 -I src -Wall -msse2 -msse4.1 -mavx2 -mavx512f -mavx512vl
AM_LDFLAGS = 

bin_PROGRAMS = ua kua

ua_SOURCES = \
  src/ua.cc src/filei.cc src/filei.h \
  src/blake3.c src/blake3_dispatch.c src/blake3_portable.c \
  src/blake3_sse2.c src/blake3_sse41.c src/blake3_avx2.c src/blake3_avx512.c \
  src/xxhash.c

kua_SOURCES = \
  src/kua.cc src/filei.cc src/filei.h \
  src/blake3.c src/blake3_dispatch.c src/blake3_portable.c \
  src/blake3_sse2.c src/blake3_sse41.c src/blake3_avx2.c src/blake3_avx512.c \
  src/xxhash.c

man_MANS = man/man1/ua.1 man/man1/kua.1

EXTRA_DIST = $(man_MANS)

DISTCLEANFILES = \
  ua kua \
  config.log config.status config.h config.h.in \
  libtool ltmain.sh aclocal.m4 Makefile.in configure \
  depcomp install-sh missing compile

distclean-local:
	$(RM) -r build autom4te.cache
