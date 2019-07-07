noinst_PROGRAMS = sample1

AM_CFLAGS = -I@srcdir@/../headers
AM_LDFLAGS = -L../src @LIBSPATIALITE_LIBS@ -lrasterlite2 -ljpeg -lpng -lm $(GCOV_FLAGS)

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = examples.doxy
