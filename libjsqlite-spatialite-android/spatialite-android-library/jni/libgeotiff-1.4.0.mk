include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libgeotiff

# based on sample found here: [https://bitbucket.org/gongminmin/klayge/src/ca44276467e2/External/7z/build/android/LZMA/jni/Android.mk]
geotiff_flags := \
        -DHAVE_LIBPROJ=1 \
        -DHAVE_TIFF=1

LOCAL_CFLAGS    := \
	$(geotiff_flags)

LOCAL_C_INCLUDES := \
	$(PROJ4_PATH)/src \
	$(GEOTIFF_PATH)/libxtiff \
 $(GEOTIFF_PATH) \
 $(TIFF_PATH)
LOCAL_SRC_FILES := \
	$(GEOTIFF_PATH)/libxtiff/xtiff.c \
	$(GEOTIFF_PATH)/cpl_csv.c \
	$(GEOTIFF_PATH)/cpl_serv.c \
	$(GEOTIFF_PATH)/geo_extra.c \
	$(GEOTIFF_PATH)/geo_free.c \
	$(GEOTIFF_PATH)/geo_get.c \
	$(GEOTIFF_PATH)/geo_names.c \
	$(GEOTIFF_PATH)/geo_new.c \
	$(GEOTIFF_PATH)/geo_normalize.c \
	$(GEOTIFF_PATH)/geo_print.c \
	$(GEOTIFF_PATH)/geo_set.c \
	$(GEOTIFF_PATH)/geo_simpletags.c \
	$(GEOTIFF_PATH)/geo_tiffp.c \
	$(GEOTIFF_PATH)/geotiff_proj4.c \
	$(GEOTIFF_PATH)/geo_trans.c \
	$(GEOTIFF_PATH)/geo_write.c \
	$(GEOTIFF_PATH)/csv/datum.c \
	$(GEOTIFF_PATH)/csv/ellipsoid.c \
	$(GEOTIFF_PATH)/csv/gcs.c \
	$(GEOTIFF_PATH)/csv/pcs.c \
	$(GEOTIFF_PATH)/csv/prime_meridian.c \
	$(GEOTIFF_PATH)/csv/projop_wparm.c  \
	$(GEOTIFF_PATH)/csv/unit_of_measure.c 
LOCAL_STATIC_LIBRARIES := liblzma libtiff proj
include $(BUILD_STATIC_LIBRARY)



