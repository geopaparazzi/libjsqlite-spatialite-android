
AM_CFLAGS = @LIBPNG_CFLAGS@ @LIBWEBP_CFLAGS@ @LIBLZMA_CFLAGS@ \
	@LIBLZ4_CFLAGS@ @LIBZSTD_CFLAGS@ @LIBOPENJP2_CFLAGS@ \
	@LIBCURL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBFONTCONFIG_CFLAGS@ \
	@LIBFREETYPE2_CFLAGS@ @LIBPIXMAN_CFLAGS@ @LIBCAIRO_CFLAGS@ \
	@LIBSPATIALITE_CFLAGS@

AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/headers

noinst_HEADERS = md5.h
lib_LTLIBRARIES = librasterlite2.la mod_rasterlite2.la

librasterlite2_la_SOURCES = rasterlite2.c rl2raw.c rl2codec.c \
	rl2jpeg.c rl2png.c rl2gif.c rl2webp.c rl2tiff.c rl2wms.c \
	rl2ascii.c rl2paint.c rl2dbms.c rl2import.c rl2pyramid.c \
	rl2sql.c rl2sqlaux.c rl2auxrender.c rl2svg.c rl2svgxml.c \
	rl2svgaux.c rl2symbolizer.c rl2symbaux.c rl2rastersym.c \
	rl2version.c rl2md5.c md5.c rl2charls.c rl2openjpeg.c \
	rl2auxgeom.c rl2auxfont.c rl2symclone.c \
	rl2_internal_data.c rl2draping.c

librasterlite2_la_LIBADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@

if MINGW
librasterlite2_la_LDFLAGS = -avoid-version -no-undefined
librasterlite2_la_LIBADD += -lm
else
librasterlite2_la_LDFLAGS = -version-info 1:0:0
librasterlite2_la_LIBADD += -lpthread -lm
endif

mod_rasterlite2_la_SOURCES = rasterlite2.c rl2raw.c rl2codec.c \
	rl2jpeg.c rl2png.c rl2gif.c rl2webp.c rl2tiff.c rl2wms.c \
	rl2ascii.c rl2paint.c rl2dbms.c rl2import.c rl2pyramid.c \
	rl2sql.c rl2sqlaux.c rl2auxrender.c rl2svg.c rl2svgxml.c \
	rl2svgaux.c rl2symbolizer.c rl2symbaux.c rl2rastersym.c \
	rl2version.c rl2md5.c md5.c rl2charls.c rl2openjpeg.c \
	rl2auxgeom.c rl2auxfont.c rl2symclone.c \
	rl2_internal_data.c rl2draping.c

mod_rasterlite2_la_LIBADD = @LIBPNG_LIBS@ @LIBWEBP_LIBS@ @LIBLZMA_LIBS@ \
	@LIBLZ4_LIBS@ @LIBZSTD_LIBS@ @LIBOPENJP2_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ @LIBFONTCONFIG_LIBS@ \
	@LIBFREETYPE2_LIBS@ @LIBPIXMAN_LIBS@ @LIBCAIRO_LIBS@ \
	@LIBSPATIALITE_LIBS@

mod_rasterlite2_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
mod_rasterlite2_la_CPPFLAGS += -I$(top_srcdir)/headers -I.
mod_rasterlite2_la_CPPFLAGS += -DLOADABLE_EXTENSION
mod_rasterlite2_la_LIBTOOLFLAGS = --tag=disable-static

if MINGW
mod_rasterlite2_la_LDFLAGS = -module -avoid-version -no-undefined
mod_rasterlite2_la_LIBADD += -lm
else
mod_rasterlite2_la_LDFLAGS = -module -version-info 1:0:0
mod_rasterlite2_la_LIBADD += -lpthread -lm
endif

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
