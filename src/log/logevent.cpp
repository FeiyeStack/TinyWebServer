#include "log/logevent.h"
#include "log/logger.h"
#include <cstdarg>
#include <iostream>

namespace WebSrv
{

    constexpr int FORMAT_BUF_SIZE = 256;

    uint64_t getElapseTime()
    {
        static uint64_t startTime=GetProcessStartTime();
            // 获取当前时间点
        auto currentTime = std::chrono::system_clock::now();

         // 将当前时间点转换为时间戳（毫秒为单位）
        auto duration = currentTime.time_since_epoch();
        uint64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        return millis-startTime;
    }


    LogEvent::LogEvent(std::shared_ptr<Logger> spLogger, LogLevel::Level level,
                       const char *fileName, int32_t lineNum, const char *func, uint64_t elapse,
                       uint32_t threadId, uint32_t fiberId, uint64_t time,const std::string& threadName )
        : _spLogger(spLogger), _level(level), _fileName(fileName),
          _lineNum(lineNum), _func(func), _elapse(elapse), _threadId(threadId),
          _fiberId(fiberId), _time(time),_threadName(threadName)
    {
    }

    LogEvent::~LogEvent()
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        char buf[FORMAT_BUF_SIZE];
        int len = vsnprintf(buf, sizeof(buf), fmt, args);
        if (len != -1)
        {
            _ss << std::string(buf, len);
            fflush(stdout);
        }
        else
        {
            std::cerr << "Error: Buffer too small to store formatted string" << std::endl;
        }

        va_end(args);
    }

   LogEventWrap::LogEventWrap(LogEvent::ptr spLogEvent)
        : _spLogEvent(spLogEvent)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        _spLogEvent->getLogger()->log(_spLogEvent);
    }
} // namespace WebSrv
