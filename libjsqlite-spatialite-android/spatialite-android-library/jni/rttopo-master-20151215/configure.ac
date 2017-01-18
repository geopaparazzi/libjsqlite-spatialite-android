dnl **********************************************************************
dnl *
dnl * rttopo - topology library
dnl * http://gitlab.com/rttopo/rttopo
dnl * Copyright 2008 Mark Cave-Ayland
dnl *
dnl * This is free software; you can redistribute and/or modify it under
dnl * the terms of the GNU General Public Licence. See the COPYING file.
dnl *
dnl **********************************************************************

AC_INIT()
AC_CONFIG_HEADERS([src/rttopo_config.h])
AC_CONFIG_MACRO_DIR([macros])


dnl Invoke libtool: we do this as it is the easiest way to find the PIC
dnl flags required to build liblwgeom
AC_PROG_LIBTOOL

dnl
dnl Compilers
dnl
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GREP

dnl
dnl Define PIC flags in PICFLAGS (note: this variable is set as part of libtool initialisation above)
dnl
PICFLAGS="$lt_prog_compiler_pic"
AC_SUBST([PICFLAGS])

dnl
dnl For GCC enable additional warning flags -Wall and -Wmissing-prototypes (using macro included with libtool)
dnl
WARNFLAGS=""
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wall], [_cv_wall], [-Wall], [], [WARNFLAGS="$WARNFLAGS -Wall"], []) 
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wmissing-prototypes], [_cv_misprot], [-Wmissing-prototypes], [], [WARNFLAGS="$WARNFLAGS -Wmissing-prototypes"], []) 
AC_SUBST([WARNFLAGS])

dnl
dnl For some GCC versions and platforms, floating point issues need to be
dnl ironed out. 
NUMERICFLAGS=""
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -ffloat-store], [dummy_cv_ffloat_store], [-ffloat-store], [], [NUMERICFLAGS="$NUMERICFLAGS -ffloat-store"], [])
AC_SUBST([NUMERICFLAGS])

dnl
dnl Exporting used library symbols in the module is a source of issues,
dnl see https://trac.osgeo.org/postgis/ticket/3281
dnl
EXCLUDELIBS_LDFLAGS=""
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports --exclude-libs], [_cv_exclude_libs], [[-Wl,--exclude-libs,ALL]], [], [EXCLUDELIBS_LDFLAGS="-Wl,--exclude-libs,ALL"], [])
AC_SUBST([EXCLUDELIBS_LDFLAGS])

dnl
dnl Define executable suffix for use with the loader Makefiles
dnl
EXESUFFIX="$ac_cv_exeext"
AC_SUBST([EXESUFFIX])

dnl
dnl Liblwgeom version information imported from Version.config
dnl

