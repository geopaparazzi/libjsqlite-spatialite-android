APP_STL := gnustl_static
APP_CPPFLAGS += -frtti
APP_CPPFLAGS += -fexceptions
APP_PLATFORM     := android-21
# Enable c++11 extentions in source code for PROJ Version >= 6.0
APP_CPPFLAGS += -std=c++11
# APP_ABI := armeabi
# APP_ABI := armeabi-v7a
# APP_ABI := arm64-v8a
# APP_ABI := x86
# APP_ABI := x86_64
# APP_ABI:= armeabi-v7a arm64-v8a x86_64
APP_ABI:= armeabi-v7a arm64-v8a x86_64
