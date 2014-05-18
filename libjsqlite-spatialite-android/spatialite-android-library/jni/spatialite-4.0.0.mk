include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := spatialite

# SQLite flags copied from ASOP
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
 -DSQLITE_DEFAULT_FILE_FORMAT=4

# spatialite flags
spatialite_flags := \
 -DOMIT_FREEXL \
 -DVERSION=\"4.0.0\" \
 -Dfdatasync=fsync \
 -DSQLITE_ENABLE_RTREE=1 \
 -DSQLITE_OMIT_BUILTIN_TEST=1

LOCAL_CFLAGS    := \
	 $(common_sqlite_flags) \
 $(spatialite_flags)

LOCAL_LDLIBS    := -llog
LOCAL_C_INCLUDES := \
 $(SQLITE_PATH) \
 $(SPATIALITE_PATH) \
 $(SPATIALITE_PATH)/src/headers \
 $(ICONV_PATH)/include \
 $(ICONV_PATH)/libcharset/include \
 $(GEOS_PATH)/include \
 $(GEOS_PATH)/capi \
 $(PROJ4_PATH)/src
LOCAL_SRC_FILES := \
 $(SPATIALITE_PATH)/src/spatialite/mbrcache.c \
 $(SPATIALITE_PATH)/src/spatialite/metatables.c \
 $(SPATIALITE_PATH)/src/spatialite/spatialite.c \
 $(SPATIALITE_PATH)/src/spatialite/statistics.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualdbf.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualfdo.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualnetwork.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualshape.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualspatialindex.c \
 $(SPATIALITE_PATH)/src/spatialite/virtualXL.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_advanced.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_endian.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_geodesic.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_geometries.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_geoscvt.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_relations.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_lwgeom.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_extras.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_shape.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_transform.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_wkb.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_wkt.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_vanuatu.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_ewkt.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_geoJSON.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_kml.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_gml.c \
 $(SPATIALITE_PATH)/src/gaiageo/gg_voronoj.c \
 $(SPATIALITE_PATH)/src/gaiaaux/gg_sqlaux.c \
 $(SPATIALITE_PATH)/src/gaiaaux/gg_utf8.c \
 $(SPATIALITE_PATH)/src/gaiaexif/gaia_exif.c \
 $(SPATIALITE_PATH)/src/srsinit/srs_init.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_00.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_01.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_02.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_03.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_04.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_05.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_06.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_07.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_08.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_09.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_10.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_11.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_12.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_13.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_14.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_15.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_16.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_17.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_18.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_19.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_20.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_21.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_22.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_23.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_24.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_25.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_26.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_27.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_28.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_29.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_30.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_31.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_32.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_33.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_34.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_35.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_36.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_37.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_38.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_39.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_40.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_wgs84_00.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_wgs84_01.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_prussian.c \
 $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_extra.c \
 $(SPATIALITE_PATH)/src/versioninfo/version.c

LOCAL_STATIC_LIBRARIES :=  iconv proj geos

include $(BUILD_STATIC_LIBRARY)
