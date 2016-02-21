#! /bin/sh

rm -rf ./build.linux
mkdir ./build.linux

cd ./build.linux
cmake -G "Unix Makefiles" ../
make -j4

