#!/bin/sh
rm -rf build
mkdir build
cd build
cmake .. -G Xcode
cmake --build . --config Release --target package
