#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(librasterlite2, 1.1.0-beta1, a.furieri@lqt.it)
AC_LANG(C)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

# enabling huge-file support (64 bit file pointers)
AH_TEMPLATE([_LARGE_FILE],
            [Must be defined in order to enable huge-file support.])
AC_DEFINE(_LARGE_FILE)
AH_TEMPLATE([_FILE_OFFSET_BITS],
            [Must be =64 in order to enable huge-file support.])
AC_DEFINE(_FILE_OFFSET_BITS, 64)
AH_TEMPLATE([_LARGEFILE_SOURCE],
            [Must be defined in order to enable huge-file support.])
AC_DEFINE(_LARGEFILE_SOURCE)

# disablibg debug support
AH_TEMPLATE([NDEBUG],
            [Must be defined in order to disable debug mode.])
AC_DEFINE(NDEBUG)
AH_TEMPLATE([TARGET_CPU],
            [Should contain a text-string describing the intended target CPU])
AH_TEMPLATE([CHARLS_2],
			[testing for CharLS 2.0])
            
# config depending options
AH_TEMPLATE([OMIT_LZMA],
            [Should be defined in order to disable LZMA support.])
AH_TEMPLATE([OMIT_LZ4],
            [Should be defined in order to disable LZ4 support.])
AH_TEMPLATE([OMIT_ZSTD],
            [Should be defined in order to disable ZSTD support.])
AH_TEMPLATE([OMIT_WEBP],
            [Should be defined in order to disable WebP support.])
AH_TEMPLATE([OMIT_OPENJPEG],
            [Should be defined in order to disable OpenJpeg support.])
AH_TEMPLATE([OMIT_CHARLS],
            [Should be defined in order to disable CharLS support.])
AH_TEMPLATE([PROJ_NEW],
            [Should be defined in order to enable PROJ.6 support.])

# Checks for header files.
AC_CHECK_HEADERS(stdlib.h,, [AC_MSG_ERROR([cannot find stdlib.h, bailing out])])
AC_CHECK_HEADERS(stdio.h,, [AC_MSG_ERROR([cannot find stdio.h, bailing out])])
AC_CHECK_HEADERS(string.h,, [AC_MSG_ERROR([cannot find string.h, bailing out])])
AC_CHECK_HEADERS(memory.h,, [AC_MSG_ERROR([cannot find memory.h, bailing out])])
AC_CHECK_HEADERS(math.h,,[AC_MSG_ERROR([cannot find math.h, bailing out])])
AC_CHECK_HEADERS(float.h,,[AC_MSG_ERROR([cannot find float.h, bailing out])])
AC_CHECK_HEADERS(jpeglib.h,, [AC_MSG_ERROR([cannot find jpeglib.h, bailing out])])
AC_CHECK_HEADERS(jerror.h,, [AC_MSG_ERROR([cannot find jerror.h, bailing out])])
AC_CHECK_HEADERS(png.h,, [AC_MSG_ERROR([cannot find png.h, bailing out])])
AC_CHECK_HEADERS(sqlite3.h,, [AC_MSG_ERROR([cannot find sqlite3.h, bailing out])])
AC_CHECK_HEADERS(sqlite3ext.h,, [AC_MSG_ERROR([cannot find sqlite3ext.h, bailing out])])
AC_CHECK_HEADERS(zlib.h,, [AC_MSG_ERROR([cannot find zlib.h, bailing out])])
AC_CHECK_HEADERS(gif_lib.h,, [AC_MSG_ERROR([cannot find gif_lib.h, bailing out])])


#
# testing GeoTiff headers
# on some systems they are located on -/include
# on other systems they are on -/include/geotiff
# or even on -/include/libgeotiff
#
# this patch was kindly contributed by Brad Hards
# 2011-09-02
#
AC_CHECK_HEADERS(geotiff/geotiff.h)
AC_CHECK_HEADERS(libgeotiff/geotiff.h)
AC_CHECK_HEADERS(geotiff.h)
if test x"$ac_cv_header_geotiff_geotiff_h" != x"yes" &&
    test x"$ac_cv_header_libgeotiff_geotiff_h" != x"yes" &&
    test x"$ac_cv_header_geotiff_h" != x"yes";
