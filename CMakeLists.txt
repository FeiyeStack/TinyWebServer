cmake_minimum_required(VERSION 3.10)

project(TinyWebServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(tests)
add_subdirectory(src)
add_subdirectory(examples)