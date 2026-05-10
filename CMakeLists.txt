cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0072 NEW)

project(ZenGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(vendor/poly2tri)
add_subdirectory(zenengine)
add_subdirectory(main)

