cmake_minimum_required(VERSION 3.17)
project(rans)

set(CMAKE_CXX_STANDARD 17)

add_executable(rans main.cpp lib/encoder_and_decoder.cpp lib/encoder_and_decoder.h)