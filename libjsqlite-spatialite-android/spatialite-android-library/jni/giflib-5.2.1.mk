include $(CLEAR_VARS)
# -------------------
#  libgif-5.2.1
# -------------------
# no ./configure 
# -------------------
# As of 2019-06-24
# -------------------
# https://sourceforge.net/projects/giflib/files/
# -------------------
LOCAL_MODULE    := libgif

# Add missing S_* vars  in lib/gif_lib.h since in 5.0.6

# based on sample found here: [https://bitbucket.org/gongminmin/klayge/src/ca44276467e2/External/7z/build/android/LZMA/jni/Android.mk]

LOCAL_CFLAGS    := \
 $(gif_flags)

LOCAL_C_INCLUDES := \
 $(GIF_PATH)
LOCAL_SRC_FILES := \
 $(GIF_PATH)/dgif_lib.c \
 $(GIF_PATH)/egif_lib.c \
 $(GIF_PATH)/gifalloc.c \
 $(GIF_PATH)/gif_err.c \
 $(GIF_PATH)/gif_font.c \
 $(GIF_PATH)/gif_hash.c \
 $(GIF_PATH)/openbsd-reallocarray.c

include $(BUILD_STATIC_LIBRARY)