then
    AC_MSG_ERROR(['libgeotiff' is required but the header (geotiff.h) doesn't seem to be installed on this system])
fi 


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([sqrt strcasecmp strerror strncasecmp strstr strerror])

# gcov support
AC_ARG_ENABLE(gcov, AC_HELP_STRING([--enable-gcov],[turn on code coverage analysis tools]))
if test "x$enable_gcov" = "xyes"; then
    dnl Check for -fprofile-arcs and -ftest-coverage option
    CFLAGS=$CFLAGS" -fprofile-arcs -ftest-coverage -g"
    GCOV_FLAGS="-lgcov"
fi
 
AC_CONFIG_FILES([Makefile \
		headers/Makefile \
		src/Makefile \
		test/Makefile \
		test/sql_stmt_tests/Makefile \
		test/sql_stmt_security_tests/Makefile \
		tools/Makefile \
		examples/Makefile \
		Doxyfile \
		rasterlite2.pc])

# exporting the TARGET_CPU string
rl2_cpu=`$CC -dumpmachine`
AC_DEFINE_UNQUOTED([TARGET_CPU], ["$rl2_cpu"])

# Checks for installed libraries
AC_CHECK_LIB(sqlite3,sqlite3_prepare_v2,,AC_MSG_ERROR(['libsqlite3' is required but it doesn't seem to be installed on this system.]),-lm)
AC_CHECK_LIB(z,inflateInit_,,AC_MSG_ERROR(['libz' is required but it doesn't seems to be installed on this system.]),-lm)
AC_CHECK_LIB(jpeg,jpeg_start_compress,,AC_MSG_ERROR(['libjpeg' is required but it doesn't seems to be installed on this system.]),-lm)
AC_CHECK_LIB(gif,DGifSlurp,,AC_MSG_ERROR(['libgif' is required but it doesn't seems to be installed on this system.]),-lm)
AC_CHECK_LIB(tiff,TIFFClientOpen,,AC_MSG_ERROR(['libtiff' is required but it doesn't seems to be installed on this system.]),-lm)
AC_CHECK_LIB(geotiff,GTIFSetFromProj4,,AC_MSG_ERROR(['libgeotiff' [>= v.1.2.5] is required but it doesn't seems to be installed on this system.]),-lm)

AC_CHECK_HEADERS(proj.h, [proj_new_incl=1], 
   [AC_CHECK_HEADERS(proj_api.h, [proj_new_incl=0], [AC_MSG_ERROR([cannot find proj_api.h, bailing out])])])
AC_SEARCH_LIBS(proj_normalize_for_visualization, proj, [proj_new_lib=1],
   [AC_SEARCH_LIBS(pj_init_plus, proj, [proj_new_lib=0], [AC_MSG_ERROR(['libproj' is required but it doesn't seem to be installed on this system.])], [-lm -lpthread -lsqlite3])])
if test $proj_new_incl -eq 1 && test $proj_new_lib -eq 1; then
   AC_DEFINE(PROJ_NEW)
fi

#-----------------------------------------------------------------------
#   --enable-openjpeg
#
AC_ARG_ENABLE(openjpeg, [AS_HELP_STRING(
  [--enable-openjpeg], [enables OpenJpeg inclusion [default=yes]])],
  [], [enable_openjpeg=yes])
if test x"$enable_openjpeg" != "xno"; then
  PKG_CHECK_MODULES([LIBOPENJP2], [libopenjp2], , AC_MSG_ERROR(['libopenjp2' (v2.1 or later) is required but it doesn't seem to be installed on this system.]))
  AC_SUBST(LIBOPENJP2_CFLAGS)
  AC_SUBST(LIBOPENJP2_LIBS)
else
  AC_DEFINE(OMIT_OPENJPEG)
fi

#-----------------------------------------------------------------------
#   --enable-webp
#
AC_ARG_ENABLE(webp, [AS_HELP_STRING(
  [--enable-webp], [enables WebP inclusion [default=yes]])],
  [], [enable_webp=yes])
if test x"$enable_webp" != "xno"; then
    PKG_CHECK_MODULES([LIBWEBP], [libwebp], , AC_MSG_ERROR(['libwebp' is required but it doesn't seems to be installed on this system.]))
    AC_SUBST(LIBWEBP_CFLAGS)
    AC_SUBST(LIBWEBP_LIBS)
else
  AC_DEFINE(OMIT_WEBP)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-lzma
#
AC_ARG_ENABLE(lzma, [AS_HELP_STRING(
  [--enable-lzma], [enables LZMA inclusion [default=yes]])],
  [], [enable_lzma=yes])
if test x"$enable_lzma" != "xno"; then
    PKG_CHECK_MODULES([LIBLZMA], [liblzma], , AC_MSG_ERROR(['liblzma' is required but it doesn't seems to be installed on this system.]))
    AC_SUBST(LIBLZMA_CFLAGS)
    AC_SUBST(LIBLZMA_LIBS)
else
  AC_DEFINE(OMIT_LZMA)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-lz4
#
AC_ARG_ENABLE(lz4, [AS_HELP_STRING(
  [--enable-lz4], [enables LZ4 inclusion [default=yes]])],
  [], [enable_lz4=yes])
if test x"$enable_lz4" != "xno"; then
    PKG_CHECK_MODULES([LIBLZ4], [liblz4], , AC_MSG_ERROR(['liblz4' is required but it doesn't seems to be installed on this system.]))
    AC_SUBST(LIBLZ4_CFLAGS)
    AC_SUBST(LIBLZ4_LIBS)
else
  AC_DEFINE(OMIT_LZ4)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-zstd
#
AC_ARG_ENABLE(zstd, [AS_HELP_STRING(
  [--enable-zstd], [enables ZSTD inclusion [default=yes]])],
  [], [enable_zstd=yes])
if test x"$enable_zstd" != "xno"; then
    PKG_CHECK_MODULES([LIBZSTD], [libzstd], , AC_MSG_ERROR(['libzstd' is required but it doesn't seems to be installed on this system.]))
    AC_SUBST(LIBZSTD_CFLAGS)
    AC_SUBST(LIBZSTD_LIBS)
else
  AC_DEFINE(OMIT_ZSTD)
fi
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
#   --enable-charls
#
AC_ARG_ENABLE(charls, [AS_HELP_STRING(
  [--enable-charls], [enables CharLS inclusion [default=yes]])],
  [], [enable_charls=yes])
  if test x"$enable_charls" != "xno"; then
  #
  # testing CharLS headers
  # they could be either
  # -/include/CharLS/interface.h (version 1.0)
  # or
  # -/include/CharLS/charls.h (version 2.0)
  #
  AC_CHECK_HEADERS(CharLS/interface.h)
  AC_CHECK_HEADERS(CharLS/charls.h)
  if test x"$ac_cv_header_CharLS_interface_h" != x"yes" &&
    test x"$ac_cv_header_CharLS_charls_h" != x"yes";
  then
	AC_MSG_ERROR(['CharLS' is required but no header file (interface.h or charls.h) seems to be installed on this system])
  fi
  AC_CHECK_LIB(CharLS,JpegLsEncode,,AC_MSG_ERROR(['libCharLS' is required but it doesn't seems to be installed on this system.]),-lm)
  # testing for CharLS 1.0 or 2.0
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#ifdef HAVE_CHARLS_CHARLS_H
									#include <CharLS/charls.h>
									#else
									#include <Charls/interface.h>
									#endif]],
								[[void *d; opj_stream_t *s; opj_stream_set_user_data(s, &d, NULL);]])],
					[
						AC_MSG_RESULT([yes])
						AC_DEFINE(CHARLS_2)
					],
					[AC_MSG_RESULT([no])]
				)  
else
  AC_DEFINE(OMIT_CHARLS)
fi
#-----------------------------------------------------------------------

PKG_CHECK_MODULES([LIBSPATIALITE], [spatialite], , AC_MSG_ERROR(['libspatialite' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(LIBSPATIALITE_CFLAGS)
AC_SUBST(LIBSPATIALITE_LIBS)

PKG_CHECK_MODULES([LIBPNG], [libpng], , AC_MSG_ERROR(['libpng' is required but it doesn't seems to be installed on this system.]))
AC_SUBST(LIBPNG_CFLAGS)
AC_SUBST(LIBPNG_LIBS)

PKG_CHECK_MODULES([LIBCURL], [libcurl], , AC_MSG_ERROR(['libcurl' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(LIBCURL_CFLAGS)
AC_SUBST(LIBCURL_LIBS)

PKG_CHECK_MODULES([LIBXML2], [libxml-2.0], , AC_MSG_ERROR(['libxml2' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

PKG_CHECK_MODULES([LIBFREETYPE2], [freetype2], , AC_MSG_ERROR(['freetype2' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(LIBFREETYPE2_CFLAGS)
AC_SUBST(LIBFREETYPE2_LIBS)

PKG_CHECK_MODULES([LIBFONTCONFIG], [fontconfig], , AC_MSG_ERROR(['freetype2' is required but it doesn't seem to be installed on this system.]))
AC_SUBST(LIBFONTCONFIG_CFLAGS)
AC_SUBST(LIBFONTCONFIG_LIBS)

PKG_CHECK_MODULES([LIBPIXMAN], [pixman-1], , AC_MSG_ERROR(['libpixman' is required but it doesn't seems to be installed on this system.]))
AC_SUBST(LIBPIXMAN_CFLAGS)
AC_SUBST(LIBPIXMAN_LIBS)

PKG_CHECK_MODULES([LIBCAIRO], [cairo], , AC_MSG_ERROR(['libcairo' is required but it doesn't seems to be installed on this system.]))
AC_SUBST(LIBCAIRO_CFLAGS)
AC_SUBST(LIBCAIRO_LIBS)

# Checking for MinGW
AM_CONDITIONAL([MINGW], [test "$target_alias" = "mingw32"])

AC_OUTPUT
