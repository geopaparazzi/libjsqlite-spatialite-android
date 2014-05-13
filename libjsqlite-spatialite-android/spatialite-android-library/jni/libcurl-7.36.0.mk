include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi --disable-manual
LOCAL_MODULE    := libcurl

# make and listed *.0 \ \ files in src ; make clean

curl_flags := \
 -DHAVE_CONFIG_H=1

LOCAL_CFLAGS    := \
	$(curl_flags)

LOCAL_C_INCLUDES := \
 $(CURL_PATH)/include \
 $(CURL_PATH)/lib \
 $(CURL_PATH)
LOCAL_SRC_FILES := \
	$(CURL_PATH)/src/tool_binmode.c \
	$(CURL_PATH)/src/tool_bname.c \
	$(CURL_PATH)/src/tool_cb_dbg.c \
	$(CURL_PATH)/src/tool_cb_hdr.c \
	$(CURL_PATH)/src/tool_cb_prg.c \
	$(CURL_PATH)/src/tool_cb_rea.c \
	$(CURL_PATH)/src/tool_cb_see.c \
	$(CURL_PATH)/src/tool_cb_wrt.c \
	$(CURL_PATH)/src/tool_cfgable.c \
	$(CURL_PATH)/src/tool_convert.c \
	$(CURL_PATH)/src/tool_dirhie.c \
	$(CURL_PATH)/src/tool_doswin.c \
	$(CURL_PATH)/src/tool_easysrc.c \
	$(CURL_PATH)/src/tool_formparse.c \
	$(CURL_PATH)/src/tool_getparam.c \
	$(CURL_PATH)/src/tool_getpass.c \
	$(CURL_PATH)/src/tool_help.c \
	$(CURL_PATH)/src/tool_helpers.c \
	$(CURL_PATH)/src/tool_homedir.c \
	$(CURL_PATH)/src/tool_hugehelp.c \
	$(CURL_PATH)/src/tool_libinfo.c \
	$(CURL_PATH)/src/tool_main.c \
	$(CURL_PATH)/src/tool_metalink.c \
	$(CURL_PATH)/src/tool_mfiles.c \
	$(CURL_PATH)/src/tool_msgs.c \
	$(CURL_PATH)/src/tool_operate.c \
	$(CURL_PATH)/src/tool_operhlp.c \
	$(CURL_PATH)/src/tool_panykey.c \
	$(CURL_PATH)/src/tool_paramhlp.c \
	$(CURL_PATH)/src/tool_parsecfg.c \
	$(CURL_PATH)/src/tool_setopt.c \
	$(CURL_PATH)/src/tool_sleep.c \
	$(CURL_PATH)/src/tool_urlglob.c \
	$(CURL_PATH)/src/tool_util.c \
	$(CURL_PATH)/src/tool_vms.c \
	$(CURL_PATH)/src/tool_writeenv.c \
	$(CURL_PATH)/src/tool_writeout.c \
	$(CURL_PATH)/src/tool_xattr.c
include $(BUILD_STATIC_LIBRARY)



 





