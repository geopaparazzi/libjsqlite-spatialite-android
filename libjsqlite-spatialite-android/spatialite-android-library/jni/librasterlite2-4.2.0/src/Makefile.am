
AM_CFLAGS = @LIBPNG_CFLAGS@ @LIBWEBP_CFLAGS@ @LIBLZMA_CFLAGS@ \
	@LIBSPATIALITE_CFLAGS@ @LIBCAIRO_CFLAGS@ @LIBCURL_CFLAGS@ \
	@LIBXML2_CFLAGS@ 

AM_CPPFLAGS = @CFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/headers

lib_LTLIBRARIES = librasterlite2.la mod_rasterlite2.la

librasterlite2_la_SOURCES = rasterlite2.c rl2raw.c rl2codec.c \
	rl2jpeg.c rl2png.c rl2gif.c rl2webp.c rl2tiff.c rl2wms.c \
	rl2ascii.c rl2paint.c rl2dbms.c rl2import.c rl2sql.c \
	rl2sqlaux.c rl2svg.c rl2svgxml.c rl2svgaux.c rl2symbolizer.c \
	rl2rastersym.c rl2version.c

if MINGW
librasterlite2_la_LDFLAGS = -avoid-version -no-undefined
else
librasterlite2_la_LDFLAGS = -version-info 1:0:0
endif

librasterlite2_la_LIBADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ \
	@LIBLZMA_LIBS@ @LIBSPATIALITE_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@

mod_rasterlite2_la_SOURCES = rasterlite2.c rl2raw.c rl2codec.c \
	rl2jpeg.c rl2png.c rl2gif.c rl2webp.c rl2tiff.c rl2wms.c \
	rl2ascii.c rl2paint.c rl2dbms.c rl2import.c rl2sql.c \
	rl2sqlaux.c rl2svg.c rl2svgxml.c rl2svgaux.c rl2symbolizer.c \
	rl2rastersym.c rl2version.c

mod_rasterlite2_la_LIBADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ \
	@LIBLZMA_LIBS@ @LIBSPATIALITE_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@

mod_rasterlite2_la_CPPFLAGS = @CFLAGS@
mod_rasterlite2_la_CPPFLAGS += -I$(top_srcdir)/headers -I.
mod_rasterlite2_la_CPPFLAGS += -DLOADABLE_EXTENSION
mod_rasterlite2_la_LIBTOOLFLAGS = --tag=disable-static

if MINGW
mod_rasterlite2_la_LDFLAGS = -module -avoid-version -no-undefined
else
mod_rasterlite2_la_LDFLAGS = -module -version-info 1:0:0
endif

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
