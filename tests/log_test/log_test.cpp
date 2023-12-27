#include <iostream>
#include <ctime>
#include <chrono>
#include "TinyWebServer/log.h"
#include <thread>
#include <mutex>
#include <filesystem>
#include "TinyWebServer/configurator.h"
using namespace std;
using namespace WebSrv;
// 简单测试日志功能

void testPrint()
{
    cout << endl;
    cout << "--------test base print--------" << endl;

    Logger::ptr logger = SRV_LOGGER_NAME("test");
    logger->addAppender(LogAppender::ptr(new ConsoleAppender));

    LogLevel::Level level = LogLevel::Debug;
    for (int i = level; i <= LogLevel::Off; i++)
    {
        level = static_cast<LogLevel::Level>(i);
        logger->setLevel(level);
        cout << "logger level=" << LogLevel::toString(level) << endl;
        SRV_LOG_DEBUG(logger) << "log debug";
        SRV_LOG_INFO(logger) << "log info";
        SRV_LOG_WARN(logger) << "log warn";
        SRV_LOG_ERROR(logger) << "log error";
        SRV_LOG_FATAL(logger) << "log fatal";
        SRV_LOG_DEBUG_FMT(logger, "log debug fmt");
        SRV_LOG_INFO_FMT(logger, "log info fmt");
        SRV_LOG_WARN_FMT(logger, "log warn fmt");
        SRV_LOG_ERROR_FMT(logger, "log error %s", "fmt");
        SRV_LOG_FATAL_FMT(logger, "log fatal %s", "fmt");
        logger->debug("log info member function");
        logger->info("log info member function");
        logger->warn("log info member function");
        logger->error("log info member function");
        logger->fatal("log info member function");
    }
}

void printLog(Logger::ptr logger)
{
    
    SRV_LOG_DEBUG(logger) << "log debug";
    SRV_LOG_INFO(logger) << "log info";

    SRV_LOG_DEBUG_FMT(logger, "log debug fmt");
    SRV_LOG_INFO_FMT(logger, "log info fmt");

    logger->debug("log info member function");
    logger->info("log info member function");
}

void testThreadPrint()
{
    cout << endl;
    cout << "--------test three thread print--------" << endl;
    Logger::ptr logger1 = SRV_LOGGER_NAME("test1");
    Logger::ptr logger2 = SRV_LOGGER_NAME("test2");
    Logger::ptr logger3 = SRV_LOGGER_NAME("test3");

    std::thread thread1(printLog,logger1);
    std::thread thread2(printLog,logger2);
    std::thread thread3(printLog,logger3);

    thread1.join();
    thread2.join();
    thread3.join();
}

void testFormater()
{
    Logger::ptr logger = SRV_LOGGER_NAME("test");
    logger->setLevel(LogLevel::Debug);
    logger->setLogFormater("%d{%Y-%m-%d %H:%M:%S} %rms (%t) (%F) [%p] [%c] %f:%l> %t%m%n %% %L %%%n");
    logger->info("add elapse");
}

void testAppender()
{
    Logger::ptr logger = SRV_LOGGER_NAME("test2");
    logger->addAppender(LogAppender::ptr(new ConsoleAppender));
    logger->addAppender(LogAppender::ptr(new FileAppender("./log/test.log")));
    logger->addAppender(LogAppender::ptr(new DailyRollingFileAppender("./log/test2.log",DailyRollingFileAppender::Rolling::MINUTE)));
    logger->info("Let's wait for 60 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    logger->info("I think it's okay");
}

void testLoadYaml()
{
    Configurator::loadConfigFile("/home/ubuntu/work/TinyWebServer/tests/log_test/log_test.yaml");
    Logger::ptr logger = SRV_LOGGER_NAME("test3");
    SRV_LOG_INFO(logger)<<"Initialize using a configuration file";
    Logger::ptr logger2 = SRV_LOGGER_NAME("system");
    logger2->info("print system");
}
int main()
{
    cout << "--------Logger Test--------\n"
         << endl;
    testPrint();
    testThreadPrint();
    testFormater();

    //testAppender();

    testLoadYaml();


}