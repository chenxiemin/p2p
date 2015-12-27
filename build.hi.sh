#! /bin/sh

rm -rf ./build.hi
mkdir ./build.hi

cd ./build.hi
cmake -G "Unix Makefiles" -DCROSS_COMPILE_HI=1 ../
make -j4

