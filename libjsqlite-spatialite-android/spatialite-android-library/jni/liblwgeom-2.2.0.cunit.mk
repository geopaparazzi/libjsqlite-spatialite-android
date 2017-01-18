include $(CLEAR_VARS)
# In 'postgis-2.2.svn' directory
# ./autogen.sh
# ./configure --without-raster --without-pgconfig --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# configure: error: cannot run test program while cross compiling
## AC_TRY_RUN of configure.ac
# manually added:
# liblwgeom/liblwgeom.h
# postgis_config.h
###
# --with-projdir=
# libjson may be needed
# ljson-c
# -------------------
# As of 2015-10-02
# -------------------
# changes:
# - geos-3.5.0
# - liblwgeom
# -------------------
LOCAL_MODULE    := liblwgeom

# spatialite flags
# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
liblwgeom_flags := \
 -DHAVE_LIBJSON=1 \
 -DTARGET_CPU=\"$(TARGET_ARCH_ABI)\"

LOCAL_CFLAGS    := \
 $(liblwgeom_flags)

LOCAL_C_INCLUDES := \
 $(ICONV_PATH)/include \
 $(ICONV_PATH)/libcharset/include \
 $(GEOS_PATH)/include \
 $(GEOS_PATH)/capi \
 $(LWGEOM_PATH) \
 $(LWGEOM_PATH)/cunit \
 $(PROJ4_PATH)/src
