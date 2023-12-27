#include "log/logformater.h"
#include <map>
#include <functional>
#include <iomanip>
#include "log/logger.h"
#include "log/loglevel.h"

namespace WebSrv
{
    LogFormater::LogFormater(const std::string &patten)
        : _pattern(patten)
    {
        if (_pattern == "")
        {
            _pattern = "%d{%Y-%m-%d %H:%M:%S} (%t) (%F) [%p] [%c] %f:%l> %m%n";
        }
        formatItemInit();
    }

    std::string LogFormater::format(LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &item : _formatItems)
        {
            item->format(ss, event);
        }
        return ss.str();
    }

    std::ostream &LogFormater::format(std::ostream &os, LogEvent::ptr event)
    {
        for (auto &item : _formatItems)
        {
            item->format(os, event);
        }
        return os;
    }

    //  * %p 日志级别
    class LevelFormatItem : public LogFormater::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << LogLevel::toString(event->getLevel());
        }
    };

    //  * %f 文件名
    class FileNameFormatItem : public LogFormater::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFileName();
        }
    };

    //  * %l 行号
    class LineNumFormatItem : public LogFormater::FormatItem
    {
    public:
        LineNumFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLineNum();
        }
    };

    //  * %r 依赖累计毫秒数
    class ElapseFormatItem : public LogFormater::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    //  * %c 日志名称
    class LoggerNameFormatItem : public LogFormater::FormatItem
    {
    public:
        LoggerNameFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };
    //  * %t 线程id
    class ThreadIdFormatItem : public LogFormater::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };
    //  * %F 协程

    class FiberIdFormatItem : public LogFormater::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };
    //  * %T 制表名
    class TableFormatItem : public LogFormater::FormatItem
    {
    public:
        TableFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << '\t';
        }
    };

    //  * %n 换行
    class NewLineFormatItem : public LogFormater::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << '\n';
        }
    };

    //  * %m 消息
    class MessageFormatItem : public LogFormater::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getStringStream().str();
        }
    };

    //  * %d 时间 {}内时间字符解析，参std::put_time 默认"%Y-%m-%d %H:%M:%S"
    class TimeFormatItem : public LogFormater::FormatItem
    {
    public:
        TimeFormatItem(const std::string &str = "") : _fmt(str)
        {
            if (_fmt == "")
            {
                _fmt = "%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            time_t time = event->getTime();
            tm *p_tm = localtime(&time);
            os << std::put_time(p_tm, _fmt.c_str());
        }

    private:
        std::string _fmt;
    };

    //  * %% 百分号
    class PercentFormatItem : public LogFormater::FormatItem
    {
    public:
        PercentFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "%";
        }
    };

    // 无格式
    class StringFormatItem : public LogFormater::FormatItem
    {
    public:
        StringFormatItem(const std::string &str = "") : _str(str)
        {
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << _str;
        }

    private:
        std::string _str;
    };

    class PositionFormatItem : public LogFormater::FormatItem
    {
    public:
        PositionFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFileName() << ':' << event->getLineNum() << "> (" << (event->getFunctionName() ? event->getFunctionName() : "[]") << ')';
        }
    };

    //  * %N 线程名
    class ThreadNameFormatItem : public LogFormater::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "")
        {
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };

    //%x %{yyy} %%
    void LogFormater::formatItemInit()
    {
        // 如果%xxx 那可能要加个字典 不然没法判断类似于 %hello，那几个字符是格式 （关于宽度可以采取 %-12d 形式目前未实现）

        // 单元格式标识(0为非格式内容,1格式内容)，单元格式字符，单元格式中格式{yyy}
        std::vector<std::tuple<int, char, std::string>> patternVec;
        // 解析格式

        int state = 0;
        std::string temp = "";
        char fistFmt;
        std::string secondFmt = "";
        for (int i = 0; i < _pattern.size(); i++)
        {
            switch (state)
            {
            case 0:
                if (_pattern[i] == '%')
                {
                    if (temp != "")
                    {
                        patternVec.emplace_back(0, char(), temp);
                        temp = "";
                    }
                    state = 1;
                    continue;
                }
                temp += _pattern[i];
                break;
            case 1:
                fistFmt = _pattern[i];
                if (i + 1 < _pattern.size() && _pattern[i + 1] == '{')
                {
                    state = 2;
                    i++;
                    continue;
                }
                patternVec.emplace_back(1, fistFmt, std::string());
                state = 0;
                break;
            case 2:
                if (_pattern[i] == '}')
                {
                    patternVec.emplace_back(1, fistFmt, secondFmt);
                    secondFmt = "";
                    state = 0;
                    continue;
                }
                secondFmt += _pattern[i];
                break;
            default:
                break;
            }
        }

        // 单元格式器实例化字典，更加key返回对应类型实例化
        std::map<char, std::function<FormatItem::ptr(const std::string &str)>> fmtItemMap = {
            {'m', [](const std::string &str)
             { return FormatItem::ptr(new MessageFormatItem(str)); }},
            {'p', [](const std::string &str)
             { return FormatItem::ptr(new LevelFormatItem(str)); }},
            {'r', [](const std::string &str)
             { return FormatItem::ptr(new ElapseFormatItem(str)); }},
            {'c', [](const std::string &str)
             { return FormatItem::ptr(new LoggerNameFormatItem(str)); }},
            {'t', [](const std::string &str)
             { return FormatItem::ptr(new ThreadIdFormatItem(str)); }},
            {'F', [](const std::string &str)
             { return FormatItem::ptr(new FiberIdFormatItem(str)); }},
            {'n', [](const std::string &str)
             { return FormatItem::ptr(new NewLineFormatItem(str)); }},
            {'f', [](const std::string &str)
             { return FormatItem::ptr(new FileNameFormatItem(str)); }},
            {'T', [](const std::string &str)
             { return FormatItem::ptr(new TableFormatItem(str)); }},
            {'l', [](const std::string &str)
             { return FormatItem::ptr(new LineNumFormatItem(str)); }},
            {'d', [](const std::string &str)
             { return FormatItem::ptr(new TimeFormatItem(str)); }},
            {'%', [](const std::string &str)
             { return FormatItem::ptr(new PercentFormatItem(str)); }},
            {'L', [](const std::string &str)
             { return FormatItem::ptr(new PositionFormatItem(str)); }},
            {'N', [](const std::string &str)
             { return FormatItem::ptr(new ThreadNameFormatItem(str)); }},
        };

        for (auto [type, firstFmt, secondFmt] : patternVec)
        {
            if (type == 0)
            {
                _formatItems.emplace_back(FormatItem::ptr(new StringFormatItem(secondFmt)));
            }
            else
            {
                if (fmtItemMap.count(firstFmt) > 0)
                {
                    _formatItems.emplace_back(fmtItemMap[firstFmt](secondFmt));
                }
                else
                {
                    _formatItems.emplace_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::to_string(firstFmt) + ">>")));
                }
            }
        }
    }

} // namespace WebSrv
