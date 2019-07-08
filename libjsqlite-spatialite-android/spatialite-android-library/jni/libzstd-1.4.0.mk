include $(CLEAR_VARS)
# -------------------
# zstd-1.4.0
# -------------------
# As of 2019-07-07
# -------------------
# https://github.com/facebook/zstd/releases
# -------------------
LOCAL_MODULE    := liblzstd

# -E -g / -g -O2

lzstd_flags := \
 -DHAVE_CONFIG_H \
 -g -O2 \
 -std=gnu99

LOCAL_CFLAGS    := \
 $(lzstd_flags)

LOCAL_C_INCLUDES := \
 $(LZSTD_PATH)/lib

LOCAL_SRC_FILES := \
 $(LZSTD_PATH)/lib/common/debug.c \
 $(LZSTD_PATH)/lib/common/entropy_common.c \
 $(LZSTD_PATH)/lib/common/error_private.c \
 $(LZSTD_PATH)/lib/common/fse_decompress.c \
 $(LZSTD_PATH)/lib/common/pool.c \
 $(LZSTD_PATH)/lib/common/threading.c \
 $(LZSTD_PATH)/lib/common/xxhash.c \
 $(LZSTD_PATH)/lib/common/zstd_common.c \
 $(LZSTD_PATH)/lib/compress/fse_compress.c \
 $(LZSTD_PATH)/lib/compress/hist.c \
 $(LZSTD_PATH)/lib/compress/huf_compress.c \
 $(LZSTD_PATH)/lib/compress/zstd_compress.c \
 $(LZSTD_PATH)/lib/compress/zstd_double_fast.c \
 $(LZSTD_PATH)/lib/compress/zstd_fast.c \
 $(LZSTD_PATH)/lib/compress/zstd_lazy.c \
 $(LZSTD_PATH)/lib/compress/zstd_ldm.c \
 $(LZSTD_PATH)/lib/compress/zstdmt_compress.c \
 $(LZSTD_PATH)/lib/compress/zstd_opt.c \
 $(LZSTD_PATH)/lib/decompress/huf_decompress.c \
 $(LZSTD_PATH)/lib/decompress/zstd_ddict.c \
 $(LZSTD_PATH)/lib/decompress/zstd_decompress_block.c \
 $(LZSTD_PATH)/lib/decompress/zstd_decompress.c \
 $(LZSTD_PATH)/lib/dictBuilder/cover.c \
 $(LZSTD_PATH)/lib/dictBuilder/divsufsort.c \
 $(LZSTD_PATH)/lib/dictBuilder/fastcover.c \
 $(LZSTD_PATH)/lib/dictBuilder/zdict.c \
 $(LZSTD_PATH)/lib/deprecated/zbuff_common.c \
 $(LZSTD_PATH)/lib/deprecated/zbuff_compress.c \
 $(LZSTD_PATH)/lib/deprecated/zbuff_decompress.c
include $(BUILD_STATIC_LIBRARY)

