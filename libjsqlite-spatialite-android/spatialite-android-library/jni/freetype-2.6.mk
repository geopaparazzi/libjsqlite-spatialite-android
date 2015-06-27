include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libfreetype

# taken from make file am__libxml2_la_SOURCES_DIST

 freetype_flags := \
 -std=gnu99 \
 -DFT2_BUILD_LIBRARY \
 -fPIC \
 -DPIC

LOCAL_CFLAGS    := \
 $(freetype_flags)

LOCAL_C_INCLUDES := \
 $(FREETYPE_PATH)/builds \
 $(FREETYPE_PATH)/include \
 $(PNG_PATH)
LOCAL_SRC_FILES := \
 $(FREETYPE_PATH)/src/autofit/autofit.c \
 $(FREETYPE_PATH)/src/base/ftbase.c \
 $(FREETYPE_PATH)/src/base/ftbbox.c \
 $(FREETYPE_PATH)/src/base/ftbdf.c \
 $(FREETYPE_PATH)/src/base/ftbitmap.c \
 $(FREETYPE_PATH)/src/base/ftcid.c \
 $(FREETYPE_PATH)/src/base/ftdebug.c \
 $(FREETYPE_PATH)/src/base/ftfntfmt.c \
 $(FREETYPE_PATH)/src/base/ftfstype.c \
 $(FREETYPE_PATH)/src/base/ftgasp.c \
 $(FREETYPE_PATH)/src/base/ftglyph.c \
 $(FREETYPE_PATH)/src/base/ftgxval.c \
 $(FREETYPE_PATH)/src/base/ftinit.c \
 $(FREETYPE_PATH)/src/base/ftlcdfil.c \
 $(FREETYPE_PATH)/src/base/ftmm.c \
 $(FREETYPE_PATH)/src/base/ftotval.c \
 $(FREETYPE_PATH)/src/base/ftpatent.c \
 $(FREETYPE_PATH)/src/base/ftpfr.c \
 $(FREETYPE_PATH)/src/base/ftstroke.c \
 $(FREETYPE_PATH)/src/base/ftsynth.c \
 $(FREETYPE_PATH)/src/base/ftsystem.c \
 $(FREETYPE_PATH)/src/base/fttype1.c \
 $(FREETYPE_PATH)/src/base/ftwinfnt.c \
 $(FREETYPE_PATH)/src/bdf/bdf.c \
 $(FREETYPE_PATH)/src/bzip2/ftbzip2.c \
 $(FREETYPE_PATH)/src/cache/ftcache.c \
 $(FREETYPE_PATH)/src/cff/cff.c \
 $(FREETYPE_PATH)/src/cid/type1cid.c \
 $(FREETYPE_PATH)/src/gzip/ftgzip.c \
 $(FREETYPE_PATH)/src/lzw/ftlzw.c \
 $(FREETYPE_PATH)/src/pcf/pcf.c \
 $(FREETYPE_PATH)/src/pfr/pfr.c \
 $(FREETYPE_PATH)/src/psaux/psaux.c \
 $(FREETYPE_PATH)/src/pshinter/pshinter.c \
 $(FREETYPE_PATH)/src/psnames/psnames.c \
 $(FREETYPE_PATH)/src/raster/raster.c \
 $(FREETYPE_PATH)/src/sfnt/sfnt.c \
 $(FREETYPE_PATH)/src/smooth/smooth.c \
 $(FREETYPE_PATH)/src/tools/apinames.c \
 $(FREETYPE_PATH)/src/truetype/truetype.c \
 $(FREETYPE_PATH)/src/type1/type1.c \
 $(FREETYPE_PATH)/src/type42/type42.c \
 $(FREETYPE_PATH)/src/winfonts/winfnt.c
LOCAL_STATIC_LIBRARIES := libpng
include $(BUILD_STATIC_LIBRARY)








