include $(CLEAR_VARS)
# librttopo-20180125 directory
# ./autogen.sh
# ./configure  --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# configure: error: cannot run test program while cross compiling
## AC_TRY_RUN of configure.ac
# manually added:
# nothing
###
# --with-projdir=
# libjson may be needed
# ljson-c
# -------------------
# As of 2018-01-25
# -------------------
# changes:
# - geos-3.6.0
# - librttopo
# HAVE_LIBJSON_C
# must not be defined
# -------------------
LOCAL_MODULE    := librttopo

# spatialite flags
# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
librttopo_flags := \
# -DHAVE_LIBJSON_C=1 \
 -DTARGET_CPU=\"$(TARGET_ARCH_ABI)\"

LOCAL_CFLAGS    := \
 $(librttopo_flags)

LOCAL_C_INCLUDES := \
 $(ICONV_PATH)/include \
 $(ICONV_PATH)/libcharset/include \
 $(GEOS_PATH)/include \
 $(GEOS_PATH)/capi \
 $(JSONC_PATH) \
 $(RTTOPO_PATH) \
 $(RTTOPO_PATH)/headers \
 $(PROJ4_PATH)/src
LOCAL_SRC_FILES := \
 $(RTTOPO_PATH)/src/box2d.c \
 $(RTTOPO_PATH)/src/bytebuffer.c \
 $(RTTOPO_PATH)/src/g_box.c \
 $(RTTOPO_PATH)/src/g_serialized.c \
 $(RTTOPO_PATH)/src/g_util.c \
 $(RTTOPO_PATH)/src/measures.c \
 $(RTTOPO_PATH)/src/measures3d.c \
 $(RTTOPO_PATH)/src/ptarray.c \
 $(RTTOPO_PATH)/src/rtalgorithm.c \
 $(RTTOPO_PATH)/src/rtcircstring.c \
 $(RTTOPO_PATH)/src/rtcollection.c \
 $(RTTOPO_PATH)/src/rtcompound.c \
 $(RTTOPO_PATH)/src/rtcurvepoly.c \
 $(RTTOPO_PATH)/src/rtgeodetic.c \
 $(RTTOPO_PATH)/src/rtgeom.c \
 $(RTTOPO_PATH)/src/rtgeom_api.c \
 $(RTTOPO_PATH)/src/rtgeom_debug.c \
 $(RTTOPO_PATH)/src/rtgeom_geos.c \
 $(RTTOPO_PATH)/src/rtgeom_geos_clean.c \
 $(RTTOPO_PATH)/src/rtgeom_geos_node.c \
 $(RTTOPO_PATH)/src/rtgeom_geos_split.c \
 $(RTTOPO_PATH)/src/rtgeom_topo.c \
 $(RTTOPO_PATH)/src/rthomogenize.c \
 $(RTTOPO_PATH)/src/rtin_geojson.c \
 $(RTTOPO_PATH)/src/rtin_twkb.c \
 $(RTTOPO_PATH)/src/rtin_wkb.c \
 $(RTTOPO_PATH)/src/rtiterator.c \
 $(RTTOPO_PATH)/src/rtline.c \
 $(RTTOPO_PATH)/src/rtlinearreferencing.c \
 $(RTTOPO_PATH)/src/rtmcurve.c \
 $(RTTOPO_PATH)/src/rtmline.c \
 $(RTTOPO_PATH)/src/rtmpoint.c \
 $(RTTOPO_PATH)/src/rtmpoly.c \
 $(RTTOPO_PATH)/src/rtmsurface.c \
 $(RTTOPO_PATH)/src/rtout_encoded_polyline.c \
 $(RTTOPO_PATH)/src/rtout_geojson.c \
 $(RTTOPO_PATH)/src/rtout_gml.c \
 $(RTTOPO_PATH)/src/rtout_kml.c \
 $(RTTOPO_PATH)/src/rtout_svg.c \
 $(RTTOPO_PATH)/src/rtout_twkb.c \
 $(RTTOPO_PATH)/src/rtout_wkb.c \
 $(RTTOPO_PATH)/src/rtout_wkt.c \
 $(RTTOPO_PATH)/src/rtout_x3d.c \
 $(RTTOPO_PATH)/src/rtpoint.c \
 $(RTTOPO_PATH)/src/rtpoly.c \
 $(RTTOPO_PATH)/src/rtprint.c \
 $(RTTOPO_PATH)/src/rtpsurface.c \
 $(RTTOPO_PATH)/src/rtspheroid.c \
 $(RTTOPO_PATH)/src/rtstroke.c \
 $(RTTOPO_PATH)/src/rttin.c \
 $(RTTOPO_PATH)/src/rttree.c \
 $(RTTOPO_PATH)/src/rttriangle.c \
 $(RTTOPO_PATH)/src/rtt_tpsnap.c \
 $(RTTOPO_PATH)/src/rtutil.c \
 $(RTTOPO_PATH)/src/stringbuffer.c \
 $(RTTOPO_PATH)/src/varint.c
include $(BUILD_STATIC_LIBRARY)
