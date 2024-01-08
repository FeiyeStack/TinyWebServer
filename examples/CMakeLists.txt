project(Examples)

# 包含头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(httpstaticfile httpstaticfile/httpstaticfile.cpp)
target_link_libraries(httpstaticfile TinyWebServerLib)

