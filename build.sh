#!bin/bash

rm -rf build
rm -f output.txt
rm -f translate_output.txt
mkdir build
cd build
cmake ..
make
