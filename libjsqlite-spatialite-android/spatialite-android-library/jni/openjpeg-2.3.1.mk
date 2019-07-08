include $(CLEAR_VARS)
# -------------------
# openjpeg-2.3.1
# -------------------
# Taken from http://stackoverflow.com/questions/4036191/sources-from-subdirectories-in-makefile
# Make does not offer a recursive wildcard function, so here's one:
# The trailing slash is required.
# mj10777: 20141003: http://stackoverflow.com/questions/12575534/build-openjpeg-for-androi
# -------------------
# As of 2019-04-02
# -------------------
# https://www.openjpeg.org/
# https://github.com/uclouvain/openjpeg/releases/tag/v2.3.1
# -------------------
# copy from ccmake created:
# 'opj_config.h' and 'opj_config_private.h'
# to src/lib/openjp2 directory
# -------------------
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

ALL_CPPS := $(call rwildcard,$(OPENJPEG_PATH)/src/lib/openjp2,*.c)
ALL_CPPS += $(call rwildcard,$(OPENJPEG_PATH)/src/lib/openjpip,*.c)

LOCAL_MODULE    := libopenjpeg

openjpeg_flags := \
 -DUSE_JPIP

LOCAL_CFLAGS    := \
 $(openjpeg_flags)

LOCAL_C_INCLUDES := \
 $(OPENJPEG_PATH)/src/lib/openjp2

LOCAL_SRC_FILES := \
$(ALL_CPPS)

include $(BUILD_STATIC_LIBRARY)

