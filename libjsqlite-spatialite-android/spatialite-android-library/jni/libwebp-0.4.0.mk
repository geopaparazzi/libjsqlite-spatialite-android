include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libwebp

# make and listed *.o \ files in libpng-1.6.10 ; make clean

LOCAL_CFLAGS    := \
	$(webp_flags)

LOCAL_C_INCLUDES := \
 $(WEBP_PATH)
LOCAL_SRC_FILES := \
	$(WEBP_PATH)/pngerror.c \
	$(WEBP_PATH)/pngget.c \
	$(WEBP_PATH)/pngmem.c \
	$(WEBP_PATH)/png.c \
	$(WEBP_PATH)/pngpread.c \
	$(WEBP_PATH)/pngread.c \
	$(WEBP_PATH)/pngrio.c \
	$(WEBP_PATH)/pngrtran.c \
	$(WEBP_PATH)/pngrutil.c \
	$(WEBP_PATH)/pngset.c \
	$(WEBP_PATH)/pngtest.c \
	$(WEBP_PATH)/pngtrans.c \
	$(WEBP_PATH)/pngwio.c \
	$(WEBP_PATH)/pngwrite.c \
	$(WEBP_PATH)/pngwtran.c \
	$(WEBP_PATH)/pngwutil.c \
	$(WEBP_PATH)/arm/arm_init.c \
	$(WEBP_PATH)/arm/filter_neon.S \
	$(WEBP_PATH)/arm/filter_neon_intrinsics.c
include $(BUILD_STATIC_LIBRARY)





