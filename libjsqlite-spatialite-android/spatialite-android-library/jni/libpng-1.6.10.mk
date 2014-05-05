include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libpng

# make and listed *.o \ files in libpng-1.6.10 ; make clean

LOCAL_CFLAGS    := \
	$(png_flags)

LOCAL_C_INCLUDES := \
 $(PNG_PATH)
LOCAL_SRC_FILES := \
	$(PNG_PATH)/pngerror.c \
	$(PNG_PATH)/pngget.c \
	$(PNG_PATH)/pngmem.c \
	$(PNG_PATH)/png.c \
	$(PNG_PATH)/pngpread.c \
	$(PNG_PATH)/pngread.c \
	$(PNG_PATH)/pngrio.c \
	$(PNG_PATH)/pngrtran.c \
	$(PNG_PATH)/pngrutil.c \
	$(PNG_PATH)/pngset.c \
	$(PNG_PATH)/pngtest.c \
	$(PNG_PATH)/pngtrans.c \
	$(PNG_PATH)/pngwio.c \
	$(PNG_PATH)/pngwrite.c \
	$(PNG_PATH)/pngwtran.c \
	$(PNG_PATH)/pngwutil.c \
	$(PNG_PATH)/arm/arm_init.c \
	$(PNG_PATH)/arm/filter_neon.S \
	$(PNG_PATH)/arm/filter_neon_intrinsics.c
include $(BUILD_STATIC_LIBRARY)





