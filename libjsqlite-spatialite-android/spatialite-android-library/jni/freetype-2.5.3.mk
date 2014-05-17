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
 $(PNG_PATH) \
LOCAL_SRC_FILES := \
	$(FREETYPE_PATH)/base/ftsystem.c \
	$(FREETYPE_PATH)/base/ftinit.c \
	$(FREETYPE_PATH)/base/ftdebug.c \
	$(FREETYPE_PATH)/base/ftbase.c \
	$(FREETYPE_PATH)/base/ftbbox.c \
	$(FREETYPE_PATH)/base/ftglyph.c \
	$(FREETYPE_PATH)/base/ftbitmap.c \
	$(FREETYPE_PATH)/base/ftfstype.c \
	$(FREETYPE_PATH)/base/ftgasp.c \
	$(FREETYPE_PATH)/base/ftlcdfil.c \
	$(FREETYPE_PATH)/base/ftmm.c \
	$(FREETYPE_PATH)/base/ftobjs.c \
	$(FREETYPE_PATH)/base/ftpatent.c \
	$(FREETYPE_PATH)/base/ftstroke.c \
	$(FREETYPE_PATH)/base/ftsynth.c \
	$(FREETYPE_PATH)/base/fttype1.c \
	$(FREETYPE_PATH)/base/ftwinfnt.c \
	$(FREETYPE_PATH)/base/ftxf86.c \
	$(FREETYPE_PATH)/cff/cff.c \
	$(FREETYPE_PATH)/sfnt/sfnt.c \
	$(FREETYPE_PATH)/truetype/truetype.c \
	$(FREETYPE_PATH)/raster/raster.c \
	$(FREETYPE_PATH)/smooth/smooth.c \
	$(FREETYPE_PATH)/autofit/autofit.c \
	$(FREETYPE_PATH)/pshinter/pshinter.c \
	$(FREETYPE_PATH)/psnames/psnames.c
LOCAL_STATIC_LIBRARIES := libpng
include $(BUILD_STATIC_LIBRARY)






