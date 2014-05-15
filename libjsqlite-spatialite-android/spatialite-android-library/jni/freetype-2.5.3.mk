include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libfreetype

# taken from make file am__libxml2_la_SOURCES_DIST

 freetype_flags := \
 -DDARWIN_NO_CARBON \
 -DFT2_BUILD_LIBRARY \
	-fPIC \
 -DPIC

freetype_flags    := \
	$(freetype_flags_flags)

LOCAL_C_INCLUDES := \
 $(FREETYPE_PATH)/builds \
 $(FREETYPE_PATH)/include \
 $(PNG_PATH) \
LOCAL_SRC_FILES := \
	$(FREETYPE_PATH)/base/ftbbox.c \
	$(FREETYPE_PATH)/base/ftbitmap.c \
	$(FREETYPE_PATH)/base/ftglyph.c \
	$(FREETYPE_PATH)/base/ftstroke.c \
	$(FREETYPE_PATH)/base/ftxf86.c \
	$(FREETYPE_PATH)/base/ftbase.c \
	$(FREETYPE_PATH)/base/ftsystem.c \
	$(FREETYPE_PATH)/base/ftinit.c \
	$(FREETYPE_PATH)/base/ftgasp.c \
	$(FREETYPE_PATH)/raster/raster.c \
	$(FREETYPE_PATH)/sfnt/sfnt.c \
	$(FREETYPE_PATH)/smooth/smooth.c \
	$(FREETYPE_PATH)/autofit/autofit.c \
	$(FREETYPE_PATH)/truetype/truetype.c \
	$(FREETYPE_PATH)/cff/cff.c \
	$(FREETYPE_PATH)/psnames/psnames.c \
	$(FREETYPE_PATH)/pshinter/pshinter.c
LOCAL_STATIC_LIBRARIES := libpng
include $(BUILD_STATIC_LIBRARY)




