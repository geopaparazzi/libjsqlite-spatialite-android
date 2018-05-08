# -------------------
# Android_5.0.0.mk
# -------------------
# For Spatialite without RasterLite2
# - jsqlite-20120209.mk
# 'HAVE_RASTERLITE2=0'
# -------------------
# [from 'jni/' directory]
# ndk-build clean
# ndk-build
# OR
# ndk-build clean ; ndk-build
# -------------------
# As of 2018-05-08
# -------------------
# Order of running:
# - javasqlite[20120209] code corrections for 64-bits
# - sqlite3[3220000]
# - spatialite [5.0.0-20180508]
# - proj4[3.9.3]
# - geos[3.6.2]
# - json-c[0.13.1]
# - librttopo[20180125]
# - libxml2[2.9.8]
# - xz[5.2.1 (5.2.3 fails)]
# - libiconv[1.15]
# -------------------
LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SQLITE_PATH := sqlite-amalgamation-3220000
SPATIALITE_PATH := libspatialite-5.0.0
PROJ4_PATH := proj-4.9.3
GEOS_PATH := geos-3.6.2
JSONC_PATH := json-c-0.13.1
RTTOPO_PATH := librttopo-20180125
XML2_PATH := libxml2-2.9.8
LZMA_PATH := xz-5.2.1
ICONV_PATH := libiconv-1.15
# -------------------
include $(LOCAL_PATH)/jsqlite-20120209.mk
include $(LOCAL_PATH)/sqlite-3220000.mk
include $(LOCAL_PATH)/spatialite-5.0.0.mk
include $(LOCAL_PATH)/proj4-4.9.3.mk
include $(LOCAL_PATH)/geos-3.6.2.mk
include $(LOCAL_PATH)/json-c-0.13.1.mk
include $(LOCAL_PATH)/librttopo-1.1.0.mk
include $(LOCAL_PATH)/libxml2-2.9.8.mk
include $(LOCAL_PATH)/lzma-xz-5.2.1.mk
include $(LOCAL_PATH)/iconv-1.15.mk
$(call import-module,android/cpufeatures)
