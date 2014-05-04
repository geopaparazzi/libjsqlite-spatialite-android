include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libjpeg

# based on sample found here: [https://bitbucket.org/gongminmin/klayge/src/ca44276467e2/External/7z/build/android/LZMA/jni/Android.mk]

LOCAL_CFLAGS    := \
	$(jpeg_flags)

LOCAL_C_INCLUDES := \
 $(JPEG_PATH)
LOCAL_SRC_FILES := \
	$(JPEG_PATH)/jcapimin.c \
	$(JPEG_PATH)/jcapistd.c \
	$(JPEG_PATH)/jccoefct.c \
	$(JPEG_PATH)/jccolor.c \
	$(JPEG_PATH)/jcdctmgr.c \
	$(JPEG_PATH)/jchuff.c \
	$(JPEG_PATH)/jcinit.c \
	$(JPEG_PATH)/jcmainct.c \
	$(JPEG_PATH)/jcmarker.c \
	$(JPEG_PATH)/jcmaster.c \
	$(JPEG_PATH)/jcomapi.c \
	$(JPEG_PATH)/jcparam.c \
	$(JPEG_PATH)/jcprepct.c \
	$(JPEG_PATH)/jcsample.c \
	$(JPEG_PATH)/jctrans.c \
	$(JPEG_PATH)/jdapimin.c \
	$(JPEG_PATH)/jdapistd.c \
	$(JPEG_PATH)/jdatadst.c \
	$(JPEG_PATH)/jdatasrc.c \
	$(JPEG_PATH)/jdcoefct.c \
	$(JPEG_PATH)/jdcolor.c \
	$(JPEG_PATH)/jddctmgr.c \
	$(JPEG_PATH)/jdhuff.c \
	$(JPEG_PATH)/jdinput.c \
	$(JPEG_PATH)/jdmainct.c \
	$(JPEG_PATH)/jdmarker.c \
	$(JPEG_PATH)/jdmaster.c \
	$(JPEG_PATH)/jdmerge.c \
	$(JPEG_PATH)/jdpostct.c \
	$(JPEG_PATH)/jdsample.c \
	$(JPEG_PATH)/jdtrans.c \
	$(JPEG_PATH)/jerror.c \
	$(JPEG_PATH)/jfdctflt.c \
	$(JPEG_PATH)/jfdctfst.c \
	$(JPEG_PATH)/jfdctint.c \
	$(JPEG_PATH)/jidctflt.c \
	$(JPEG_PATH)/jidctfst.c \
	$(JPEG_PATH)/jidctint.c \
	$(JPEG_PATH)/jquant1.c \
	$(JPEG_PATH)/jquant2.c \
	$(JPEG_PATH)/jutils.c \
	$(JPEG_PATH)/jmemmgr.c \
	$(JPEG_PATH)/jcarith.c \
	$(JPEG_PATH)/jdarith.c \
	$(JPEG_PATH)/jaricom.c 
include $(BUILD_STATIC_LIBRARY)


