include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
#--------------------------------------
# ./configure --host=arm-linux-androideabi --build=x86_64-pc-linux-gnu  --enable-libxml2
# ./configure --host=aarch64-linux-android --build=x86_64-pc-linux-gnu  --enable-libxml2
# ./configure --host=x86_64-linux-android --build=x86_64-pc-linux-gnu  --enable-libxml2
#--------------------------------------
LOCAL_MODULE    := libfontconfig

 fontconfig_flags := \
 -DFONTCONFIG_PATH=\"/sdcard/.fcconfig\" \
 -DFC_CACHEDIR=\"/sdcard/.fccache\" \
 -DFC_DEFAULT_FONTS=\"/system/fonts\" \
 -DHAVE_RANDOM=0 \
 -DSIZEOF_VOID_P=4 \
 -DHAVE_CONFIG_H

ifeq ($(TARGET_ARCH_ABI),$(filter $(TARGET_ARCH_ABI),armeabi-v7a arm64-v8a x86_64))
 fontconfig_flags += -DALIGNOF_DOUBLE=8
 fontconfig_flags += -DSIZEOF_VOID_P=8
else
 fontconfig_flags += -DALIGNOF_DOUBLE=4
 fontconfig_flags += -DSIZEOF_VOID_P=4
endif

# fontconfig-2.11.1/config.h: comment out
# HAVE_RANDOM_R
# HAVE_SYS_STATVFS_H

LOCAL_CFLAGS    := \
 $(fontconfig_flags)

LOCAL_C_INCLUDES := \
 $(FONTCONFIG_PATH) \
 $(FREETYPE_PATH)/builds \
 $(FREETYPE_PATH)/include \
 $(EXPAT_PATH)/lib
LOCAL_SRC_FILES := \
 $(FONTCONFIG_PATH)/src/fcarch.c \
 $(FONTCONFIG_PATH)/src/fcatomic.c \
 $(FONTCONFIG_PATH)/src/fcblanks.c \
 $(FONTCONFIG_PATH)/src/fccache.c \
 $(FONTCONFIG_PATH)/src/fccfg.c \
 $(FONTCONFIG_PATH)/src/fccharset.c \
 $(FONTCONFIG_PATH)/src/fcdbg.c \
 $(FONTCONFIG_PATH)/src/fcdefault.c \
 $(FONTCONFIG_PATH)/src/fcdir.c \
 $(FONTCONFIG_PATH)/src/fcformat.c \
 $(FONTCONFIG_PATH)/src/fcfreetype.c \
 $(FONTCONFIG_PATH)/src/fcfs.c \
 $(FONTCONFIG_PATH)/src/fchash.c \
 $(FONTCONFIG_PATH)/src/fcinit.c \
 $(FONTCONFIG_PATH)/src/fclang.c \
 $(FONTCONFIG_PATH)/src/fclist.c \
 $(FONTCONFIG_PATH)/src/fcmatch.c \
 $(FONTCONFIG_PATH)/src/fcmatrix.c \
 $(FONTCONFIG_PATH)/src/fcname.c \
 $(FONTCONFIG_PATH)/src/fcpat.c \
 $(FONTCONFIG_PATH)/src/fcserialize.c \
 $(FONTCONFIG_PATH)/src/fcstr.c \
 $(FONTCONFIG_PATH)/src/fcxml.c \
 $(FONTCONFIG_PATH)/src/fccompat.c \
 $(FONTCONFIG_PATH)/src/fcobjs.c \
 $(FONTCONFIG_PATH)/src/fcstat.c \
 $(FONTCONFIG_PATH)/src/ftglue.c
LOCAL_STATIC_LIBRARIES := libfreetype libexpat
include $(BUILD_STATIC_LIBRARY)