LIBRTGEOM_CURRENT=`cat Version.config | grep ^LIBRTGEOM_IFACE_CUR | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
LIBRTGEOM_AGE=`cat Version.config | grep ^LIBRTGEOM_IFACE_AGE | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
LIBRTGEOM_REV=`cat Version.config | grep ^LIBRTGEOM_IFACE_REV | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
AC_SUBST([LIBRTGEOM_CURRENT])
AC_SUBST([LIBRTGEOM_AGE])
AC_SUBST([LIBRTGEOM_REV])

LIBRTGEOM_VERSION=`grep ^LIBRTGEOM_VERSION Version.config | sed 's/.*=//'`
AC_DEFINE_UNQUOTED([LIBRTGEOM_VERSION], ["$LIBRTGEOM_VERSION"], [rtgeom version])
AC_SUBST([LIBRTGEOM_VERSION])


dnl
dnl Search for flex/bison to build the parser
dnl

AC_PROG_LEX
AC_PROG_YACC
AC_SUBST([LEX])
AC_SUBST([YACC])

dnl
dnl Search for OS-specific headers
dnl
AC_CHECK_HEADER([ieeefp.h], [HAVE_IEEEFP_H=1], [HAVE_IEEEFP_H=0])
AC_DEFINE_UNQUOTED([HAVE_IEEEFP_H], [$HAVE_IEEEFP_H], [ieeefp.h header])
AC_CHECK_HEADER([termios.h], [HAVE_TERMIOS_H=1], [HAVE_TERMIOS_H=0])
AC_DEFINE_UNQUOTED([HAVE_TERMIOS_H], [$HAVE_TERMIOS_H], [termios.h header])


dnl
dnl Check for platform-specific functions
dnl
AC_CHECK_FUNC(vasprintf, AC_DEFINE([HAVE_VASPRINTF]))
AC_CHECK_FUNC(asprintf, AC_DEFINE([HAVE_ASPRINTF]))
AC_FUNC_FSEEKO()

dnl 
dnl First see if we have isfinite in basic header
dnl then check to see if it's a GNU extension
dnl
AC_CHECK_DECL(isfinite, 
  [AC_DEFINE([HAVE_ISFINITE])], 
  [],
  [#include <math.h>])

dnl Clear the cache
unset ac_cv_have_decl_isfinite

AC_CHECK_DECL(isfinite,
  AC_DEFINE([HAVE_GNU_ISFINITE]),
  [],
  [[
    #define _GNU_SOURCE 
    #include <math.h>
  ]])

dnl 
dnl MingW requires use of pwd -W to give proper Windows (not MingW) paths 
dnl for in-place regression tests 
dnl 
case $host_os in
         *mingw*)
                 MINGWBUILD=1
                 ;;
         *)
                 MINGWBUILD=0
                 ;;
esac
AC_SUBST([MINGWBUILD])

dnl
dnl Allow the user to specify the location of the html/docbook.xsl stylesheet
dnl

dnl ===========================================================================
dnl Detect libxml2 if it is installed
dnl (needed to GeomFromGML and GeomFromKML functions)
dnl ===========================================================================

AC_ARG_WITH([xml2config], 
	[AS_HELP_STRING([--with-xml2config=FILE], [specify an alternative xml2-config file])], 
	[XML2CONFIG="$withval"], [XML2CONFIG=""])

if test "x$XML2CONFIG" = "x"; then
	dnl XML2CONFIG was not specified, so search within the current path
	AC_PATH_PROG([XML2CONFIG], [xml2-config])

	dnl If we couldn't find xml2-config, display a warning
	if test "x$XML2CONFIG" = "x"; then
		AC_MSG_ERROR([could not find xml2-config from libxml2 within the current path. You may need to try re-running configure with a --with-xml2config parameter.])
	fi
else
	dnl XML2CONFIG was specified; display a message to the user
	if test "x$XML2CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
	else
		if test -f $XML2CONFIG; then
			AC_MSG_RESULT([Using user-specified xml2-config file: $XML2CONFIG])
		else
			AC_MSG_ERROR([the user-specified xml2-config file $XML2CONFIG does not exist])
		fi	
	fi
fi


dnl Extract the linker and include flags 
XML2_LDFLAGS=`$XML2CONFIG --libs`
XML2_CPPFLAGS=`$XML2CONFIG --cflags`

dnl Extract the version
RTGEOM_LIBXML2_VERSION=`$XML2CONFIG --version`

dnl Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XML2_CPPFLAGS"
AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h libxml/xpath.h libxml/xpathInternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxml2])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libxml2
LIBS_SAVE="$LIBS"
LIBS="$XML2_LDFLAGS"
AC_CHECK_LIB([xml2], [xmlInitParser], [], [AC_MSG_ERROR([could not find libxml2])], [])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([RTGEOM_LIBXML2_VERSION], ["$RTGEOM_LIBXML2_VERSION"], [PostGIS libxml2 version])
AC_SUBST([RTGEOM_LIBXML2_VERSION])



dnl ===========================================================================
dnl Detect the version of GEOS installed on the system
dnl ===========================================================================

AC_ARG_WITH([geosconfig], 
	[AS_HELP_STRING([--with-geosconfig=FILE], [specify an alternative geos-config file])], 
	[GEOSCONFIG="$withval"], [GEOSCONFIG=""])

if test "x$GEOSCONFIG" = "x"; then
	dnl GEOSCONFIG was not specified, so search within the current path
	AC_PATH_PROG([GEOSCONFIG], [geos-config])

	dnl If we couldn't find geos-config, display an error
	if test "x$GEOSCONFIG" = "x"; then
		AC_MSG_ERROR([could not find geos-config within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
	fi
else
	dnl GEOSCONFIG was specified; display a message to the user
	if test "x$GEOSCONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
	else
		if test -f $GEOSCONFIG; then
			AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
		else
			AC_MSG_ERROR([the user-specified geos-config file $GEOSCONFIG does not exist])
		fi	
	fi
fi

dnl Extract the version information from geos_config
dnl Note: we extract the major & minor separately, ensure they are numeric, 
dnl and then combine to give the final version. 
dnl This is to guard against user error... 
GEOS_MAJOR_VERSION=`$GEOSCONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
GEOS_MINOR_VERSION=`$GEOSCONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
GEOS_PATCH_VERSION=`$GEOSCONFIG --version | cut -d. -f3 | sed 's/[[^0-9]]//g'`
if test "x$GEOS_PATCH_VERSION" = "x"; then 
	GEOS_PATCH_VERSION="0"; 
fi
GEOS_FULL_VERSION=`$GEOSCONFIG --version`
RTGEOM_GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"
GEOS_NUMERIC_PATCH_VERSION=`printf "%02d" $GEOS_PATCH_VERSION`
GEOS_NUMERIC_MINOR_VERSION=`printf "%02d" $GEOS_MINOR_VERSION`
GEOS_NUMERIC_VERSION="$GEOS_MAJOR_VERSION$GEOS_NUMERIC_MINOR_VERSION$GEOS_NUMERIC_PATCH_VERSION"

dnl Ensure that we are using GEOS >= 3.4.0
AC_MSG_RESULT([checking GEOS version... $GEOS_FULL_VERSION])
if test ! "$GEOS_NUMERIC_VERSION" -ge 30400; then
	AC_MSG_ERROR([librttopo requires GEOS >= 3.4.0])
fi

dnl Extract the linker and include flags 
GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
GEOS_CPPFLAGS=-I`$GEOSCONFIG --includes`
AC_SUBST([GEOS_LDFLAGS])
AC_SUBST([GEOS_CPPFLAGS])

dnl Ensure that we can parse geos_c.h
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$GEOS_CPPFLAGS"
AC_CHECK_HEADER([geos_c.h], [], [AC_MSG_ERROR([could not find geos_c.h - you may need to specify the directory of a geos-config file using --with-geosconfig])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libgeos_c
LIBS_SAVE="$LIBS"
LIBS="$GEOS_LDFLAGS"
AC_CHECK_LIB([geos_c], [initGEOS],
	[],
	[AC_MSG_ERROR([could not find libgeos_c - you may need to specify the directory of a geos-config file using --with-geosconfig])],
	[])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([RTGEOM_GEOS_VERSION], [$RTGEOM_GEOS_VERSION], [GEOS library version])	
AC_SUBST([RTGEOM_GEOS_VERSION])
AC_SUBST([GEOS_NUMERIC_VERSION])

dnl ===========================================================================
dnl Detect if json-c installed
dnl ===========================================================================

CHECK_JSON=yes
HAVE_JSON=no
HAVE_JSON_C=no

AC_ARG_WITH([json],
	[AS_HELP_STRING([--without-json], [build without json-c support])],
	[CHECK_JSON="$withval"], [])

if test "$CHECK_JSON" != "no"; then dnl {

AC_ARG_WITH([jsondir],
	[AS_HELP_STRING([--with-jsondir=PATH], [specify the json-c installation directory])],
	[JSONDIR="$withval"], [JSONDIR=])

if test ! "x$JSONDIR" = "x"; then
	dnl Make sure that the directory exists
	if test "x$JSONDIR" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-jsondir, e.g. --with-jsondir=/path/to])
	else
		AC_MSG_RESULT([Using user-specified json-c directory: $JSONDIR])

		dnl Add the include directory to JSON_CPPFLAGS
		JSON_CPPFLAGS="-I$JSONDIR/include"
		JSON_LDFLAGS="-L$JSONDIR/lib"
	fi
fi

dnl Check that we can find the json/json.h header file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$JSON_CPPFLAGS"
AC_CHECK_HEADER([json/json.h], [HAVE_JSON=yes], [
  AC_CHECK_HEADER([json-c/json.h], [HAVE_JSON=yes; HAVE_JSON_C=yes], [])
])	
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libjson
LIBS_SAVE="$LIBS"
LIBS="$JSON_LDFLAGS"
AC_CHECK_LIB([json-c], [json_object_get], [HAVE_JSON=yes; JSON_LDFLAGS="${JSON_LDFLAGS} -ljson-c"], [
  AC_CHECK_LIB([json], [json_object_get], [HAVE_JSON=yes; JSON_LDFLAGS="${JSON_LDFLAGS} -ljson"], [], [])
], [])
LIBS="$LIBS_SAVE"

if test "$HAVE_JSON" = "yes"; then
	AC_DEFINE([HAVE_LIBJSON], 1, [Define to 1 if libjson is present])
fi
if test "$HAVE_JSON_C" = "yes"; then
	AC_DEFINE([HAVE_LIBJSON_C], 1, [Define to 1 if libjson resides in a json-c subdir])
fi

AC_SUBST([JSON_CPPFLAGS])
AC_SUBST([JSON_LDFLAGS])
AC_SUBST([HAVE_JSON])

fi dnl }




dnl ===========================================================================
dnl Allow the user to enable debugging with --enable-debug
dnl
dnl Currently we default to debug level 4. See DEBUG for more information. 
dnl

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug], [Enable verbose debugging messages]), 
	[RTGEOM_DEBUG_LEVEL=4], [RTGEOM_DEBUG_LEVEL=0]) 

AC_DEFINE_UNQUOTED([RTGEOM_DEBUG_LEVEL], [$RTGEOM_DEBUG_LEVEL], [library debug level (0=disabled)])

dnl ===========================================================================
dnl Allow the user to enable GEOS profiling with --enable-profile
dnl

AC_ARG_ENABLE([profile], AC_HELP_STRING([--enable-profile], [Enable GEOS profiling messages]), 
	[RTGEOM_PROFILE=1], [RTGEOM_PROFILE=0]) 

AC_DEFINE_UNQUOTED([RTGEOM_PROFILE], [$RTGEOM_PROFILE], [Enable GEOS profiling (0=disabled)])

dnl ===========================================================================
dnl Define version macros
dnl


CPPFLAGS="$PGSQL_CPPFLAGS $GEOS_CPPFLAGS $XML2_CPPFLAGS $JSON_CPPFLAGS $PCRE_CPPFLAGS $CPPFLAGS"
dnl AC_MSG_RESULT([CPPFLAGS: $CPPFLAGS])

SHLIB_LINK="$PGSQL_LDFLAGS $GEOS_LDFLAGS -lgeos_c -lproj $JSON_LDFLAGS $XML2_LDFLAGS $PCRE_LDFLAGS $EXCLUDELIBS_LDFLAGS"
AC_SUBST([SHLIB_LINK])
dnl AC_MSG_RESULT([SHLIB_LINK: $SHLIB_LINK])

dnl ====================================
dnl interrupt tests
dnl ====================================
INTERRUPTTESTS="no"
AC_ARG_WITH([interrupt-tests],
    [AS_HELP_STRING([--without-interrupt-tests],
                    [Disable the interrupt tests (for CI servers)])],
    [], [])

if test "x$with_interrupt_tests" != "xno"; then
    INTERRUPTTESTS="yes"
fi

AC_SUBST([INTERRUPTTESTS])

dnl ===========================================================================
dnl SRID stuff
dnl ===========================================================================

SRID_MAX=999999
SRID_USR_MAX=998999
AC_SUBST([SRID_MAX])
AC_SUBST([SRID_USR_MAX])

AC_OUTPUT([
   src/Makefile 
   src/librtgeom.h
   ])
    
dnl ===========================================================================
dnl Display the configuration status information
dnl ===========================================================================

AC_MSG_RESULT()
AC_MSG_RESULT([  rttopo is now configured for ${host}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Compiler Info ------------- ])
AC_MSG_RESULT([  C compiler:           ${CC} ${CFLAGS}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Dependencies -------------- ])
AC_MSG_RESULT([  GEOS config:          ${GEOSCONFIG}])
AC_MSG_RESULT([  GEOS version:         ${GEOS_FULL_VERSION}])
AC_MSG_RESULT([  Libxml2 config:       ${XML2CONFIG}])
AC_MSG_RESULT([  Libxml2 version:      ${RTGEOM_LIBXML2_VERSION}])
AC_MSG_RESULT([  JSON-C support:       ${HAVE_JSON}])
AC_MSG_RESULT([  PCRE support:         ${HAVE_PCRE}])
AC_MSG_RESULT([  Debug level:          ${RTGEOM_DEBUG_LEVEL}])
AC_MSG_RESULT([  Perl:                 ${PERL}])
AC_MSG_RESULT()

if test "$GEOS_NUMERIC_VERSION" -lt 30500; then
AC_MSG_WARN([ --------- GEOS VERSION WARNING ------------ ])
AC_MSG_WARN([  You are building against GEOS ${GEOS_FULL_VERSION} ])
AC_MSG_WARN([  To take advantage of all the features of ])
AC_MSG_WARN([  PostGIS we recommend you build using ])
AC_MSG_WARN([  GEOS 3.5.0 or higher. You can download ])
AC_MSG_WARN([  the latest versions from ])
AC_MSG_WARN([  http://trac.osgeo.org/geos ])
AC_MSG_WARN()
fi

