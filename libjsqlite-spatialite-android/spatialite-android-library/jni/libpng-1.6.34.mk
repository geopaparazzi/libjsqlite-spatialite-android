include $(CLEAR_VARS)
# -------------------
#  libpng-1.6.34
# -------------------
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# After configure
# comment out VERSION in config.h - conflicts with RASTERLITE2 version. avoiding macro redefined warnings
# cp scripts/pnglibconf.h.prebuilt pnglibconf.h
# ls -la *.c [without example.c] \ files in libpng-1.6.34
# -------------------
# As of 2018-03-10
# -------------------
# http://www.libpng.org/pub/png/libpng.html
# -------------------
LOCAL_MODULE    := libpng

LOCAL_CFLAGS    := \
 $(png_flags)

LOCAL_C_INCLUDES := \
 $(PNG_PATH)
LOCAL_SRC_FILES := \
 $(PNG_PATH)/png.c \
 $(PNG_PATH)/pngerror.c \
 $(PNG_PATH)/pngget.c \
 $(PNG_PATH)/pngmem.c \
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





