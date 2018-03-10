# -------------------
# Android_R4.3.0.mk
# -------------------
# For RasterLite2 with Spatialite
# - jsqlite-R20120209.mk
# 'HAVE_RASTERLITE2=1'
# -------------------
# [from 'jni/' directory]
# ndk-build clean
# ndk-build
# OR
# ndk-build clean ; ndk-build
# -------------------
# Order of running:
# -------------------
#  Rasterlite2 with Spatialite
# As of 2018-03-09
# -------------------
# - javasqlite[R20120209] code corrections for 64-bits
# - sqlite3[3220000]
# - rasterlite2[4.5.0-20180309]
# - charls[1.0]
# - openjpeg[2.0.0 ; todo: 2.3.0]
# - libwebp[0.4.0 ; todo: 0.6.1 ; see: pixman]
# - spatialite[4.5.0-20180309]
# - cairo[1.14.12]
# - curl[7.36.0 ; todo: 7.58.0]
# - libgeotiff[1.4.2]
# - giflib[5.1.4]
# - geos[3.6.2]
# - json-c[0.13.1]
# - librttopo[20180125]
# - libxml2[2.9.8]
# - pixman[0.32.6 - with processor specific makefile ]
# - fontconfig[2.11.1 ; 2.12.93 : fatal error]
# - libtiff[4.0.9]
# - proj4[3.9.3]
# - libiconv[1.15]
# - cpufeatures
# - freetype[2.9]
# - expat[2.2.5]
# - xz[5.2.1 (5.2.3 fails)]
# - jpeg[6b]
# - libpng[1.6.34]
# -------------------
LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SQLITE_PATH := sqlite-amalgamation-3220000
RASTERLITE2_PATH := librasterlite2-4.5.0
CHARLS_PATH := charls-1.0
OPENJPEG_PATH := openjpeg-2.0.0
WEBP_PATH := libwebp-0.4.0
SPATIALITE_PATH := libspatialite-4.5.0
CAIRO_PATH := cairo-1.14.12/src
CURL_PATH := curl-7.36.0
GEOTIFF_PATH := libgeotiff-1.4.2
GIF_PATH := giflib-5.1.4/lib
GEOS_PATH := geos-3.6.2
JSONC_PATH := json-c-0.13.1
RTTOPO_PATH := librttopo-20180125
XML2_PATH := libxml2-2.9.8
PIXMAN_PATH := pixman-0.34.0
FONTCONFIG_PATH := fontconfig-2.11.1
TIFF_PATH := tiff-4.0.9/libtiff
PROJ4_PATH := proj-4.9.3
ICONV_PATH := libiconv-1.15
FREETYPE_PATH := freetype-2.9
EXPAT_PATH := expat-2.2.5
LZMA_PATH := xz-5.2.1
JPEG_PATH := jpeg-6b
PNG_PATH := libpng-1.6.34
# -------------------
include $(LOCAL_PATH)/jsqlite-R20120209.mk
include $(LOCAL_PATH)/sqlite-3220000.mk
include $(LOCAL_PATH)/rasterlite2-4.5.0.mk
include $(LOCAL_PATH)/charls-1.0.mk
include $(LOCAL_PATH)/openjpeg-2.0.0.mk
include $(LOCAL_PATH)/libwebp-0.4.0.mk
include $(LOCAL_PATH)/spatialite-4.5.0.mk
include $(LOCAL_PATH)/cairo-1.14.12.mk
include $(LOCAL_PATH)/libcurl-7.36.0.mk
include $(LOCAL_PATH)/libgeotiff-1.4.2.mk
include $(LOCAL_PATH)/giflib-5.1.4.mk
include $(LOCAL_PATH)/geos-3.6.2.mk
include $(LOCAL_PATH)/json-c-0.13.1.mk
include $(LOCAL_PATH)/librttopo-1.1.0.mk
include $(LOCAL_PATH)/libxml2-2.9.8.mk
include $(LOCAL_PATH)/pixman-0.34.0.mk
include $(LOCAL_PATH)/fontconfig-2.11.1.mk
include $(LOCAL_PATH)/libtiff-4.0.9.mk
include $(LOCAL_PATH)/proj4-4.9.3.mk
include $(LOCAL_PATH)/iconv-1.15.mk
include $(LOCAL_PATH)/freetype-2.9.mk
include $(LOCAL_PATH)/expat-2.2.5.mk
include $(LOCAL_PATH)/lzma-xz-5.2.1.mk
include $(LOCAL_PATH)/libjpeg-6b.mk
include $(LOCAL_PATH)/libpng-1.6.34.mk
$(call import-module,android/cpufeatures)
