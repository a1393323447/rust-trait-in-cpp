cmake_minimum_required(VERSION 3.23)
project(rust_trait_in_cpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(traits_demo main.cpp Add.h Point.h)
