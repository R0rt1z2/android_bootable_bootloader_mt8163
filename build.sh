#!/bin/bash

ANDROID_VERSION="marshmallow-release"

if [ $# -eq 0 ]
  then
    echo "[-] No arguments supplied"
fi

echo "[?] Building for $1"
rm -rf build-$1 >/dev/null 2>&1
[ -d "arm-linux-androideabi-4.9" ] || git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/ -b $ANDROID_VERSION --depth=1
make TOOLCHAIN_PREFIX=./arm-linux-androideabi-4.9/bin/arm-linux-androideabi- PROJECT:=$1 | tee ../logs/build-log-$(date +%y%m%d%H%M)

echo "[+] All jobs done"
exit 0
