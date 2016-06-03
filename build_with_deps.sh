#!/bin/bash
git submodule init
git submodule update

cd third_party/pykep
mkdir build
cd build
ccmake ../
make -j8 && sudo make install

cd ../../..

cd third_party/pagmo
mkdir build
cd build
ccmake ../
make -j8 && sudo make install

cd ../../..

cd third_party/libbson
mkdir build
cd build
ccmake ../
make -j8 && sudo make install

cd ../../..

cd third_party/mongo-c-drive
mkdir build
cd build
ccmake ../
make -j8 && sudo make install

cd ../../..

cd third_party/mongo-cxx-driver
mkdir build
cd build
ccmake ../
make -j8 && sudo make install

cd ../../..

mkdir build
cd build
cmake ../
make -j8 && make install
