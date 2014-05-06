include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libpixman

# make and listed *.0 \ \ files in src ; make clean

LOCAL_CFLAGS    := \
	$(pixman_flags)

LOCAL_C_INCLUDES := \
 $(PIXMAN_PATH)
LOCAL_SRC_FILES := \
	$(PIXMAN_PATH)/pixman-region.c		\
	$(PIXMAN_PATH)/pixman-image.c		\
	$(PIXMAN_PATH)/pixman-compose.c	\
	$(PIXMAN_PATH)/pixman-compose-accessors.c	\
	$(PIXMAN_PATH)/pixman-pict.c		\
	$(PIXMAN_PATH)/pixman-utils.c		\
	$(PIXMAN_PATH)/pixman-edge.c		\
	$(PIXMAN_PATH)/pixman-edge-accessors.c		\
	$(PIXMAN_PATH)/pixman-trap.c		\
	$(PIXMAN_PATH)/pixman-compute-region.c \
	$(PIXMAN_PATH)/pixman-timer.c
include $(BUILD_STATIC_LIBRARY)


 





