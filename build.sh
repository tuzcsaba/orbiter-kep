#!/bin/bash
mkdir build
cd build
cmake ../
make -j8 && make install
