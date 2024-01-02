#include "log/logappender.h"
#include <iostream>
#include <filesystem>
#include "util.h"
namespace WebSrv
{

    LogAppender::LogAppender(const std::string& name)
    {
        _name=name;
    }



    void ConsoleAppender::log(LogEvent::ptr spLogEvent)
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        _spLogFormater->format(std::cout, spLogEvent);
    }

    FileAppender::FileAppender(const std::string &fileName,const std::string& name)
        :LogAppender(name),_filename(fileName)
    {
        open();
    }

    FileAppender::FileAppender()
    {
    }

    FileAppender::~FileAppender()
    {
        _ofs.close();
        _ofs.clear();
    }

    void FileAppender::log(LogEvent::ptr spLogEvent)
    {
        // 如果流不好，尝试重新打开写入文件,如果设置间隔就根据间隔时差重新打开写入
        if (!_ofs.good())
        {
            if (!reopen())
            {
                std::cerr << "file open failed" << std::endl;
                return;
            }
        }

        std::lock_guard<SpinLock> lock(_spinlock);

        if (!_spLogFormater->format(_ofs, spLogEvent))
        {
            std::cerr << "format error" << std::endl;
        }

        // 据上个日志超过_delayed秒刷新缓存时间
        if (spLogEvent->getTime() >= _flushTime + _delayed)
        {
            _flushTime = spLogEvent->getTime();
            _ofs.flush();
        }
    }

    void FileAppender::open()
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        if(_filename=="%"){
            return;
        }
        if (!FileSystemUtil::openFileForWrite(_ofs, _filename, std::ios::app))
        {
            std::cerr << "file open failed" << std::endl;
            return;
        }
    }

    bool FileAppender::reopen()
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        _ofs.close();
        _ofs.clear();

        if (!FileSystemUtil::openFileForWrite(_ofs, _filename, std::ios::app))
        {
            std::cerr << "file open failed" << std::endl;
            return false;
        }
        return true;
    }

    std::string DailyRollingFileAppender::toString(Rolling rolling)
    {
        switch (rolling)
        {
        case Rolling::MINUTE:
            return "MINUTE";
        case Rolling::HOUR:
            return "HOUR";
        case Rolling::DAY:
            return "DAY";
        case Rolling::WEEK:
            return "WEEK";
        case Rolling::MONTH:
            return "MONTH";
        default:
            return "DAY";
        }
    }

    DailyRollingFileAppender::Rolling DailyRollingFileAppender::formString(const std::string &rollingStr)
    {
        if (rollingStr == "MINUTE" || rollingStr == "minute")
        {
            return Rolling::MINUTE;
        }
        if (rollingStr == "HOUR" || rollingStr == "HOUR")
        {
            return Rolling::HOUR;
        }
        if (rollingStr == "DAY" || rollingStr == "day")
        {
            return Rolling::DAY;
        }
        if (rollingStr == "WEEK" || rollingStr == "week")
        {
            return Rolling::WEEK;
        }
        if (rollingStr == "MONTH" || rollingStr == "month")
        {
            return Rolling::MONTH;
        }
        return Rolling::DAY;
    }

    DailyRollingFileAppender::DailyRollingFileAppender(const std::string &filename,Rolling rolling,const std::string& name)
        :  _rollingType(rolling)
    {
        _orgFilename=filename;
        _filename= modifyFilename(filename);
        FileAppender::open();
    }

    void DailyRollingFileAppender::log(LogEvent::ptr spLogEvent)
    {
        rollingFile(spLogEvent->getTime());
        FileAppender::log(spLogEvent);
    }

    std::string DailyRollingFileAppender::modifyFilename(const std::string &originalFilename)
    {
        std::filesystem::path filepath(originalFilename);
        auto[parent_path,addpath]= toChangeFilePath();

        std::stringstream ss;
        
        ss<<filepath.parent_path().string()<<parent_path<<filepath.stem().string()<<addpath<<filepath.extension().string();
        return ss.str();
    }

    void DailyRollingFileAppender::rollingFile(time_t curTime)
    {
        if(curTime>=_nextTime){
            _filename=modifyFilename(_orgFilename);
            FileAppender::reopen();
        }
    }

    std::pair<std::string, std::string> DailyRollingFileAppender::toChangeFilePath()
    {
        time_t time = SystemClock::to_time_t(SystemClock::now());

        tm *p_tm = localtime(&time);

        std::string  fm1;
        std::string fmt2;
        tm next = *p_tm;

        switch (_rollingType)
        {
        case Rolling::DAY:
            //%Y-%m-%d %H:%M:%S
            fm1="/%Y/%m/";
            fmt2 = "_%Y_%m_%d";
            next.tm_hour = 0;
            next.tm_min = 0;
            next.tm_sec = 0;
            next.tm_mday++;
            break;
        case Rolling::MONTH:
            fm1="_/%Y/";
            fmt2 = "_%Y_%m";
            next.tm_hour = 0;
            next.tm_min = 0;
            next.tm_sec = 0;
            next.tm_mday = 0;
            next.tm_mon++;
            break;
        case Rolling::WEEK:
            fm1="/%Y/%m/";
            fmt2 = "_%Y_%m_%A";
            next.tm_hour = 0;
            next.tm_min = 0;
            next.tm_sec = 0;
            do
            {
                next.tm_mday += 1;
            } while (next.tm_wday != 1);
            break;
        case Rolling::MINUTE:
            fm1="/%Y/%m/%d/%H/";
            fmt2 = "_%Y_%m_%d_%H_%M";
            next.tm_sec = 0;
            next.tm_min++;
            break;
        case Rolling::HOUR:
            fm1="/%Y/%m/%d/";
            next.tm_min = 0;
            next.tm_sec = 0;
            next.tm_hour++;
            fmt2 = "_%Y_%m_%d_%H";
            break;
        default:
            break;
        }
        // 设置下一次的时间
        _nextTime = std::mktime(&next);
        char addName[80];
        std::strftime(addName, sizeof(addName), fmt2.c_str(), &(*p_tm));
        char addParent[80];
        std::strftime(addParent, sizeof(addParent), fm1.c_str(), &(*p_tm));

        return {addParent, addName};
    }

    ConsoleAppender::ConsoleAppender(const std::string& name)
        :LogAppender(name)
    {
    }

    AsyncAppender::AsyncAppender(const std::string& name)
        :LogAppender(name)
    {
    }

    void AsyncAppender::log(LogEvent::ptr spLogEvent)
    {
    }



} // namespace WebSrv
