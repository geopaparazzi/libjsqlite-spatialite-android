include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libfontconfig

# taken from make file am__libxml2_la_SOURCES_DIST

 fontconfig_flags := 

LOCAL_CFLAGS    := \
	$(fontconfig_flags)

LOCAL_C_INCLUDES := \
 $(FONTCONFIG_PATH) \
 $(EXPAT_PATH)/lib 
LOCAL_SRC_FILES := \
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
LOCAL_STATIC_LIBRARIES := libexpat
include $(BUILD_STATIC_LIBRARY)






