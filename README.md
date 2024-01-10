# TinyWebServer

## 前言
该服务器框架，完成了服务器基础部分，并添加了mysql接口，实现了一个简单http静态文件例子
原本打算同时实现windows,linux双端，但因为因为c++20的协程结构问题（需要协程函数包裹等），对于设计模式不清楚的我只能转头先实现linux，后续可能会使用boost的协程来实现windows端，从而兼容双端

## 结构

- 日志器模块 (N+M)
  - 使用log4j风格设计
  - 日志器（Logger）
  - 日志事件(LogEvent)
  - 日志追加器(LogAppender->FileAppender,DailyRollingFileAppender,ConsoleAppender)
  - 日志格式器(LogFormater)
- 配置模块
  - 使用yaml作为配置
  - 配置变量 N
  - 配置器 1
- 协程模块
  - 使用 linux ucontext进行封装
- IO调度器模块
  - io事件处理
  - 定时器
  - 协程模块
- Hook模块
  - 对socket事件系统函数进行重写
- Socket模块
  - 封装socket
- ByteArray序列化模块
  - 提供缓存结构，将不同类型转成字节序
- HTTP模块
  - 提供简单http的请求报文和响应报文的解析
  - 包含http服务器和连接客户端
- Servlet模块
  - http服务器路径处理请求和应答
- db模块
  - 对数据库接口进行封装，实现mysql部分

## 一些问题

最开始实现的是windows端，并且使用c++20 协程，但后面先放弃了
一是结构问题，当初使用c++20 协程，但应为是协程函数体包裹的，注定构建形式会不一样，由于设计能力较弱，出现那种协程函数体套娃包裹，不是很合适
二是日志模块，在c++20协程无法正确执行，和上述一样，投递日志事件时，报错点为析构`std::base_string （0xfeeefeeefeeefeee <error: Cannot access memory at address 0xfeeefeeefeeefeee>）`疑似指针悬空，但它是智能指针包裹，使用linux协程到未出错。
三使用读写锁（c++ 17的`std::shared_mutex`）会出现锁不住的情况，只能使用互斥锁，但希望使用读写锁的场景，使用互斥锁的会拖慢速度。


