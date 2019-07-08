include $(CLEAR_VARS)
# -------------------
# 2016-05-15
# -------------------
#  libcharls-1.1.0
# -------------------
# https://github.com/team-charls/charls/releases
# https://github.com/team-charls/charls/releases/tag/1.1.0
# -------------------
LOCAL_MODULE    := libcharls

LOCAL_CFLAGS    := \
 $(charls_flags)

LOCAL_C_INCLUDES := \
 $(CHARLS_PATH)
LOCAL_SRC_FILES := \
 $(CHARLS_PATH)/header.cpp \
 $(CHARLS_PATH)/interface.cpp \
 $(CHARLS_PATH)/jpegls.cpp \
 $(CHARLS_PATH)/jpegmarkersegment.cpp \
 $(CHARLS_PATH)/jpegstreamwriter.cpp
include $(BUILD_STATIC_LIBRARY)






