#!/bin/bash

clang++ -g -O3 language/*.cpp *.cpp `llvm-config --cxxflags --link-static --ldflags --system-libs --libs all` -fstack-protector -lssp -frtti -std=c++20 -static -o mascal
