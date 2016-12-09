#!/bin/bash
echo 'ANDROID_NDK' $ANDROID_NDK

TOOLCHAIN=/tmp/toolchain
SYSROOT=$TOOLCHAIN/sysroot/
if [ ! -d $TOOLCHAIN ]; then
  $ANDROID_NDK/build/tools/make-standalone-toolchain.sh --platform=$PLATFORM --install-dir=$TOOLCHAIN
fi

export PATH=$TOOLCHAIN/bin:$PATH
# export CC=arm-linux-androideabi-gcc
export CC=arm-linux-androideabi-g++
export LD=arm-linux-androideabi-ld
export AR=arm-linux-androideabi-ar

make clean
make