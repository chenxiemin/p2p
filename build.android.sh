#! /bin/sh

rm -rf ./build.android
mkdir ./build.android

cd ./build.android
cmake -G "Unix Makefiles" -DCROSS_COMPILE_ANDROID=1 ../
make -j4

