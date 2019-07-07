include $(CLEAR_VARS)
# -------------------
# lz4-1.9.1
# -------------------
# As of 2019-07-07
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
 $(LZSTD_PATH)/src/lib

LOCAL_SRC_FILES := \
 $(LZSTD_PATH)/src/common/debug.c \
 $(LZSTD_PATH)/src/common/entropy_common.c \
 $(LZSTD_PATH)/src/common/error_private.c \
 $(LZSTD_PATH)/src/common/fse_decompress.c \
 $(LZSTD_PATH)/src/common/pool.c \
 $(LZSTD_PATH)/src/common/threading.c \
 $(LZSTD_PATH)/src/common/xxhash.c \
 $(LZSTD_PATH)/src/common/zstd_common.c \
 $(LZSTD_PATH)/src/compress/fse_compress.c \
 $(LZSTD_PATH)/src/compress/hist.c \
 $(LZSTD_PATH)/src/compress/huf_compress.c \
 $(LZSTD_PATH)/src/compress/zstd_compress.c \
 $(LZSTD_PATH)/src/compress/zstd_double_fast.c \
 $(LZSTD_PATH)/src/compress/zstd_fast.c \
 $(LZSTD_PATH)/src/compress/zstd_lazy.c \
 $(LZSTD_PATH)/src/compress/zstd_ldm.c \
 $(LZSTD_PATH)/src/compress/zstdmt_compress.c \
 $(LZSTD_PATH)/src/compress/zstd_opt.c \
 $(LZSTD_PATH)/src/decompress/huf_decompress.c \
 $(LZSTD_PATH)/src/decompress/zstd_ddict.c \
 $(LZSTD_PATH)/src/decompress/zstd_decompress_block.c \
 $(LZSTD_PATH)/src/decompress/zstd_decompress.c \
 $(LZSTD_PATH)/src/dictBuilder/cover.c \
 $(LZSTD_PATH)/src/dictBuilder/divsufsort.c \
 $(LZSTD_PATH)/src/dictBuilder/fastcover.c \
 $(LZSTD_PATH)/src/dictBuilder/zdict.c \
 $(LZSTD_PATH)/src/deprecated/zbuff_common.c \
 $(LZSTD_PATH)/src/deprecated/zbuff_compress.c \
 $(LZSTD_PATH)/src/deprecated/zbuff_decompress.c
include $(BUILD_STATIC_LIBRARY)

