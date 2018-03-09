# -------------------
# Android_R4.3.0.mk
# ndk-build clean
# ndk-build
# OR
# ndk-build clean ; ndk-build
# -------------------
# As of 2018-03-09
# -------------------
# Order of running:
# -------------------
# Spatialite
# -------------------
# - javasqlite[20120209] code corrections for 64-bits
# - sqlite3[3220000]
# - spatialite [4.5.0-20180309]
# - proj4[3.9.3]
# - geos[3.6.2]
# - json-c[0.13.1]
# - librttopo[20180125]
# - libxml2[2.9.8]
# - xz[5.2.1 (5.2.3 fails)]
# - libiconv[1.15]
# -------------------
# Rasterlite2
# -------------------
LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SQLITE_PATH := sqlite-amalgamation-3220000
SPATIALITE_PATH := libspatialite-4.5.0
PROJ4_PATH := proj-4.9.3
GEOS_PATH := geos-3.6.2
JSONC_PATH := json-c-0.13.1
RTTOPO_PATH := librttopo-20180125
XML2_PATH := libxml2-2.9.8
LZMA_PATH := xz-5.2.1
ICONV_PATH := libiconv-1.15
RASTERLITE2_PATH := librasterlite2-4.5.0
GEOTIFF_PATH := libgeotiff-1.4.2
TIFF_PATH := tiff-4.0.4/libtiff
JPEG_PATH := jpeg-6b
GIF_PATH := giflib-5.1.1/lib
CAIRO_PATH := cairo-1.14.2/src
FREETYPE_PATH := freetype-2.6
FONTCONFIG_PATH := fontconfig-2.11.1
EXPAT_PATH := expat-2.1.0
PIXMAN_PATH := pixman-0.32.6
PNG_PATH := libpng-1.6.10
WEBP_PATH := libwebp-0.4.0
CURL_PATH := curl-7.36.0
CHARLS_PATH := charls-1.0
OPENJPEG_PATH := openjpeg-2.0.0

include $(LOCAL_PATH)/jsqlite-20120209.mk
include $(LOCAL_PATH)/sqlite-3220000.mk
include $(LOCAL_PATH)/spatialite-4.5.0.mk
include $(LOCAL_PATH)/proj4-4.9.3.mk
include $(LOCAL_PATH)/geos-3.6.2.mk
include $(LOCAL_PATH)/json-c-0.13.1.mk
include $(LOCAL_PATH)/librttopo-1.1.0.mk
include $(LOCAL_PATH)/libxml2-2.9.8.mk
include $(LOCAL_PATH)/lzma-xz-5.2.1.mk
include $(LOCAL_PATH)/iconv-1.15.mk
include $(LOCAL_PATH)/rasterlite2-4.5.0.mk
include $(LOCAL_PATH)/libgeotiff-1.4.2.mk
include $(LOCAL_PATH)/libtiff-4.0.4.mk
include $(LOCAL_PATH)/libjpeg-6b.mk
include $(LOCAL_PATH)/giflib-5.1.1.mk
include $(LOCAL_PATH)/cairo-1.14.2.mk
include $(LOCAL_PATH)/freetype-2.6.mk
include $(LOCAL_PATH)/fontconfig-2.11.1.mk
include $(LOCAL_PATH)/expat-2.1.0.mk
include $(LOCAL_PATH)/pixman-0.32.6.mk
include $(LOCAL_PATH)/libpng-1.6.10.mk
include $(LOCAL_PATH)/libwebp-0.4.0.mk
include $(LOCAL_PATH)/libcurl-7.36.0.mk
include $(LOCAL_PATH)/charls-1.0.mk
include $(LOCAL_PATH)/openjpeg-2.0.0.mk
$(call import-module,android/cpufeatures)
