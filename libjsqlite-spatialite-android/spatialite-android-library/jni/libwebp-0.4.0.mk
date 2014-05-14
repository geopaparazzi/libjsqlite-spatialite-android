include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libwebp

webp_flags := \
 -DHAVE_CONFIG_H=1 

ifeq ($(TARGET_ARCH),x86)
 LOCAL_CFLAGS   := $(webp_flags)
else 
 ifeq ($(TARGET_ARCH),mips)
  LOCAL_CFLAGS   := $(webp_flags)
 else
  LOCAL_CFLAGS   := -mfpu=neon $(webp_flags)
 endif
endif

LOCAL_C_INCLUDES := \
 $(WEBP_PATH)
LOCAL_SRC_FILES := \
	$(WEBP_PATH)/src/dec/alpha.c \
	$(WEBP_PATH)/src/dec/buffer.c \
	$(WEBP_PATH)/src/dec/frame.c \
	$(WEBP_PATH)/src/dec/idec.c \
	$(WEBP_PATH)/src/dec/io.c \
	$(WEBP_PATH)/src/dec/layer.c \
	$(WEBP_PATH)/src/dec/quant.c \
	$(WEBP_PATH)/src/dec/tree.c \
	$(WEBP_PATH)/src/dec/vp8.c \
	$(WEBP_PATH)/src/dec/vp8l.c \
	$(WEBP_PATH)/src/dec/webp.c \
	$(WEBP_PATH)/src/dsp/cpu.c \
	$(WEBP_PATH)/src/dsp/dec.c \
	$(WEBP_PATH)/src/dsp/dec_neon.c \
	$(WEBP_PATH)/src/dsp/dec_sse2.c \
	$(WEBP_PATH)/src/dsp/enc.c \
	$(WEBP_PATH)/src/dsp/enc_sse2.c \
	$(WEBP_PATH)/src/dsp/lossless.c \
	$(WEBP_PATH)/src/dsp/upsampling.c \
	$(WEBP_PATH)/src/dsp/upsampling_sse2.c \
	$(WEBP_PATH)/src/dsp/yuv.c \
	$(WEBP_PATH)/src/enc/alpha.c \
	$(WEBP_PATH)/src/enc/analysis.c \
	$(WEBP_PATH)/src/enc/backward_references.c \
	$(WEBP_PATH)/src/enc/config.c \
	$(WEBP_PATH)/src/enc/cost.c \
	$(WEBP_PATH)/src/enc/filter.c \
	$(WEBP_PATH)/src/enc/frame.c \
	$(WEBP_PATH)/src/enc/histogram.c \
	$(WEBP_PATH)/src/enc/iterator.c \
	$(WEBP_PATH)/src/enc/layer.c \
	$(WEBP_PATH)/src/enc/picture.c \
	$(WEBP_PATH)/src/enc/quant.c \
	$(WEBP_PATH)/src/enc/syntax.c \
	$(WEBP_PATH)/src/enc/token.c \
	$(WEBP_PATH)/src/enc/tree.c \
	$(WEBP_PATH)/src/enc/vp8l.c \
	$(WEBP_PATH)/src/enc/webpenc.c \
	$(WEBP_PATH)/src/utils/bit_reader.c \
	$(WEBP_PATH)/src/utils/bit_writer.c \
	$(WEBP_PATH)/src/utils/color_cache.c \
	$(WEBP_PATH)/src/utils/filters.c \
	$(WEBP_PATH)/src/utils/huffman.c \
	$(WEBP_PATH)/src/utils/huffman_encode.c \
	$(WEBP_PATH)/src/utils/quant_levels.c \
	$(WEBP_PATH)/src/utils/random.c \
	$(WEBP_PATH)/src/utils/rescaler.c \
	$(WEBP_PATH)/src/utils/thread.c \
	$(WEBP_PATH)/src/utils/utils.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_STATIC_LIBRARY)




 



