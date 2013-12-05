include $(CLEAR_VARS)
# -DOMIT_GEOS=0
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := spatialite
LOCAL_CFLAGS    := -Dfdatasync=fsync \
        -DOMIT_FREEXL \
        -DVERSION=\"3.0.1\" \
        -D__ANDROID__ \
        -DSQLITE_ENABLE_RTREE=1
LOCAL_LDLIBS    := -llog 
LOCAL_C_INCLUDES := \
        $(ICONV_PATH)/include \
        $(ICONV_PATH)/libcharset/include \
        $(GEOS_PATH)/source/headers \
        $(GEOS_PATH)/capi \
        $(PROJ4_PATH)/src
LOCAL_SRC_FILES := \
        $(SPATIALITE_PATH)/spatialite.c \
        $(SPATIALITE_PATH)/sqlite3.c
LOCAL_STATIC_LIBRARIES := iconv proj geos
include $(BUILD_STATIC_LIBRARY)