LOCAL_SRC_FILES := \
 $(LWGEOM_PATH)/cunit/cu_algorithm.c \
 $(LWGEOM_PATH)/cunit/cu_buildarea.c \
 $(LWGEOM_PATH)/cunit/cu_bytebuffer.c \
 $(LWGEOM_PATH)/cunit/cu_clean.c \
 $(LWGEOM_PATH)/cunit/cu_clip_by_rect.c \
 $(LWGEOM_PATH)/cunit/cu_effectivearea.c \
 $(LWGEOM_PATH)/cunit/cu_force_sfs.c \
 $(LWGEOM_PATH)/cunit/cu_geodetic.c \
 $(LWGEOM_PATH)/cunit/cu_geos.c \
 $(LWGEOM_PATH)/cunit/cu_geos_cluster.c \
 $(LWGEOM_PATH)/cunit/cu_homogenize.c \
 $(LWGEOM_PATH)/cunit/cu_in_encoded_polyline.c \
 $(LWGEOM_PATH)/cunit/cu_in_geojson.c \
 $(LWGEOM_PATH)/cunit/cu_in_twkb.c \
 $(LWGEOM_PATH)/cunit/cu_in_wkb.c \
 $(LWGEOM_PATH)/cunit/cu_in_wkt.c \
 $(LWGEOM_PATH)/cunit/cu_libgeom.c \
 $(LWGEOM_PATH)/cunit/cu_measures.c \
 $(LWGEOM_PATH)/cunit/cu_misc.c \
 $(LWGEOM_PATH)/cunit/cu_node.c \
 $(LWGEOM_PATH)/cunit/cu_out_encoded_polyline.c \
 $(LWGEOM_PATH)/cunit/cu_out_geojson.c \
 $(LWGEOM_PATH)/cunit/cu_out_gml.c \
 $(LWGEOM_PATH)/cunit/cu_out_kml.c \
 $(LWGEOM_PATH)/cunit/cu_out_svg.c \
 $(LWGEOM_PATH)/cunit/cu_out_twkb.c \
 $(LWGEOM_PATH)/cunit/cu_out_wkb.c \
 $(LWGEOM_PATH)/cunit/cu_out_wkt.c \
 $(LWGEOM_PATH)/cunit/cu_out_x3d.c \
 $(LWGEOM_PATH)/cunit/cu_print.c \
 $(LWGEOM_PATH)/cunit/cu_ptarray.c \
 $(LWGEOM_PATH)/cunit/cu_sfcgal.c \
 $(LWGEOM_PATH)/cunit/cu_split.c \
 $(LWGEOM_PATH)/cunit/cu_stringbuffer.c \
 $(LWGEOM_PATH)/cunit/cu_surface.c \
 $(LWGEOM_PATH)/cunit/cu_tester.c \
 $(LWGEOM_PATH)/cunit/cu_tree.c \
 $(LWGEOM_PATH)/cunit/cu_triangulate.c \
 $(LWGEOM_PATH)/cunit/cu_unionfind.c \
 $(LWGEOM_PATH)/cunit/cu_varint.c \
 $(LWGEOM_PATH)/box2d.c \
 $(LWGEOM_PATH)/bytebuffer.c \
 $(LWGEOM_PATH)/effectivearea.c \
 $(LWGEOM_PATH)/g_box.c \
 $(LWGEOM_PATH)/g_serialized.c \
 $(LWGEOM_PATH)/g_util.c \
 $(LWGEOM_PATH)/lwalgorithm.c \
 $(LWGEOM_PATH)/lwcircstring.c \
 $(LWGEOM_PATH)/lwcollection.c \
 $(LWGEOM_PATH)/lwcompound.c \
 $(LWGEOM_PATH)/lwcurvepoly.c \
 $(LWGEOM_PATH)/lwgeodetic.c \
 $(LWGEOM_PATH)/lwgeodetic_tree.c \
 $(LWGEOM_PATH)/lwgeom_api.c \
 $(LWGEOM_PATH)/lwgeom.c \
 $(LWGEOM_PATH)/lwgeom_debug.c \
 $(LWGEOM_PATH)/lwgeom_geos.c \
 $(LWGEOM_PATH)/lwgeom_geos_clean.c \
 $(LWGEOM_PATH)/lwgeom_geos_cluster.c \
 $(LWGEOM_PATH)/lwgeom_geos_node.c \
 $(LWGEOM_PATH)/lwgeom_geos_split.c \
 $(LWGEOM_PATH)/lwgeom_sfcgal.c \
 $(LWGEOM_PATH)/lwgeom_topo.c \
 $(LWGEOM_PATH)/lwgeom_transform.c \
 $(LWGEOM_PATH)/lwhomogenize.c \
 $(LWGEOM_PATH)/lwin_encoded_polyline.c \
 $(LWGEOM_PATH)/lwin_geojson.c \
 $(LWGEOM_PATH)/lwin_twkb.c \
 $(LWGEOM_PATH)/lwin_wkb.c \
 $(LWGEOM_PATH)/lwin_wkt.c \
 $(LWGEOM_PATH)/lwin_wkt_lex.c \
 $(LWGEOM_PATH)/lwin_wkt_parse.c \
 $(LWGEOM_PATH)/lwlinearreferencing.c \
 $(LWGEOM_PATH)/lwline.c \
 $(LWGEOM_PATH)/lwmcurve.c \
 $(LWGEOM_PATH)/lwmline.c \
 $(LWGEOM_PATH)/lwmpoint.c \
 $(LWGEOM_PATH)/lwmpoly.c \
 $(LWGEOM_PATH)/lwmsurface.c \
 $(LWGEOM_PATH)/lwout_encoded_polyline.c \
 $(LWGEOM_PATH)/lwout_geojson.c \
 $(LWGEOM_PATH)/lwout_gml.c \
 $(LWGEOM_PATH)/lwout_kml.c \
 $(LWGEOM_PATH)/lwout_svg.c \
 $(LWGEOM_PATH)/lwout_twkb.c \
 $(LWGEOM_PATH)/lwout_wkb.c \
 $(LWGEOM_PATH)/lwout_wkt.c \
 $(LWGEOM_PATH)/lwout_x3d.c \
 $(LWGEOM_PATH)/lwpoint.c \
 $(LWGEOM_PATH)/lwpoly.c \
 $(LWGEOM_PATH)/lwprint.c \
 $(LWGEOM_PATH)/lwpsurface.c \
 $(LWGEOM_PATH)/lwspheroid.c \
 $(LWGEOM_PATH)/lwstroke.c \
 $(LWGEOM_PATH)/lwtin.c \
 $(LWGEOM_PATH)/lwtree.c \
 $(LWGEOM_PATH)/lwtriangle.c \
 $(LWGEOM_PATH)/lwunionfind.c \
 $(LWGEOM_PATH)/lwutil.c \
 $(LWGEOM_PATH)/measures3d.c \
 $(LWGEOM_PATH)/measures.c \
 $(LWGEOM_PATH)/ptarray.c \
 $(LWGEOM_PATH)/snprintf.c \
 $(LWGEOM_PATH)/stringbuffer.c \
 $(LWGEOM_PATH)/varint.c
include $(BUILD_STATIC_LIBRARY)
