include $(CLEAR_VARS)
# 2014-10-03
LOCAL_MODULE    := libcharls

LOCAL_CFLAGS    := \
 $(charls_flags)

LOCAL_C_INCLUDES := \
 $(CHARLS_PATH)
LOCAL_SRC_FILES := \
 $(CHARLS_PATH)/header.cpp \
 $(CHARLS_PATH)/interface.cpp \
 $(CHARLS_PATH)/jpegls.cpp 
include $(BUILD_STATIC_LIBRARY)





