include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# DO NOT use the config.h, only with the setting of PACKAGE will this work
# -DUSE_ARM_IWMMXT
LOCAL_MODULE    := libpixman

 pixman_flags := \
 -DPACKAGE="android-cairo" \
 -D_USE_MATH_DEFINES \
 -DPIXMAN_NO_TLS \
 -O2 \
 -std=c99 \
 -Wno-missing-field-initializers \
 -include "limits.h" 

ifeq ($(TARGET_ARCH),x86)
 LOCAL_CFLAGS   := $(pixman_flags)
else 
 ifeq ($(TARGET_ARCH),mips)
  LOCAL_CFLAGS   := $(pixman_flags)
 else
  LOCAL_CFLAGS   := -DUSE_ARM_NEON -DUSE_ARM_SIMD  $(pixman_flags)
  pixman_src := \
  	$(PIXMAN_PATH)/pixman/pixman-arm-simd.c \
 	 $(PIXMAN_PATH)/pixman/pixman-arm-simd-asm.S  \
	 $(PIXMAN_PATH)/pixman/pixman-arm-neon.c \
	 $(PIXMAN_PATH)/pixman/pixman-arm-neon-asm.S  \
	 $(PIXMAN_PATH)/pixman/pixman-arm-neon-asm-bilinear.S \
	 $(PIXMAN_PATH)/pixman/pixman-arm-simd-asm-scaled.S 
 endif
endif

LOCAL_C_INCLUDES := \
 $(PIXMAN_PATH)/pixman \
 $(PIXMAN_PATH)
LOCAL_SRC_FILES := \
	$(PIXMAN_PATH)/pixman/pixman.c \
	$(PIXMAN_PATH)/pixman/pixman-access.c \
	$(PIXMAN_PATH)/pixman/pixman-access-accessors.c	\
	$(PIXMAN_PATH)/pixman/pixman-bits-image.c		\
	$(PIXMAN_PATH)/pixman/pixman-combine32.c		\
	$(PIXMAN_PATH)/pixman/pixman-combine-float.c		\
	$(PIXMAN_PATH)/pixman/pixman-conical-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-filter.c \
	$(PIXMAN_PATH)/pixman/pixman-x86.c		\
	$(PIXMAN_PATH)/pixman/pixman-mips.c \
	$(PIXMAN_PATH)/pixman/pixman-arm.c \
	$(PIXMAN_PATH)/pixman/pixman-ppc.c \
	$(PIXMAN_PATH)/pixman/pixman-edge.c \
	$(PIXMAN_PATH)/pixman/pixman-edge-accessors.c		\
	$(PIXMAN_PATH)/pixman/pixman-fast-path.c		\
	$(PIXMAN_PATH)/pixman/pixman-glyph.c \
	$(PIXMAN_PATH)/pixman/pixman-general.c		\
	$(PIXMAN_PATH)/pixman/pixman-gradient-walker.c	\
	$(PIXMAN_PATH)/pixman/pixman-image.c \
	$(PIXMAN_PATH)/pixman/pixman-implementation.c		\
	$(PIXMAN_PATH)/pixman/pixman-linear-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-matrix.c \
	$(PIXMAN_PATH)/pixman/pixman-noop.c \
	$(PIXMAN_PATH)/pixman/pixman-arm-simd.c \
	$(PIXMAN_PATH)/pixman/pixman-arm-simd-asm.S  \
	$(PIXMAN_PATH)/pixman/pixman-arm-neon.c \
	$(PIXMAN_PATH)/pixman/pixman-arm-neon-asm.S  \
	$(PIXMAN_PATH)/pixman/pixman-arm-neon-asm-bilinear.S \
	$(PIXMAN_PATH)/pixman/pixman-arm-simd-asm-scaled.S \
	$(PIXMAN_PATH)/pixman/pixman-radial-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-region16.c		\
	$(PIXMAN_PATH)/pixman/pixman-region32.c		\
	$(PIXMAN_PATH)/pixman/pixman-solid-fill.c		\
	$(PIXMAN_PATH)/pixman/pixman-timer.c \
	$(PIXMAN_PATH)/pixman/pixman-trap.c \
	$(PIXMAN_PATH)/pixman/pixman-utils.c	 \
 $(pixman_src)
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_STATIC_LIBRARY)
	


 





