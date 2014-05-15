include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libfreetype

# taken from make file am__libxml2_la_SOURCES_DIST

 freetype_flags := \
 -DDARWIN_NO_CARBON \
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
	$(FREETYPE_PATH)/src/base/ftbbox.c \
	$(FREETYPE_PATH)/src/base/ftbitmap.c \
	$(FREETYPE_PATH)/src/base/ftglyph.c \
	$(FREETYPE_PATH)/src/base/ftstroke.c \
	$(FREETYPE_PATH)/src/base/ftxf86.c \
	$(FREETYPE_PATH)/src/base/ftbase.c \
	$(FREETYPE_PATH)/src/base/ftsystem.c \
	$(FREETYPE_PATH)/src/base/ftinit.c \
	$(FREETYPE_PATH)/src/base/ftgasp.c \
	$(FREETYPE_PATH)/src/raster/raster.c \
	$(FREETYPE_PATH)/src/sfnt/sfnt.c \
	$(FREETYPE_PATH)/src/smooth/smooth.c \
	$(FREETYPE_PATH)/src/autofit/autofit.c \
	$(FREETYPE_PATH)/src/truetype/truetype.c \
	$(FREETYPE_PATH)/src/cff/cff.c \
	$(FREETYPE_PATH)/src/psnames/psnames.c \
	$(FREETYPE_PATH)/src/pshinter/pshinter.c
include $(BUILD_STATIC_LIBRARY)




