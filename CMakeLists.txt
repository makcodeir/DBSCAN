cmake_minimum_required(VERSION 3.16)
project(dbscan CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dbscan point.cpp)
target_include_directories(dbscan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(dbscan PRIVATE -Wall -Wextra)
