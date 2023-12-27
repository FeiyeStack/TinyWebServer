#pragma once
#include <string>
namespace WebSrv
{
       // 日志等级
    class LogLevel
    {
    public:
        enum Level
        {
            Debug = 0x0,
            Info,
            Warn,
            Error,
            Fatal,
            Off,
        };
        /**
         * @brief 日志等级转string 默认debug
         *
         * @param level 日志级别
         * @return std::string
         */
        static std::string toString(LogLevel::Level level);

        /**
         * @brief string转日志等级 默认debug
         *
         * @param levelStr 日志字符串
         * @return LogLevel::Level
         */
        static LogLevel::Level formString(std::string levelStr);
    };

}