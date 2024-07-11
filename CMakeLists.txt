cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(fastpng VERSION 1.0.0)

# source files
file(GLOB_RECURSE SOURCES
	src/*.cpp
    external/*.c
    external/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})


target_include_directories(${PROJECT_NAME} PRIVATE external/)
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# make spng use miniz instead of zlib
target_compile_definitions(${PROJECT_NAME} PRIVATE SPNG_USE_MINIZ=1 SPNG_STATIC=1)

# benchmarks
# target_compile_definitions(${PROJECT_NAME} PRIVATE FASTPNG_BENCH)

CPMAddPackage("gh:camila314/uibuilder#ba63097")
CPMAddPackage("gh:dankmeme01/asp2#89924e6")
target_link_libraries(${PROJECT_NAME} asp UIBuilder)
