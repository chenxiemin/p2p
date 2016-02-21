#!/bin/sh

rm -rf ./build.android
mkdir build.android
cd build.android

cmake -D CMAKE_TOOLCHAIN_FILE=../android.cmake -DCROSS_COMPILE_ANDROID=1 ../

