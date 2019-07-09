include $(CLEAR_VARS)
# -------------------
#  libiconv-1.16
# -------------------
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# changes in libcharset/config.h must be made
# -------------------
LOCAL_MODULE := iconv
# -------------------
# As of 2019-04-26
# -------------------
# https://www.gnu.org/software/libiconv/#downloading
# -------------------
# comment out HAVE_LANGINFO_CODESET in config.h, lib/config.h and  libcharset/config.h
LOCAL_CFLAGS := \
        -Wno-multichar \
 -D_ANDROID \
 -DBUILDING_LIBICONV \
 -DIN_LIBRARY \
 -DLIBDIR="\"~/android-libs/usr/local/lib\""

LOCAL_C_INCLUDES := \
 $(ICONV_PATH) \
 $(ICONV_PATH)/include \
 $(ICONV_PATH)/lib \
 $(ICONV_PATH)/libcharset/include

LOCAL_SRC_FILES := \
 $(ICONV_PATH)/lib/iconv.c \
 $(ICONV_PATH)/lib/relocatable.c \
 $(ICONV_PATH)/lib/genaliases.c \
 $(ICONV_PATH)/lib/genaliases2.c \
 $(ICONV_PATH)/lib/genflags.c \
 $(ICONV_PATH)/lib/gentranslit.c \
 $(ICONV_PATH)/libcharset/lib/localcharset.c
include $(BUILD_STATIC_LIBRARY)

