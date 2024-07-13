#!/bin/bash

# Copyright 2024 The Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

ROOT_DIR=`pwd`
CXXSTD="c++20"
CXXFLAGS="-O0 -g -std=${CXXSTD}"

############# Boost ##############
cd ${ROOT_DIR}
BOOST_SRC="${ROOT_DIR}/deps/boost"
BOOST_INSTALL_DIR="${ROOT_DIR}/build/deps/boost"
rm -rf ${BOOST_INSTALL_DIR}
cd ${BOOST_SRC}
# Bootstrap Boost.Build
./bootstrap.sh --prefix=${BOOST_INSTALL_DIR}
# Build Boost libraries (adjust --with options as needed)
./b2 --prefix=${BOOST_INSTALL_DIR} --with-unit_test_framework cxxflags=${CXXFLAGS}
# ./b2 --prefix=${INSTALL_DIR} --with-program_options link=static runtime-link=static
# Install Boost libraries
./b2 install