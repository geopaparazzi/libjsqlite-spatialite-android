include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := giflib

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
	$(GIF_PATH)/quantize.c 
include $(BUILD_STATIC_LIBRARY)



