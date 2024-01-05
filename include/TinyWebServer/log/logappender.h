#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <mutex>
#include "mutex.h"
#include "logformater.h"
#include <fstream>
#include "logevent.h"

namespace WebSrv
{

    /**
     * @brief 日志输出目标
     * 将格式化的字符流输出到对应目标
     */
    class LogAppender
    {
    public:
        using ptr = std::shared_ptr<LogAppender>;
        LogAppender(const std::string& name="");
        virtual ~LogAppender() {}

        virtual void log(LogEvent::ptr spLogEvent) = 0;

        /**
         * @brief 如果没有设置格式器，它将默认使用Logger的格式器
         *
         * @param spLogFormat
         */
        void setFormater(LogFormater::ptr spLogFormat)
        {
            std::lock_guard<SpinLock> lock(_spinlock);
            _spLogFormater = spLogFormat;
        }

        LogFormater::ptr getFormater()
        {
            std::lock_guard<SpinLock> lock(_spinlock);
            return _spLogFormater;
        }

        void setName(const std::string name)
        {
            _name = name;
        }

        std::string getName() const
        {
            return _name;
        }

        bool isFormater()
        {
            return _spLogFormater != nullptr;
        }

    protected:
        LogFormater::ptr _spLogFormater;
        SpinLock _spinlock;

        std::string _name = "";
    };

    // 除异步目标器，其余为同步

    /**
     * @brief 文件目标器
     * 设置延时刷新缓存，多线程下，多个ofstream写入文件会出现文件缓存丢失
     * 如果有多个FileAppend将出现缓存不可靠，所以默认写入立即刷新缓存（有待优化）
     */
    class FileAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileAppender>;

        FileAppender(const std::string &filename,const std::string& name="");
        FileAppender();

        ~FileAppender();
        void log(LogEvent::ptr spLogEvent) override;

    protected:
        void open();
        /**
         * @brief 发生流异常时重新打开文件
         *
         * @return true
         * @return false
         */
        bool reopen();
        std::string _filename;
        std::ofstream _ofs;
        // 刷新缓存存入文件时间
        time_t _flushTime=0;
        // 刷新缓存间隔,为0时立即刷新
        int _delayed=0;
    };

    /**
     * @brief 日期滚动文件目标器
     * 按照设定日期频率，转存日志文件
     */
    class DailyRollingFileAppender : public FileAppender
    {
    public:
        using ptr = std::shared_ptr<DailyRollingFileAppender>;

        enum class Rolling
        {
            MINUTE,
            HOUR,
            DAY,
            WEEK,
            MONTH
        };
        static std::string toString(Rolling rolling);

        static Rolling formString(const std::string &rollingStr);

        DailyRollingFileAppender(const std::string &filename,Rolling rolling = Rolling::DAY,const std::string& name="");
        
        void log(LogEvent::ptr spLogEvent) override;

    protected:
        std::string modifyFilename(const std::string &originalFilename);
        void rollingFile(time_t curTime);
        /**
         * @brief 根据rollingType类型返回改变格式
         *
         * @param filename
         * @return std::pair<std::string,std::string>  first 父文件名 second扩增文件名
         */
        std::pair<std::string, std::string> toChangeFilePath();
        // 原始文件名称，不加入时间后缀
        std::string _orgFilename;
        Rolling _rollingType = Rolling::DAY;
        time_t _nextTime=0;
    };

    /**
     * @brief 控制台目标器
     *
     */
    class ConsoleAppender : public LogAppender
    {
    public:
        ConsoleAppender(const std::string& name="");
        void log(LogEvent::ptr spLogEvent) override;
    };

    /**
     * @brief 异步目标器
     * 以异步的形式输出到控制台中
     */
    class AsyncAppender : public LogAppender
    {
    public:
        AsyncAppender(const std::string& name="");
        void log(LogEvent::ptr spLogEvent) override;
    };
} // namespace WebSrv
