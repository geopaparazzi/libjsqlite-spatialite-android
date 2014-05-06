include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := rasterlite2

# SQLite flags copied from ASOP [may not be needed for rasterlite2]
common_sqlite_flags := \
        -DHAVE_USLEEP=1 \
        -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
        -DSQLITE_THREADSAFE=1 \
        -DNDEBUG=1 \
        -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
        -DSQLITE_DEFAULT_AUTOVACUUM=1 \
        -DSQLITE_TEMP_STORE=3 \
        -DSQLITE_ENABLE_FTS3 \
        -DSQLITE_ENABLE_FTS3_BACKWARDS \
        -DSQLITE_ENABLE_RTREE=1 \
        -DSQLITE_DEFAULT_FILE_FORMAT=4


# spatialite flags [may not be needed for rasterlite2]
spatialite_flags := \
	-DOMIT_FREEXL \
        -DVERSION=\"4.2.0\" \
        -Dfdatasync=fsync \
        -DSQLITE_ENABLE_RTREE=1 \
        -DSQLITE_OMIT_BUILTIN_TEST=1

rasterlite2_flags := \
 -DTARGET_CPU=\"$(ANDROID_ABI)\" \
	-O 

LOCAL_CFLAGS    := \
	$(common_sqlite_flags) \
	$(spatialite_flags) \
	$(rasterlite2_flags)

LOCAL_C_INCLUDES := \
	$(SQLITE_PATH) \
	$(GEOTIFF_PATH)/libxtiff \
 $(GEOTIFF_PATH) \
 $(TIFF_PATH) \
 $(JPEG_PATH) \
 $(GIF_PATH) \
 $(PNG_PATH) \
 $(WEBP_PATH) \
 $(CAIRO_PATH) \
 $(ICONV_PATH)/include \
 $(ICONV_PATH)/libcharset/include \
 $(XML2_PATH)/include \
 $(CURL_PATH) \
 $(CURL_PATH)/include \
	$(RASTERLITE2_PATH) \
	$(RASTERLITE2_PATH)/headers \
	$(SPATIALITE_PATH)/src/headers \
 $(LZMA_PATH)/src/liblzma/api 
LOCAL_SRC_FILES := \
	$(RASTERLITE2_PATH)/src/rasterlite2.c \
	$(RASTERLITE2_PATH)/src/rl2ascii.c \
	$(RASTERLITE2_PATH)/src/rl2codec.c \
	$(RASTERLITE2_PATH)/src/rl2dbms.c \
	$(RASTERLITE2_PATH)/src/rl2gif.c \
	$(RASTERLITE2_PATH)/src/rl2import.c \
	$(RASTERLITE2_PATH)/src/rl2jpeg.c \
	$(RASTERLITE2_PATH)/src/rl2paint.c \
	$(RASTERLITE2_PATH)/src/rl2png.c \
	$(RASTERLITE2_PATH)/src/rl2rastersym.c \
	$(RASTERLITE2_PATH)/src/rl2raw.c \
	$(RASTERLITE2_PATH)/src/rl2sqlaux.c \
	$(RASTERLITE2_PATH)/src/rl2sql.c \
	$(RASTERLITE2_PATH)/src/rl2symbolizer.c \
	$(RASTERLITE2_PATH)/src/rl2tiff.c \
	$(RASTERLITE2_PATH)/src/rl2webp.c \
	$(RASTERLITE2_PATH)/src/rl2version.c \
	$(RASTERLITE2_PATH)/src/rl2wms.c
LOCAL_STATIC_LIBRARIES := liblzma libgeotiff giflib libcairo libpng libxml2 libwebp libcurl
include $(BUILD_STATIC_LIBRARY)
