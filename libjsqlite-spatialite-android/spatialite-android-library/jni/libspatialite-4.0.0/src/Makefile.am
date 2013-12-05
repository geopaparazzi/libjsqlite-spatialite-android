
SUBDIRS = headers \
	gaiaaux \
	gaiaexif \
	gaiageo \
	spatialite \
	shapefiles \
	srsinit \
	virtualtext

INCLUDES = @CFLAGS@
INCLUDES += -I$(top_srcdir)/src/headers
INCLUDES += -I$(top_srcdir)

lib_LTLIBRARIES = libspatialite.la

libspatialite_la_SOURCES = versioninfo/version.c

libspatialite_la_LIBADD = ./gaiaaux/libgaiaaux.la \
	./gaiaexif/libgaiaexif.la \
	./gaiageo/libgaiageo.la \
	./spatialite/libsplite.la \
	./shapefiles/libshapefiles.la \
	./srsinit/libsrsinit.la \
	./virtualtext/libvirtualtext.la


if MINGW
libspatialite_la_LDFLAGS = -version-info 4:0:0 -no-undefined
else 
if ANDROID
libspatialite_la_LDFLAGS = -version-info 5:1:0
libspatialite_la_LIBADD += -ldl
else
libspatialite_la_LDFLAGS = -version-info 5:1:0
libspatialite_la_LIBADD += -lpthread -ldl
endif
endif

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
