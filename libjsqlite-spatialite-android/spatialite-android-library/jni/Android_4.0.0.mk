LOCAL_PATH := $(call my-dir)

JSQLITE_PATH := javasqlite-20120209
SPATIALITE_PATH := libspatialite-4.0.0
GEOS_PATH := geos-3.4.2
PROJ4_PATH := proj-4.8.0
SQLITE_PATH := sqlite-amalgamation-3080100

include $(LOCAL_PATH)/sqlite-3080100.mk
include $(LOCAL_PATH)/proj4-4.8.0.mk
include $(LOCAL_PATH)/geos-3.4.2.mk
include $(LOCAL_PATH)/spatialite-4.0.0.mk
include $(LOCAL_PATH)/jsqlite-20120209.mk
