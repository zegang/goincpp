#!/bin/bash

CURRENTPATH=`pwd`
echo "Current Path: $CURRENTPATH"

# Build Boost
# git submodule update --init
# ./bootstrap.sh

# Build Goincpp
# docker build -t goincpp .
# docker run -v .:/goincpp -it --rm --name goincpp goincpp:latest
# docker run -v .:/goincpp -it --name goincpp goincpp:latest
# podman run goincpp
# podman attach goincpp
echo -e "\n\033[47;30m-----------Build Source Codes----------\033[0m"
cmake -S . -B build;cmake --build build

# Do Tests
echo -e "\n\033[47;30m-----------Build Test Codes----------\033[0m"
cd test; cmake -S . -B build;cmake --build build
echo -e "\n\033[47;30m-----------Do Tests----------\033[0m"
cd build; ctest $*
# ctest -N; ctest -R boost_test
# valgrind --leak-check=full --trace-children=yes ctest
# valgrind --trace-children=yes ctest
cd $CURRENTPATH