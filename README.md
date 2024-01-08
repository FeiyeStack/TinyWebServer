# TinyWebServer

## 前言
该服务器框架主要参考b站[[C++高级教程]从零开始开发服务器框架(sylar)教程实现](https://www.bilibili.com/video/BV184411s7qF/?spm_id_from=333.337.search-card.all.click)，完成了服务器基础部分，并添加了mysql接口，实现了一个简单http静态文件例子
原本打算跟着教程实现windows,linux双端，但因为因为c++20的协程结构问题（需要协程函数包裹等），对于设计模式不清楚的我只能转头先实现linux，后续可能会使用boost的协程已实现windows端

## 日志器模块

使用log4j风格设计,包含日志器、日志事件、日志追加器、日志格式器

## 配置模块

## 协程模块


## 调度器模块

## Hook模块

## Socket模块

## ByteArray序列化模块

## HTTP模块

## Servlet模块

## db模块

## 一些问题

最开始实现的是windows端，并且使用c++20 协程，但后面先放弃了
一是结构问题，当初使用c++20 协程，全程使用智能指针包裹，但还是会出现释放报错，所以是自己设计的结构有问题；
二是日志模块，在c++20协程无法正确执行，和上述一样，投递日志事件时，报错点为析构`std::base_string （0xfeeefeeefeeefeee <error: Cannot access memory at address 0xfeeefeeefeeefeee>）`疑似指针悬空，但它是智能指针包裹
三使用读写锁（c++ 17的`std::shared_mutex`）会出现锁不住的情况，只能使用互斥锁，但希望使用读写锁的场景，使用互斥锁的会拖慢速度。


