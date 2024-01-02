project(StaticFileServer)

# 包含头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(StaticFileServer staticfileserver.cpp)
target_link_libraries(StaticFileServer TinyWebServerLib)