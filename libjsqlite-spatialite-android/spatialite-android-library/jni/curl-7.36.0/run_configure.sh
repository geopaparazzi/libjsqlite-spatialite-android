#!/bin/bash
# the path to the cross compiler
# $ANDROID_BASE=/home/mj10777/000_links/gnu_source/adt-bundle-linux/
export CC=$ANDROID_BASE/android-ndk-r9d/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc

#the path to the ndk
export SYSROOT=$ANDROID_BASE/android-ndk-r9d/platforms/android-18/arch-arm

export CFLAGS="-march=armv5te -mtune=xscale -msoft-float -mandroid -fPIC -mthumb-interwork -mthumb -mlong-calls -ffunction-sections -fstack-protector -fno-short-enums -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 "

export LDFLAGS="-L$SYSROOT/usr/lib -Wl,--gc-sections -nostdlib -Wl,--no-undefined,-z,nocopyreloc -Wl,-dynamic-linker,/system/bin/linker "

# configure:3576: LDFLAGS note: LDFLAGS should only be used to specify linker flags, not libraries. Use LIBS for:
export LIBS="-lc -ldl -llog -lgcc -lm"
# configure: CFLAGS note: CFLAGS should only be used to specify C compiler flags, not macro definitions. Use CPPFLAGS for:
export CPPFLAGS="-I$SYSROOT/usr/include -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -DANDROID -DOS_ANDROID -D__NEW__ -D__SGI_STL_INTERNAL_PAIR_H "

# NOTE: in my case I wanted a static lib and I specifically did not want
# docs or zip support so I added these 2 flags to the configure command:

./configure --host=arm-linux-eabi --without-zlib --disable-manual

# make showcommands

