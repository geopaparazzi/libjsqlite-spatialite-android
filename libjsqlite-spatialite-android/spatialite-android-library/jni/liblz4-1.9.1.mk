include $(CLEAR_VARS)
# -------------------
# lz4-1.9.1
# -------------------
# As of 2019-07-07
# -------------------
LOCAL_MODULE    := liblz4

# -E -g / -g -O2

lz4_flags := \
 -DHAVE_CONFIG_H \
 -g -O2 \
 -std=gnu99

LOCAL_CFLAGS    := \
 $(lz4_flags)

LOCAL_C_INCLUDES := \
 $(LZ4_PATH)/src/lib

LOCAL_SRC_FILES := \
 $(LZ4_PATH)/src/lib/lz4.c \
 $(LZ4_PATH)/src/lib/lz4frame.c \
 $(LZ4_PATH)/src/lib/lz4hc.c
include $(BUILD_STATIC_LIBRARY)

