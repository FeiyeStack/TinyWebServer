cmake_minimum_required(VERSION 3.10)

project(TinyWebServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS}  -rdynamic -O3 -fPIC -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-error=trigraphs")
#add_subdirectory(tests)
add_subdirectory(src)
add_subdirectory(examples)