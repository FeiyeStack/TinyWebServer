#pragma once
#include <string>
#include <memory>
#include <thread>
#include <sstream>
#include <chrono>
#include "loglevel.h"

namespace WebSrv
{
    //获取程序启动到现在的时间（ms）
    uint64_t getElapseTime();

    //跨协程跨线程切换上下文智能指针在末尾处释放时出现base_string 错误

    class Logger;
    /**
     * @brief 日志事件
     *
     * 记录事件，传输字符流
     */
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;
        LogEvent(std::shared_ptr<Logger> spLogger, LogLevel::Level level,
                 const char *fileName, int32_t lineNum, const char *func, uint64_t elapse,
                 uint32_t threadId, uint32_t fiberId, uint64_t time,const std::string& threadName );
        ~LogEvent();
        /**
         * @brief 输入日志消息
         *
         * 可以以print格式器形式输入字符
         * @param fmt 格式化字符
         * @param ...
         */
        void format(const char *fmt, ...);

        LogLevel::Level getLevel() const
        {
            return _level;
        }

        const char *getFileName() const
        {
            return _fileName;
        }

        uint32_t getLineNum() const
        {
            return _lineNum;
        }
        uint64_t getElapse() const
        {
            return _elapse;
        }
        uint32_t getThreadId() const
        {
            return _threadId;
        }

        uint32_t getFiberId() const
        {
            return _fiberId;
        }
        time_t getTime() const
        {
            return _time;
        }
        std::stringstream &getStringStream()
        {
            return _ss;
        }

        std::shared_ptr<Logger> getLogger() const
        {
            return _spLogger;
        }
        const char *getFunctionName()
        {
            return _func;
        }
        std::string getThreadName() const{
            return _threadName;
        }
    private:
        // 日志器
        std::shared_ptr<Logger> _spLogger;
        // 日志级别
        LogLevel::Level _level;
        // 对应代码文件名
        const char *_fileName;
        // 文件行号
        uint32_t _lineNum;
        // 函数名
        const char *_func;
        // 启动程序到现在的耗时毫秒数
        uint64_t _elapse;
        // 线程id
        uint32_t _threadId;
        // 协程id
        uint32_t _fiberId;
        // 日志事件发生时间
        time_t _time;
        // 写入日志消息字符流
        std::stringstream _ss;
        //线程名称
        std::string _threadName;
    };

    /**
     * @brief 日志事件包装器
     * 管理日志生命周期,当日志事件析构时，将消息输出
     */
    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr spLogEvent);

        ~LogEventWrap();
        /**
         * @brief 允许通过<<将消息直接传达给事件
         *
         * @return std::stringstream&
         */
        std::stringstream &getStringStream()
        {
            return _spLogEvent->getStringStream();
        }

        LogEvent::ptr getEvent() const
        {
            return _spLogEvent;
        }

    private:
        LogEvent::ptr _spLogEvent;
    };
} // namespace WebSrv
