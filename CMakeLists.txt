cmake_minimum_required(VERSION 3.16)
project(dbscan_csv_adapter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(csv_adapter_demo main.cpp)
target_include_directories(csv_adapter_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(csv_adapter_demo PRIVATE -Wall -Wextra)
