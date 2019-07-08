include $(CLEAR_VARS)
# -------------------
#  libwebp-1.0.2
# -------------------
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# -------------------
# As of 2019-01-15
# -------------------
# https://chromium.googlesource.com/webm/libwebp/
# -------------------
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
 $(WEBP_PATH)/src/dec \
 $(WEBP_PATH)/src/dsp \
 $(WEBP_PATH)/src/enc \
 $(WEBP_PATH)/src/mux \
 $(WEBP_PATH)/src/utils \
 $(WEBP_PATH)/src/webp \
 $(WEBP_PATH)
LOCAL_SRC_FILES := \
 $(WEBP_PATH)/src/dec/alpha_dec.c \
 $(WEBP_PATH)/src/dec/buffer_dec.c \
 $(WEBP_PATH)/src/dec/frame_dec.c \
 $(WEBP_PATH)/src/dec/idec_dec.c \
 $(WEBP_PATH)/src/dec/io_dec.c \
 $(WEBP_PATH)/src/dec/quant_dec.c \
 $(WEBP_PATH)/src/dec/tree_dec.c \
 $(WEBP_PATH)/src/dec/vp8_dec.c \
 $(WEBP_PATH)/src/dec/vp8l_dec.c \
 $(WEBP_PATH)/src/dec/webp_dec.c \
 $(WEBP_PATH)/src/dsp/alpha_processing.c \
 $(WEBP_PATH)/src/dsp/cpu.c \
 $(WEBP_PATH)/src/dsp/dec.c \
 $(WEBP_PATH)/src/dsp/dec_clip_tables.c \
 $(WEBP_PATH)/src/dsp/dec_sse2.c \
 $(WEBP_PATH)/src/dsp/enc.c \
 $(WEBP_PATH)/src/dsp/enc_sse2.c \
 $(WEBP_PATH)/src/dsp/lossless.c \
 $(WEBP_PATH)/src/dsp/rescaler.c  \
 $(WEBP_PATH)/src/dsp/upsampling.c \
 $(WEBP_PATH)/src/dsp/upsampling_sse2.c \
 $(WEBP_PATH)/src/dsp/yuv.c \
 $(WEBP_PATH)/src/enc/alpha_enc.c \
 $(WEBP_PATH)/src/enc/analysis_enc.c \
 $(WEBP_PATH)/src/enc/backward_references_cost_enc.c \
 $(WEBP_PATH)/src/enc/backward_references_enc.c \
 $(WEBP_PATH)/src/enc/config_enc.c \
 $(WEBP_PATH)/src/enc/cost_enc.c \
 $(WEBP_PATH)/src/enc/filter_enc.c \
 $(WEBP_PATH)/src/enc/frame_enc.c \
 $(WEBP_PATH)/src/enc/histogram_enc.c \
 $(WEBP_PATH)/src/enc/iterator_enc.c \
 $(WEBP_PATH)/src/enc/near_lossless_enc.c \
 $(WEBP_PATH)/src/enc/picture_enc.c \
 $(WEBP_PATH)/src/enc/picture_csp_enc.c \
 $(WEBP_PATH)/src/enc/picture_psnr_enc.c \
 $(WEBP_PATH)/src/enc/picture_rescale_enc.c \
 $(WEBP_PATH)/src/enc/picture_tools_enc.c \
 $(WEBP_PATH)/src/enc/predictor_enc.c \
 $(WEBP_PATH)/src/enc/quant_enc.c \
 $(WEBP_PATH)/src/enc/syntax_enc.c \
 $(WEBP_PATH)/src/enc/token_enc.c \
 $(WEBP_PATH)/src/enc/tree_enc.c \
 $(WEBP_PATH)/src/enc/vp8l_enc.c \
 $(WEBP_PATH)/src/enc/webp_enc.c \
 $(WEBP_PATH)/src/mux/anim_encode.c \
 $(WEBP_PATH)/src/mux/muxedit.c \
 $(WEBP_PATH)/src/mux/muxinternal.c \
 $(WEBP_PATH)/src/mux/muxread.c \
 $(WEBP_PATH)/src/utils/bit_reader_utils.c \
 $(WEBP_PATH)/src/utils/bit_writer_utils.c \
 $(WEBP_PATH)/src/utils/color_cache_utils.c \
 $(WEBP_PATH)/src/utils/filters_utils.c \
 $(WEBP_PATH)/src/utils/huffman_utils.c \
 $(WEBP_PATH)/src/utils/quant_levels_dec_utils.c \
 $(WEBP_PATH)/src/utils/random_utils.c \
 $(WEBP_PATH)/src/utils/rescaler_utils.c \
 $(WEBP_PATH)/src/utils/thread_utils.c \
 $(WEBP_PATH)/src/utils/utils.c

# prefer arm over thumb mode for performance gains
LOCAL_ARM_MODE := arm

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
 # Setting LOCAL_ARM_NEON will enable -mfpu=neon which may cause illegal
 # instructions to be generated for armv7a code. Instead target the neon code
 # specifically.
 LOCAL_SRC_FILES += $(WEBP_PATH)/src/dsp/dec_neon.c.neon
 LOCAL_SRC_FILES += $(WEBP_PATH)/src/dsp/upsampling_neon.c.neon
 LOCAL_SRC_FILES += $(WEBP_PATH)/src/dsp/enc_neon.c.neon
endif

LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_STATIC_LIBRARY)






