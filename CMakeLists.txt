# Copyright 2024 The Authors. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.10)
project(goincpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

# Print out the build root directory
message("Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message("Build root directory: ${CMAKE_BINARY_DIR}")

# Add all your source files
file(GLOB_RECURSE SOURCES src/*.cpp)
# file(GLOB_RECURSE REMOVE_SOURCES src/context/*.cpp)
# list(REMOVE_ITEM SOURCES ${REMOVE_SOURCES})


add_library(goincpp SHARED ${SOURCES})
target_include_directories(goincpp PUBLIC src)

# Create the shared library
# add_library(mysharedlibrary SHARED ${SOURCES})

# Example: Linking with external libraries (replace with your own)
# target_link_libraries(mysharedlibrary
#     ${EXTERNAL_LIBRARIES}
# )

# Example: Set include directories (replace with your own)
# target_include_directories(mysharedlibrary PUBLIC
#     ${PROJECT_SOURCE_DIR}/include
# )

# Set installation directory for the shared library
# install(TARGETS mysharedlibrary LIBRARY DESTINATION /usr/local/lib)

# Optionally, install header files if needed
# install(FILES ${PROJECT_SOURCE_DIR}/include/your_header.h DESTINATION /usr/local/include)

# Create an executable target from sources
# add_executable(goincpp ${SOURCES})

# Set C++ standard
# set_target_properties(goincpp PROPERTIES CXX_STANDARD 20)