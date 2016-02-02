rmdir /s /q build
mkdir build
cd build
cmake .. -G "Visual Studio 14 2015"
cmake --build . --config Release --target package
