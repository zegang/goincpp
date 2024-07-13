#!/bin/bash

# Copyright 2024 The Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Build Boost
# git submodule update --init
# ./bootstrap.sh

# Build Goincpp
# docker build -t goincpp .
# docker run -v .:/goincpp -it --rm --name goincpp goincpp:latest
# docker run -v .:/goincpp -it --name goincpp goincpp:latest
cmake -S . -B build;cmake --build build

# Do Tests
cd test; cmake -S . -B build;cmake --build build
cd build; ctest -V
# ctest -N; ctest -R boost_test
# valgrind --leak-check=full --trace-children=yes ctest
# valgrind --trace-children=yes ctest