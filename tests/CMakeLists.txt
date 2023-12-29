project(Test)

# 包含头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)


add_executable(LoggerTest log_test/log_test.cpp)
target_link_libraries(LoggerTest TinyWebServerLib)

add_executable(configuratorTest configurator_test/configurator_test.cpp)
target_link_libraries(configuratorTest TinyWebServerLib)

add_executable(test_bytearray test_bytearray.cpp)
target_link_libraries(test_bytearray TinyWebServerLib)

add_executable(test_fiber test_fiber.cpp)
target_link_libraries(test_fiber TinyWebServerLib)

add_executable(test_scheduler test_scheduler.cpp)
target_link_libraries(test_scheduler TinyWebServerLib)

add_executable(test_iomanager test_iomanager.cpp)
target_link_libraries(test_iomanager TinyWebServerLib)

add_executable(test_socket test_socket.cpp)
target_link_libraries(test_socket TinyWebServerLib)

add_executable(test_address test_address.cpp)
target_link_libraries(test_address TinyWebServerLib)

add_executable(test_http test_http.cpp)
target_link_libraries(test_http TinyWebServerLib)

add_executable(test_httpparse test_httpparse.cpp)
target_link_libraries(test_httpparse TinyWebServerLib)