cmake_minimum_required(VERSION 3.10)
project(goincpp_ut)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

enable_testing()
include(CTest)

# Enable AddressSanitizer for debug builds
# if(CMAKE_BUILD_TYPE MATCHES Debug)
#     message(STATUS "AddressSanitizer enabled")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-sanitize=undefined")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-sanitize=undefined")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fno-sanitize=undefined")
# endif()

# Print out the build root directory
message("Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message("Build root directory: ${CMAKE_BINARY_DIR}")
set(DPES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../build/deps")
include_directories(${DPES_ROOT})
set(BOOST_ROOT "${DPES_ROOT}/boost")
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
message(STATUS "Boost_VERSION: ${Boost_VERSION}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

set(LIBGOINCPP "${CMAKE_CURRENT_SOURCE_DIR}/../build/libgoincpp.so")

file(GLOB_RECURSE SOURCES0 boost_test.cpp)

foreach(SOURCE ${SOURCES0})
    get_filename_component(EXECUTABLE_NAME ${SOURCE} NAME_WE)
    add_executable(${EXECUTABLE_NAME} ${SOURCE})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE_NAME} Boost::unit_test_framework)

    add_test(NAME ${EXECUTABLE_NAME} COMMAND ${EXECUTABLE_NAME})
endforeach()

file(GLOB_RECURSE SOURCES errors_test.cpp channel_test.cpp context_test.cpp)

foreach(SOURCE ${SOURCES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE} NAME_WE)
    add_executable(${EXECUTABLE_NAME} ${SOURCE})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE_NAME} Boost::unit_test_framework "${LIBGOINCPP}")

    add_test(NAME ${EXECUTABLE_NAME} COMMAND ${EXECUTABLE_NAME})
endforeach()