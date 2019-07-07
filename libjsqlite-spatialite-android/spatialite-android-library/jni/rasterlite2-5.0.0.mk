include $(CLEAR_VARS)
# 20190707.libspatialite-5.0.0
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# -------------------
# As of 2019-07-07
# -------------------
# changes:
# - geos-3.6.1
# - proj4-3.9.3
# - rttopo 1.1.0-dev
# - json-c-0.12
# - spatialite [4.4.0-20170118]
# -------------------
LOCAL_MODULE    := rasterlite2

# SQLite flags copied from ASOP [may not be needed for rasterlite2]
common_sqlite_flags := \
 -DHAVE_USLEEP=1 \
 -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
 -DSQLITE_THREADSAFE=1 \
 -DNDEBUG=1 \
 -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
 -DSQLITE_HAS_COLUMN_METADATA=1 \
 -DSQLITE_DEFAULT_AUTOVACUUM=1 \
 -DSQLITE_TEMP_STORE=3 \
 -DSQLITE_ENABLE_FTS3 \
 -DSQLITE_ENABLE_FTS3_BACKWARDS \
 -DSQLITE_ENABLE_RTREE=1 \
 -DSQLITE_DEFAULT_FILE_FORMAT=4

# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
spatialite_flags := \
 -DOMIT_FREEXL \
 -DTARGET_CPU=\"$(TARGET_ARCH_ABI)\" \
 -Dfdatasync=fsync \
 -DSQLITE_ENABLE_RTREE=1 \
 -DENABLE_GCP=1 \
 -DENABLE_GEOPACKAGE=1 \
 -DENABLE_LIBXML2=1 \
 -DENABLE_RTTOPO=1 \
 -DPROJ_NEW=1 \
 -DSQLITE_OMIT_BUILTIN_TEST=1

# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
# comment out VERSION in config.h - manually set to avoid conflict with other packages
rasterlite2_flags := \
 -DTARGET_CPU=\"$(TARGET_ARCH_ABI)\" \
 -DVERSION="\"1.1.0-devel\"" \
 -O

LOCAL_CFLAGS    := \
 $(common_sqlite_flags) \
 $(spatialite_flags) \
 $(rasterlite2_flags)

# 2014-10-03 - adapted based on ls -1 result
LOCAL_C_INCLUDES := \
 $(SQLITE_PATH) \
 $(GEOTIFF_PATH)/libxtiff \
 $(GEOTIFF_PATH) \
 $(TIFF_PATH) \
 $(JPEG_PATH) \
 $(GIF_PATH) \
 $(PNG_PATH) \
 $(WEBP_PATH)/src/webp \
 $(WEBP_PATH)/src/dec \
 $(WEBP_PATH)/src/dsp \
 $(WEBP_PATH)/src/enc \
 $(WEBP_PATH)/src/utils \
 $(WEBP_PATH)/src \
 $(WEBP_PATH) \
 $(CAIRO_PATH) \
 $(FONTCONFIG_PATH) \
 $(ICONV_PATH)/include \
 $(FREETYPE_PATH)/include \
 $(ICONV_PATH)/libcharset/include \
 $(XML2_PATH)/include \
 $(CURL_PATH) \
 $(CURL_PATH)/include \
 $(CURL_PATH)/lib \
 $(RASTERLITE2_PATH) \
 $(RASTERLITE2_PATH)/headers \
 $(SPATIALITE_PATH)/src/headers \
 $(LZMA_PATH)/src/liblzma/api \
 $(LZ4_PATH)/src/lib \
 $(LZSTD_PATH)/src/lib \
 $(OPENJPEG_PATH)/src/lib/openjp2 \
 $(CHARLS_PATH)

LOCAL_SRC_FILES := \
 $(RASTERLITE2_PATH)/src/md5.c \
 $(RASTERLITE2_PATH)/src/rasterlite2.c \
 $(RASTERLITE2_PATH)/src/rl2ascii.c \
 $(RASTERLITE2_PATH)/src/rl2auxfont.c \
 $(RASTERLITE2_PATH)/src/rl2auxgeom.c \
 $(RASTERLITE2_PATH)/src/rl2auxrender.c \
 $(RASTERLITE2_PATH)/src/rl2charls.c \
 $(RASTERLITE2_PATH)/src/rl2codec.c \
 $(RASTERLITE2_PATH)/src/rl2dbms.c \
 $(RASTERLITE2_PATH)/src/rl2gif.c \
 $(RASTERLITE2_PATH)/src/rl2import.c \
 $(RASTERLITE2_PATH)/src/rl2_internal_data.c \
 $(RASTERLITE2_PATH)/src/rl2jpeg.c \
 $(RASTERLITE2_PATH)/src/rl2md5.c \
 $(RASTERLITE2_PATH)/src/rl2openjpeg.c \
 $(RASTERLITE2_PATH)/src/rl2paint.c \
 $(RASTERLITE2_PATH)/src/rl2png.c \
 $(RASTERLITE2_PATH)/src/rl2pyramid.c \
 $(RASTERLITE2_PATH)/src/rl2rastersym.c \
 $(RASTERLITE2_PATH)/src/rl2raw.c \
 $(RASTERLITE2_PATH)/src/rl2sql.c \
 $(RASTERLITE2_PATH)/src/rl2sqlaux.c \
 $(RASTERLITE2_PATH)/src/rl2svg.c \
 $(RASTERLITE2_PATH)/src/rl2svgaux.c \
 $(RASTERLITE2_PATH)/src/rl2svgxml.c \
 $(RASTERLITE2_PATH)/src/rl2symbaux.c \
 $(RASTERLITE2_PATH)/src/rl2symbolizer.c \
 $(RASTERLITE2_PATH)/src/rl2symclone.c \
 $(RASTERLITE2_PATH)/src/rl2tiff.c \
 $(RASTERLITE2_PATH)/src/rl2version.c \
 $(RASTERLITE2_PATH)/src/rl2webp.c \
 $(RASTERLITE2_PATH)/src/rl2wms.c
LOCAL_STATIC_LIBRARIES := libcharls libopenjpeg libpng libwebp libxml2 spatialite libfreetype libcairo libcurl libgeotiff libtiff libgif libjpeg liblz4 liblzstd
include $(BUILD_STATIC_LIBRARY)
