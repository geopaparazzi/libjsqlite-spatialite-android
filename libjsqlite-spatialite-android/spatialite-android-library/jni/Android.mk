# -------------------
# Android_R4.2.0.mk
# ndk-build clean
# ndk-build
# -------------------
LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SPATIALITE_PATH := libspatialite-4.2.0
GEOS_PATH := geos-3.4.2
PROJ4_PATH := proj-4.8.0
SQLITE_PATH := sqlite-amalgamation-3080500
ICONV_PATH := libiconv-1.13.1
RASTERLITE2_PATH := librasterlite2-4.2.0
GEOTIFF_PATH := libgeotiff-1.4.0
TIFF_PATH := tiff-4.0.3/libtiff
JPEG_PATH := jpeg-8d
GIF_PATH := giflib-5.0.6/lib
CAIRO_PATH := cairo-1.12.14/src
FREETYPE_PATH := freetype-2.5.3
FONTCONFIG_PATH := fontconfig-2.11.1
EXPAT_PATH := expat-2.1.0
PIXMAN_PATH := pixman-0.32.4
PNG_PATH := libpng-1.6.10
WEBP_PATH := libwebp-0.4.0
XML2_PATH := libxml2-2.9.1
CURL_PATH := curl-7.36.0
LZMA_PATH := xz-5.1.3alpha

include $(LOCAL_PATH)/jsqlite-R4.2.0.mk
include $(LOCAL_PATH)/iconv-1.13.1.mk
include $(LOCAL_PATH)/sqlite-3080500.mk
include $(LOCAL_PATH)/proj4-4.8.0.mk
include $(LOCAL_PATH)/geos-3.4.2.mk
include $(LOCAL_PATH)/spatialite-4.2.0.mk
include $(LOCAL_PATH)/libjpeg-8d.mk
include $(LOCAL_PATH)/giflib-5.0.6.mk
include $(LOCAL_PATH)/libpng-1.6.10.mk
include $(LOCAL_PATH)/libtiff-4.0.3.mk
include $(LOCAL_PATH)/libwebp-0.4.0.mk
include $(LOCAL_PATH)/pixman-0.32.4.mk
include $(LOCAL_PATH)/freetype-2.5.3.mk
include $(LOCAL_PATH)/fontconfig-2.11.1.mk
include $(LOCAL_PATH)/expat-2.1.0.mk
include $(LOCAL_PATH)/cairo-1.12.14.mk
include $(LOCAL_PATH)/libgeotiff-1.4.0.mk
include $(LOCAL_PATH)/libxml2-2.9.1.mk
include $(LOCAL_PATH)/libcurl-7.36.0.mk
include $(LOCAL_PATH)/lzma-xz-5.1.3a.mk
include $(LOCAL_PATH)/rasterlite2-4.2.0.mk
$(call import-module,android/cpufeatures)
