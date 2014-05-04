LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SPATIALITE_PATH := libspatialite-4.2.0
RASTERLITE2_PATH := librasterlite2-4.2.0
GEOTIFF_PATH := libgeotiff-1.4.0
TIFF_PATH := tiff-4.0.3/libtiff
LZMA_PATH := lzma-9.2.0
GEOS_PATH := geos-3.4.2
PROJ4_PATH := proj-4.8.0
SQLITE_PATH := sqlite-amalgamation-3080100
ICONV_PATH := libiconv-1.13.1

include $(LOCAL_PATH)/iconv-1.13.1.mk
include $(LOCAL_PATH)/sqlite-3080100.mk
include $(LOCAL_PATH)/proj4-4.8.0.mk
include $(LOCAL_PATH)/geos-3.4.2.mk
include $(LOCAL_PATH)/spatialite-4.2.0.mk
include $(LOCAL_PATH)/lzma-9.2.0.mk
include $(LOCAL_PATH)/libtiff-4.0.3.mk.mk
include $(LOCAL_PATH)/libgeotiff-1.4.0.mk
include $(LOCAL_PATH)/rasterlite2-4.2.0.mk
include $(LOCAL_PATH)/jsqlite-R4.2.0.mk
