# Web 服务器子项目
project(TinyWebServerLib)

set(SRC
    log/logappender.cpp
    log/logevent.cpp
    log/logformater.cpp
    log/logger.cpp
    log/loglevel.cpp
    util.cpp
    configurator.cpp
    thread.cpp
    bytearray.cpp
    fiber.cpp
    scheduler.cpp
    iomanager.cpp
    timer.cpp
    hook.cpp
    fdmanager.cpp
    socket.cpp
    address.cpp
    http/http.cpp
    http/httpparser.cpp
    tcpserver.cpp
    http/httpserver.cpp
    stream.cpp
    streams/socketstream.cpp
    http/httpsession.cpp
    http/servlet.cpp
    http/httpconnection.cpp
    db/mysql.cpp
    )

# 添加源文件
add_library(TinyWebServerLib ${SRC})

# 包含头文件路径
target_include_directories(TinyWebServerLib PUBLIC ${CMAKE_SOURCE_DIR}/include/TinyWebServer)


include_directories(/usr/include/mysql)

# 查找yaml-cpp包
find_package(yaml-cpp REQUIRED)

# 链接yaml-cpp库
target_link_libraries(TinyWebServerLib yaml-cpp mysqlclient)
