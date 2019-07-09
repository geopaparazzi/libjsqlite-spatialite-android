include $(CLEAR_VARS)
# -------------------
# libexpat-2.2.7
# -------------------
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# directory: libexpat-R_2_2_7/expat
# -------------------
LOCAL_MODULE    := libexpat
# -------------------
# As of 2019-06-19
# -------------------
# https://github.com/libexpat/libexpat/releases
# -------------------
 expat_flags := \
 -DHAVE_EXPAT_CONFIG_H

LOCAL_CFLAGS    := \
 $(expat_flags)

LOCAL_C_INCLUDES := \
 $(EXPAT_PATH)/lib \
 $(EXPAT_PATH)

LOCAL_SRC_FILES := \
# $(EXPAT_PATH)/lib/loadlibrary.c \
 $(EXPAT_PATH)/lib/xmlparse.c \
 $(EXPAT_PATH)/lib/xmlrole.c \
 $(EXPAT_PATH)/lib/xmltok.c
include $(BUILD_STATIC_LIBRARY)

