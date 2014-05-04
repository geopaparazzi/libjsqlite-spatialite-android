include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := lzma

# based on sample found here: [https://bitbucket.org/gongminmin/klayge/src/ca44276467e2/External/7z/build/android/LZMA/jni/Android.mk]
lzma_flags := \
        -D_7ZIP_ST 

LOCAL_CFLAGS    := \
	$(lzma_flags)

LOCAL_C_INCLUDES := \
 $(LZMA_PATH)
LOCAL_SRC_FILES := \
	$(LZMA_PATH)/7zAlloc.c \
	$(LZMA_PATH)/7zBuf2.c \
	$(LZMA_PATH)/7zBuf.c \
	$(LZMA_PATH)/7zCrc.c \
	$(LZMA_PATH)/7zCrcOpt.c \
	$(LZMA_PATH)/7zDec.c \
	$(LZMA_PATH)/7zFile.c \
	$(LZMA_PATH)/7zIn.c \
	$(LZMA_PATH)/7zStream.c \
	$(LZMA_PATH)/Alloc.c \
	$(LZMA_PATH)/Bcj2.c \
	$(LZMA_PATH)/Bra86.c \
	$(LZMA_PATH)/Bra.c \
	$(LZMA_PATH)/BraIA64.c \
	$(LZMA_PATH)/CpuArch.c \
	$(LZMA_PATH)/Delta.c \
	$(LZMA_PATH)/LzFind.c \
	$(LZMA_PATH)/LzFindMt.c \
	$(LZMA_PATH)/Lzma2Dec.c \
	$(LZMA_PATH)/Lzma2Enc.c \
	$(LZMA_PATH)/Lzma86Dec.c \
	$(LZMA_PATH)/Lzma86Enc.c \
	$(LZMA_PATH)/LzmaDec.c \
	$(LZMA_PATH)/LzmaEnc.c \
	$(LZMA_PATH)/LzmaLib.c \
	$(LZMA_PATH)/MtCoder.c \
	$(LZMA_PATH)/Ppmd7.c \
	$(LZMA_PATH)/Ppmd7Dec.c \
	$(LZMA_PATH)/Ppmd7Enc.c \
	$(LZMA_PATH)/Sha256.c \
	$(LZMA_PATH)/Threads.c \
	$(LZMA_PATH)/Xz.c \
	$(LZMA_PATH)/XzCrc64.c \
	$(LZMA_PATH)/XzDec.c \
	$(LZMA_PATH)/XzEnc.c \
	$(LZMA_PATH)/XzIn.c 
include $(BUILD_STATIC_LIBRARY)
