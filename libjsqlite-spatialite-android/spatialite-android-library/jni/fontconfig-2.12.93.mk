include $(CLEAR_VARS)
# -------------------
# fontconfig-2.12.93
# -------------------
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# -------------------
LOCAL_MODULE    := libfontconfig

# -------------------
# As of 2018-02-15
# -------------------
# https://www.freedesktop.org/software/fontconfig/release/
# -------------------

 fontconfig_flags := \
 -DFONTCONFIG_PATH=\"/sdcard/.fcconfig\" \
 -DFC_CACHEDIR=\"/sdcard/.fccache\" \
 -DFC_DEFAULT_FONTS=\"/system/fonts\" \
 -DHAVE_RANDOM=0 \
 -DSIZEOF_VOID_P=4 \
 -DHAVE_CONFIG_H

ifeq ($(TARGET_ARCH),arm)
 fontconfig_flags += -DALIGNOF_DOUBLE=8
else
 fontconfig_flags += -DALIGNOF_DOUBLE=4
endif

# fontconfig-2.11.1/config.h: comment out
# fontconfig-2.12.93/config.h: comment out
# HAVE_RANDOM_R
# HAVE_SYS_STATVFS_H
# uses uuid which is not available


LOCAL_CFLAGS    := \
 $(fontconfig_flags)

LOCAL_C_INCLUDES := \
 $(FONTCONFIG_PATH) \
 $(FREETYPE_PATH)/builds \
 $(FREETYPE_PATH)/include \
 $(EXPAT_PATH)/lib
LOCAL_SRC_FILES := \
 $(FONTCONFIG_PATH)/src/fcarch.h \
 $(FONTCONFIG_PATH)/src/fcatomic.c \
 $(FONTCONFIG_PATH)/src/fccache.c \
 $(FONTCONFIG_PATH)/src/fccfg.c \
 $(FONTCONFIG_PATH)/src/fccharset.c \
 $(FONTCONFIG_PATH)/src/fccompat.c \
 $(FONTCONFIG_PATH)/src/fcdbg.c \
 $(FONTCONFIG_PATH)/src/fcdefault.c \
 $(FONTCONFIG_PATH)/src/fcdir.c \
 $(FONTCONFIG_PATH)/src/fcformat.c \
 $(FONTCONFIG_PATH)/src/fcfreetype.c \
 $(FONTCONFIG_PATH)/src/fcfs.c \
 $(FONTCONFIG_PATH)/src/fcptrlist.c \
 $(FONTCONFIG_PATH)/src/fchash.c \
 $(FONTCONFIG_PATH)/src/fcinit.c \
 $(FONTCONFIG_PATH)/src/fclang.c \
 $(FONTCONFIG_PATH)/src/fclist.c \
 $(FONTCONFIG_PATH)/src/fcmatch.c \
 $(FONTCONFIG_PATH)/src/fcmatrix.c \
 $(FONTCONFIG_PATH)/src/fcname.c \
 $(FONTCONFIG_PATH)/src/fcobjs.c \
 $(FONTCONFIG_PATH)/src/fcpat.c \
 $(FONTCONFIG_PATH)/src/fcrange.c \
 $(FONTCONFIG_PATH)/src/fcserialize.c \
 $(FONTCONFIG_PATH)/src/fcstat.c \
 $(FONTCONFIG_PATH)/src/fcstr.c \
 $(FONTCONFIG_PATH)/src/fcweight.c \
 $(FONTCONFIG_PATH)/src/fcxml.c \
 $(FONTCONFIG_PATH)/src/ftglue.c
LOCAL_STATIC_LIBRARIES := libfreetype libexpat
include $(BUILD_STATIC_LIBRARY)