#include "log/loglevel.h"

namespace WebSrv
{
     std::string LogLevel::toString(LogLevel::Level level)
    {
        // 默认debug
        switch (level)
        {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        case Level::Off:
            return "OFF";
        default:
            return "DEBUG";
        }
    }

    LogLevel::Level LogLevel::formString(std::string levelStr)
    {
        if (levelStr == "DEBUG" || levelStr == "debug")
        {
            return Level::Debug;
        }
        if (levelStr == "INFO" || levelStr == "info")
        {
            return Level::Info;
        }
        if (levelStr == "WARN" || levelStr == "warn")
        {
            return Level::Warn;
        }
        if (levelStr == "ERROR" || levelStr == "error")
        {
            return Level::Error;
        }
        if (levelStr == "FATAL" || levelStr == "fatal")
        {
            return Level::Fatal;
        }
        if (levelStr == "OFF" || levelStr == "off")
        {
            return Level::Off;
        }
        return Level::Debug;
    }
} // namespace WebSrv
