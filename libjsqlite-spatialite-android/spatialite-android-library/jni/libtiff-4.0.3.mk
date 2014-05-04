include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libtiff

# based on sample found here: [https://bitbucket.org/gongminmin/klayge/src/ca44276467e2/External/7z/build/android/LZMA/jni/Android.mk]

LOCAL_CFLAGS    := \
	$(tiff_flags)

LOCAL_C_INCLUDES := \
 $(LZMA_PATH)/api \
 $(JPEG_PATH) \
 $(TIFF_PATH)
LOCAL_SRC_FILES := \
	$(TIFF_PATH)/mkg3states.c \
	$(TIFF_PATH)/tif_aux.c \
	$(TIFF_PATH)/tif_close.c \
	$(TIFF_PATH)/tif_codec.c \
	$(TIFF_PATH)/tif_color.c \
	$(TIFF_PATH)/tif_compress.c \
	$(TIFF_PATH)/tif_dir.c \
	$(TIFF_PATH)/tif_dirinfo.c \
	$(TIFF_PATH)/tif_dirread.c \
	$(TIFF_PATH)/tif_dirwrite.c \
	$(TIFF_PATH)/tif_dumpmode.c \
	$(TIFF_PATH)/tif_error.c \
	$(TIFF_PATH)/tif_extension.c \
	$(TIFF_PATH)/tif_fax3.c \
	$(TIFF_PATH)/tif_fax3sm.c \
	$(TIFF_PATH)/tif_flush.c \
	$(TIFF_PATH)/tif_getimage.c \
	$(TIFF_PATH)/tif_jbig.c \
	$(TIFF_PATH)/tif_jpeg_12.c \
	$(TIFF_PATH)/tif_jpeg.c \
	$(TIFF_PATH)/tif_luv.c \
	$(TIFF_PATH)/tif_lzma.c \
	$(TIFF_PATH)/tif_lzw.c \
	$(TIFF_PATH)/tif_next.c \
	$(TIFF_PATH)/tif_ojpeg.c \
	$(TIFF_PATH)/tif_open.c \
	$(TIFF_PATH)/tif_packbits.c \
	$(TIFF_PATH)/tif_pixarlog.c \
	$(TIFF_PATH)/tif_predict.c \
	$(TIFF_PATH)/tif_print.c \
	$(TIFF_PATH)/tif_read.c \
	$(TIFF_PATH)/tif_strip.c \
	$(TIFF_PATH)/tif_swab.c \
	$(TIFF_PATH)/tif_thunder.c \
	$(TIFF_PATH)/tif_tile.c \
	$(TIFF_PATH)/tif_unix.c \
	$(TIFF_PATH)/tif_version.c \
	$(TIFF_PATH)/tif_warning.c \
	$(TIFF_PATH)/tif_win32.c \
	$(TIFF_PATH)/tif_write.c \
	$(TIFF_PATH)/tif_zip.c 
LOCAL_STATIC_LIBRARIES := libjpeg
include $(BUILD_STATIC_LIBRARY)
