include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libpixman

# make and listed *.0 \ \ files in src ; make clean

pixman_flags := \
 -DHAVE_CONFIG_H=1

LOCAL_CFLAGS    := \
	$(pixman_flags)

LOCAL_C_INCLUDES := \
 $(PIXMAN_PATH)/pixman \
 $(PIXMAN_PATH)
LOCAL_SRC_FILES := \
	$(PIXMAN_PATH)/pixman/pixman.c			\
	$(PIXMAN_PATH)/pixman/pixman-access.c			\
	$(PIXMAN_PATH)/pixman/pixman-access-accessors.c	\
	$(PIXMAN_PATH)/pixman/pixman-bits-image.c		\
	$(PIXMAN_PATH)/pixman/pixman-combine32.c		\
	$(PIXMAN_PATH)/pixman/pixman-combine-float.c		\
	$(PIXMAN_PATH)/pixman/pixman-conical-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-filter.c			\
	$(PIXMAN_PATH)/pixman/pixman-x86.c			\
	$(PIXMAN_PATH)/pixman/pixman-mips.c			\
	$(PIXMAN_PATH)/pixman/pixman-arm.c			\
	$(PIXMAN_PATH)/pixman/pixman-ppc.c			\
	$(PIXMAN_PATH)/pixman/pixman-edge.c			\
	$(PIXMAN_PATH)/pixman/pixman-edge-accessors.c		\
	$(PIXMAN_PATH)/pixman/pixman-fast-path.c		\
	$(PIXMAN_PATH)/pixman/pixman-glyph.c			\
	$(PIXMAN_PATH)/pixman/pixman-general.c		\
	$(PIXMAN_PATH)/pixman/pixman-gradient-walker.c	\
	$(PIXMAN_PATH)/pixman/pixman-image.c			\
	$(PIXMAN_PATH)/pixman/pixman-implementation.c		\
	$(PIXMAN_PATH)/pixman/pixman-linear-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-matrix.c			\
	$(PIXMAN_PATH)/pixman/pixman-noop.c			\
	$(PIXMAN_PATH)/pixman/pixman-radial-gradient.c	\
	$(PIXMAN_PATH)/pixman/pixman-region16.c		\
	$(PIXMAN_PATH)/pixman/pixman-region32.c		\
	$(PIXMAN_PATH)/pixman/pixman-solid-fill.c		\
	$(PIXMAN_PATH)/pixman/pixman-timer.c			\
	$(PIXMAN_PATH)/pixman/pixman-trap.c			\
	$(PIXMAN_PATH)/pixman/pixman-utils.c	
include $(BUILD_STATIC_LIBRARY)
	


 





