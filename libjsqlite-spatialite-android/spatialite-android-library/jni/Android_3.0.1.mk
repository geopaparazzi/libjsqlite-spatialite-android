# -------------------
# Android_3.0.1.mk
# ndk-build clean
# ndk-build
# -------------------
LOCAL_PATH := $(call my-dir)

JSQLITE_PATH := javasqlite-20120209
SPATIALITE_PATH := libspatialite-amalgamation-3.0.1
ICONV_PATH := libiconv-1.13.1
GEOS_PATH := geos-3.2.2
PROJ4_PATH := proj-4.7.0

include $(LOCAL_PATH)/iconv-1.13.1.mk
include $(LOCAL_PATH)/proj4-4.7.0.mk
include $(LOCAL_PATH)/geos-3.2.2.mk
include $(LOCAL_PATH)/spatialite-3.0.1.mk
include $(LOCAL_PATH)/jsqlite-3.0.1.mk
