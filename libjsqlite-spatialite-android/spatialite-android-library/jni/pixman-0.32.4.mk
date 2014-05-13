include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libpixman

# make and listed *.0 \ \ files in src ; make clean

LOCAL_CFLAGS    := \
	$(pixman_flags)

LOCAL_C_INCLUDES := \
 $(PIXMAN_PATH)
LOCAL_SRC_FILES := \
	$(PIXMAN_PATH)/pixman.c			\
	$(PIXMAN_PATH)/pixman-access.c			\
	$(PIXMAN_PATH)/pixman-access-accessors.c	\
	$(PIXMAN_PATH)/pixman-bits-image.c		\
	$(PIXMAN_PATH)/pixman-combine32.c		\
	$(PIXMAN_PATH)/pixman-combine-float.c		\
	$(PIXMAN_PATH)/pixman-conical-gradient.c	\
	$(PIXMAN_PATH)/pixman-filter.c			\
	$(PIXMAN_PATH)/pixman-x86.c			\
	$(PIXMAN_PATH)/pixman-mips.c			\
	$(PIXMAN_PATH)/pixman-arm.c			\
	$(PIXMAN_PATH)/pixman-ppc.c			\
	$(PIXMAN_PATH)/pixman-edge.c			\
	$(PIXMAN_PATH)/pixman-edge-accessors.c		\
	$(PIXMAN_PATH)/pixman-fast-path.c		\
	$(PIXMAN_PATH)/pixman-glyph.c			\
	$(PIXMAN_PATH)/pixman-general.c		\
	$(PIXMAN_PATH)/pixman-gradient-walker.c	\
	$(PIXMAN_PATH)/pixman-image.c			\
	$(PIXMAN_PATH)/pixman-implementation.c		\
	$(PIXMAN_PATH)/pixman-linear-gradient.c	\
	$(PIXMAN_PATH)/pixman-matrix.c			\
	$(PIXMAN_PATH)/pixman-noop.c			\
	$(PIXMAN_PATH)/pixman-radial-gradient.c	\
	$(PIXMAN_PATH)/pixman-region16.c		\
	$(PIXMAN_PATH)/pixman-region32.c		\
	$(PIXMAN_PATH)/pixman-solid-fill.c		\
	$(PIXMAN_PATH)/pixman-timer.c			\
	$(PIXMAN_PATH)/pixman-trap.c			\
	$(PIXMAN_PATH)/pixman-utils.c	
include $(BUILD_STATIC_LIBRARY)

	


 





