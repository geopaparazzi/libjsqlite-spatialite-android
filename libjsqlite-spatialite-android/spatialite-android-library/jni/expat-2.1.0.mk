include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libexpat

# taken from make file am__libxml2_la_SOURCES_DIST

 expat_flags := \
 -DHAVE_EXPAT_CONFIG_H

LOCAL_CFLAGS    := \
	$(expat_flags)

LOCAL_C_INCLUDES := \
 $(EXPAT_PATH)/lib \
 $(EXPAT_PATH)
LOCAL_SRC_FILES := \
	$(EXPAT_PATH)/lib/xmlparse.c \ \
	$(EXPAT_PATH)/lib/xmlrole.c \
	$(EXPAT_PATH)/lib/xmltok.c
include $(BUILD_STATIC_LIBRARY)

	






