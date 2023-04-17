#!/bin/bash

#
# This shouldn't be changed since this LK
# is part of the Android 6.0.1 BSP and other
# toolchain(s) will probably cause unexpected
# build issue(s).
#
ANDROID_VERSION="marshmallow-release"

# Make sure we have one target at least.
if [ $# -eq 0 ]; then
  echo "USAGE: '$0 <target>'"
  exit 1
fi

# If we got a target, save it.
TARGET=$1

# If present, delete old build directorie(s) and log(s).
rm -rf build-$TARGET* > /dev/null 2>&1

# If not present, clone the toolchain.
if [ ! -d 'arm-linux-androideabi-4.9' ]; then
  echo "[?] Cloning the toolchain, please wait..."
  git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9 -b $ANDROID_VERSION --depth=1 > /dev/null 2>2&1 # Use depth=1 because we don't care about the history.
fi

# Start the build, use the proper toolchain and the supplied build target.
echo "[?] Building LK for target $1... This may take a while, please wait..."
make TOOLCHAIN_PREFIX=./arm-linux-androideabi-4.9/bin/arm-linux-androideabi- PROJECT:=$TARGET > build-$TARGET.txt 2>&1

# Check if the command executed fine.
if [ ! -f "build-$TARGET/lk.bin" ]; then
  echo "[!] Something went horribly wrong while trying to build for $1. Please check log(s) for more details!"
  exit 1
fi

# If we're here, that means everything succeed.
echo "[+] Successfully built LK for target $1!"
echo "[?] LK image  = $(readlink -f build-$TARGET/lk.bin)"
echo "[?] LK md5sum = $(md5sum build-$TARGET/lk.bin)"
echo "[?] LK size   = $(stat -c%s build-$TARGET/lk.bin)"
