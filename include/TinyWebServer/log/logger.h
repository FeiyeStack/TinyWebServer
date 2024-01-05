#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <thread>

#include <map>
#include "mutex.h"
#include "logevent.h"
#include "logappender.h"
#include "logformater.h"
#include "util.h"
#include "thread.h"
#include "fiber.h"
#define CALLER_FILE() __builtin_FILE()
#define CALLER_LINE() __builtin_LINE()
#define CALLER_FUNCTION() __builtin_FUNCTION()

#define SRV_LOGGER_LEVEL(logger, level)                                                                             \
    if (level >= logger->getLevel())                                                                                \
    WebSrv::LogEventWrap(WebSrv::LogEvent::ptr(                                                                       \
                             new WebSrv::LogEvent(logger, level,                                                            \
                                          CALLER_FILE(), CALLER_LINE(), CALLER_FUNCTION(), WebSrv::getElapseTime(), \
                                          WebSrv::getSystemTheadId(), WebSrv::Fiber::getFiberId(),                                            \
                                          WebSrv::SystemClock::to_time_t(WebSrv::SystemClock::now()),WebSrv::Thread::getName())))             \
        .getStringStream()
#define SRV_LOG_DEBUG(logger) SRV_LOGGER_LEVEL(logger, WebSrv::LogLevel::Debug)
#define SRV_LOG_INFO(logger) SRV_LOGGER_LEVEL(logger, WebSrv::LogLevel::Info)
#define SRV_LOG_WARN(logger) SRV_LOGGER_LEVEL(logger, WebSrv::LogLevel::Warn)
#define SRV_LOG_ERROR(logger) SRV_LOGGER_LEVEL(logger, WebSrv::LogLevel::Error)
#define SRV_LOG_FATAL(logger) SRV_LOGGER_LEVEL(logger, WebSrv::LogLevel::Fatal)

#define SRV_LOGGER_LEVEL_FMT(logger, level, fmt, ...)                                                               \
    if (level >= logger->getLevel())                                                                                \
    WebSrv::LogEventWrap(WebSrv::LogEvent::ptr(                                                                       \
                             new WebSrv::LogEvent(logger, level,                                                            \
                                          CALLER_FILE(), CALLER_LINE(), CALLER_FUNCTION(), WebSrv::getElapseTime(), \
                                          WebSrv::getSystemTheadId(), 0,                                            \
                                          WebSrv::SystemClock::to_time_t(WebSrv::SystemClock::now()),WebSrv::Thread::getName())))             \
        .getEvent()                                                                                                 \
        ->format(fmt, ##__VA_ARGS__)

#define SRV_LOG_DEBUG_FMT(logger, fmt, ...) SRV_LOGGER_LEVEL_FMT(logger, WebSrv::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define SRV_LOG_INFO_FMT(logger, fmt, ...) SRV_LOGGER_LEVEL_FMT(logger, WebSrv::LogLevel::Info, fmt, ##__VA_ARGS__)
#define SRV_LOG_WARN_FMT(logger, fmt, ...) SRV_LOGGER_LEVEL_FMT(logger, WebSrv::LogLevel::Warn, fmt, ##__VA_ARGS__)
#define SRV_LOG_ERROR_FMT(logger, fmt, ...) SRV_LOGGER_LEVEL_FMT(logger, WebSrv::LogLevel::Error, fmt, ##__VA_ARGS__)
#define SRV_LOG_FATAL_FMT(logger, fmt, ...) SRV_LOGGER_LEVEL_FMT(logger, WebSrv::LogLevel::Fatal, fmt, ##__VA_ARGS__)

#define SRV_LOGGER_NAME(name) WebSrv::LoggerManage::getInstance()->getLogger(name)
#define SRV_LOGGER_ROOT() WebSrv::LoggerManage::getInstance()->getRoot()
namespace WebSrv
{
    // std::enable_shared_from_this<Logger> 允许在 Logger 函数中使用 shared_from_this() 创建一个与对象相关联的

    /**
     * @brief 日志器
     * 发布日志事件，指定打印格式，输出目标
     */
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManage;

    public:
        using ptr = std::shared_ptr<Logger>;

        Logger(std::string name);

        std::string getName() const
        {
            return _name;
        }

        /**
         * @brief 使用事件，向目标器输出日志
         *
         * @param spLogEvent
         */
        void log(LogEvent::ptr spLogEvent);

        /**
         * @brief 直接构建日志事件，向目标器输出日志
         *
         * @param msg 消息
         * @param level 日志级别
         * @param curFile 当前运行文件名
         * @param curLine 当前运行行数
         */
        void log(const std::string &msg, LogLevel::Level level,
                 const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());

        void debug(const std::string &msg,
                   const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());
        void info(const std::string &msg,
                  const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());
        void warn(const std::string &msg,
                  const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());
        void error(const std::string &msg,
                   const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());
        void fatal(const std::string &msg,
                   const char *curFile = CALLER_FILE(), int curLine = CALLER_LINE(), const char *curFunc = CALLER_FUNCTION());

        /**
         * @brief 添加目标器
         *
         * @param appender
         */
        void addAppender(LogAppender::ptr appender);

        /**
         * @brief 删除目标器
         * 用对应指针移除
         * @param appender
         */
        void delAppender(LogAppender::ptr appender);

        /**
         * @brief 删除目标器
         * 先提是非匿名的
         * @param name 目标器名
         */
        void delAppender(const std::string &name);

        void clearAppender();

        LogAppender::ptr getLogAppender(const std::string &name);

        std::list<LogAppender::ptr> getLogAppender();
        /**
         * @brief 设置格式器，如果目标器使用相同的格式器，将被一起被设置
         *
         * @param spLogFormater
         */
        void setLogFormater(LogFormater::ptr spLogFormater);
        /**
         * @brief 以字符格式的形式添加格式器
         *
         * @param fmt 字符格式
         */
        void setLogFormater(const std::string &fmt);

        LogFormater::ptr getLogFormater()
        {
            return _spLogFormater;
        }
        /**
         * @brief 主日志器
         *
         * @return Logger::ptr
         */
        Logger::ptr getRoot();

        void setLevel(LogLevel::Level level)
        {
            _minLevel = level;
        }

        LogLevel::Level getLevel()
        {
            return _minLevel;
        }

    private:
        std::string _name;
        std::list<LogAppender::ptr> _appenderList;
        // 格式器
        LogFormater::ptr _spLogFormater;
        // 日志最小级别
        LogLevel::Level _minLevel = LogLevel::Debug;
        SpinLock _spinlock;
        // 主日志器
        Logger::ptr rootLogger;
    };

    /**
     * @brief 管理日志器
     *
     */
    class LoggerManage
    {
    public:
        LoggerManage(const LoggerManage &) = delete;
        LoggerManage(LoggerManage &&) = delete;
        LoggerManage &operator=(const LoggerManage &) = delete;

    private:
        LoggerManage();
        ~LoggerManage() {}

    public:
        static LoggerManage *getInstance();
        /**
         * @brief 通过名称获取日志器，如果不存在创建一个
         *
         * @param name
         */
        Logger::ptr getLogger(std::string name);
        /**
         * @brief 获取主日志器
         *
         */
        Logger::ptr getRoot();

    private:
        std::map<std::string, Logger::ptr> _loggerMap;
        SpinLock _spinlock;
    };

} // namespace WebSrv
