include $(CLEAR_VARS)
#./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# -------------------
# As of 2015-10-02
# -------------------
# changes:
# - geos-3.5.0
# - liblwgeom
# -------------------
LOCAL_MODULE    := libljson-c

# spatialite flags
# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
libljson-c_flags := \
 -DTARGET_CPU=\"$(TARGET_ARCH_ABI)\"

LOCAL_CFLAGS    := \
 $(libljson-c_flags)

LOCAL_C_INCLUDES := \
$(JSONC_PATH)
LOCAL_SRC_FILES := \
$(JSONC_PATH)/arraylist.c \
$(JSONC_PATH)/debug.c \
$(JSONC_PATH)/json_c_version.c \
$(JSONC_PATH)/json_object.c \
$(JSONC_PATH)/json_object_iterator.c \
$(JSONC_PATH)/json_tokener.c \
$(JSONC_PATH)/json_util.c \
$(JSONC_PATH)/libjson.c \
$(JSONC_PATH)/linkhash.c \
$(JSONC_PATH)/printbuf.c \
$(JSONC_PATH)/random_seed.c
include $(BUILD_STATIC_LIBRARY)
