#include "log/logger.h"
#include <iostream>
#include <functional>
#include <tuple>
#include <iomanip>
#include <chrono>
#include "util.h"
#include "configurator.h"
#include <set>
#include <unordered_set>
#include <iostream>
#include <type_traits>

namespace WebSrv
{
    Logger::Logger(std::string name)
        : _name(name), _spLogFormater(new LogFormater())
    {
    }

    void Logger::log(LogEvent::ptr spLogEvent)
    {
        if (spLogEvent->getLevel() < _minLevel)
        {
            return;
        }
        std::lock_guard<SpinLock> lock(_spinlock);
        if (!_appenderList.empty())
        {
            for (auto appender : _appenderList)
            {
                appender->log(spLogEvent);
            }
        }
        else
        {
            rootLogger->log(spLogEvent);
        }
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (appender != nullptr)
        {
            std::lock_guard<SpinLock> lock(_spinlock);
            if (!appender->isFormater())
            {
                appender->setFormater(_spLogFormater);
            }
            _appenderList.emplace_back(appender);
        }
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        if (appender == nullptr)
        {
            return;
        }
        std::lock_guard<SpinLock> lock(_spinlock);
        for (auto it = _appenderList.begin(); it != _appenderList.end(); it++)
        {
            if (*it == appender)
            {
                _appenderList.erase(it);
                break;
            }
        }
    }

    void Logger::delAppender(const std::string &name)
    {
        if (name == "")
        {
            return;
        }
        std::lock_guard<SpinLock> lock(_spinlock);
        for (auto it = _appenderList.begin(); it != _appenderList.end(); it++)
        {
            if ((*it)->getName() == name)
            {
                _appenderList.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender()
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        _appenderList.clear();
    }

    std::list<LogAppender::ptr> Logger::getLogAppender()
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        return _appenderList;
    }

    LogAppender::ptr Logger::getLogAppender(const std::string &name)
    {
        if (name != "")
        {
            std::lock_guard<SpinLock> lock(_spinlock);
            for (auto it = _appenderList.begin(); it != _appenderList.end(); it++)
            {
                if ((*it)->getName() == name)
                {
                    return *it;
                }
            }
        }

        return nullptr;
    }

    void Logger::setLogFormater(LogFormater::ptr spLogFormater)
    {
        if (spLogFormater == nullptr)
        {
            return;
        }
        std::lock_guard<SpinLock> lock(_spinlock);
        for (auto appender : _appenderList)
        {
            if (_spLogFormater == appender->getFormater())
            {
                appender->setFormater(spLogFormater);
            }
        }
        _spLogFormater = spLogFormater;
    }

    void Logger::setLogFormater(const std::string &fmt)
    {
        if (fmt == "")
        {
            return;
        }

        LogFormater::ptr spLogFormater(new LogFormater(fmt));
        setLogFormater(spLogFormater);
    }

    Logger::ptr Logger::getRoot()
    {
        if (rootLogger != nullptr)
        {
            return rootLogger;
        }
        return shared_from_this();
    }

    void Logger::log(const std::string &msg, LogLevel::Level level, const char *curFile, int curLine, const char *curFunc)
    {
        if (level < _minLevel)
        {
            return;
        }

        // 暂时无协程id
        LogEvent::ptr event = LogEvent::ptr(
            new LogEvent(shared_from_this(), level,
                         curFile, curLine, curFunc, WebSrv::getElapseTime(),
                         getSystemTheadId(), WebSrv::Fiber::getFiberId(),
                         SystemClock::to_time_t(SystemClock::now()),Thread::getName()));
        event->getStringStream() << msg;
        LogEventWrap eventWrap(event);
    }

    void Logger::debug(const std::string &msg, const char *curFile, int curLine, const char *curFunc)
    {
        log(msg, LogLevel::Debug, curFile, curLine, curFunc);
    }
    void Logger::info(const std::string &msg, const char *curFile, int curLine, const char *curFunc)
    {
        log(msg, LogLevel::Info, curFile, curLine, curFunc);
    }
    void Logger::warn(const std::string &msg, const char *curFile, int curLine, const char *curFunc)
    {
        log(msg, LogLevel::Warn, curFile, curLine, curFunc);
    }
    void Logger::error(const std::string &msg, const char *curFile, int curLine, const char *curFunc)
    {
        log(msg, LogLevel::Error, curFile, curLine, curFunc);
    }
    void Logger::fatal(const std::string &msg, const char *curFile, int curLine, const char *curFunc)
    {
        log(msg, LogLevel::Fatal, curFile, curLine, curFunc);
    }

    LoggerManage::LoggerManage()
    {
        Logger::ptr root(new Logger("root"));
        LogAppender::ptr console(new ConsoleAppender());
        console->setName("console");
        root->addAppender(console);
        _loggerMap["root"] = root;
    }

    LoggerManage *LoggerManage::getInstance()
    {
        static LoggerManage instance;
        return &instance;
    }

    Logger::ptr LoggerManage::getLogger(std::string name)
    {
        std::lock_guard<SpinLock> lock(_spinlock);
        if (_loggerMap.find(name) == _loggerMap.end())
        {
            Logger::ptr logger(new Logger(name));
            logger->rootLogger = getRoot();
            _loggerMap[name] = logger;
        }
        return _loggerMap[name];
    }

    Logger::ptr LoggerManage::getRoot()
    {
        return _loggerMap["root"];
    }

    // 配置logger
    enum class AppenderType
    {
        ConsoleAppender = 1,
        FileAppender,
        DailyRollingFileAppender,
        AsyncAppender,
    };

    struct LogAppenderDefine
    {
        // append名 可以匿名，但不推荐，重名将会覆盖
        std::string name;
        // 格式，如果没有就会默认使用logger的LogFormater
        std::string format;
        // 设置类型(必须的)

        // ConsoleAppender 1
        // FileAppender 2
        // DailyRollingFileAppender 3
        //  AsyncAppender 4
        AppenderType type;
        // 非公共部分appender参数（根据类型判断是否必须）
        std::string filename;
        DailyRollingFileAppender::Rolling rolling;
        bool operator==(const LogAppenderDefine &other) const
        {
            // 非匿名时使用key作为键值
            return !name.empty() && !other.name.empty() ? name == other.name : format == other.format && type == other.type && filename == other.filename && rolling == other.rolling;
        }

        bool operator!=(const LogAppenderDefine &other) const
        {
            // 非匿名时使用key作为键值
            return !(*this== other);
        }

        struct Hash
        {
            std::size_t operator()(const LogAppenderDefine &other) const
            {
                // 非匿名时使用key作为键值
                return !other.name.empty() ? std::hash<std::string>()(other.name) : std::hash<std::string>()(other.format) ^ std::hash<std::string>()(other.filename) ^ std::hash<int>()((int)other.type) ^ std::hash<int>()((int)other.rolling);
            }
        };
    };

    struct LoggerDefine
    {
        // 日志器名，重名将会覆盖
        std::string name;
        // 日志器格式,没有就默认
        std::string format;
        // 日志器等级
        LogLevel::Level level;
        // 格式器容器
        std::unordered_set<LogAppenderDefine, LogAppenderDefine::Hash> appenderSet;
        bool operator==(const LoggerDefine &other) const
        {
            bool b = true;
            auto it1 = appenderSet.begin(), it2 = other.appenderSet.begin();
            for (; it1 != appenderSet.end() && it2 != appenderSet.end(); it1++, it2++)
            {
                if (*it1 != *it2)
                {
                    break;
                }
            }
            if (it1 != appenderSet.end())
            {
                b = false;
            }
            if (it2 != appenderSet.end())
            {
                b = false;
            }
            return name == other.name &&
                   format == other.format &&
                   level == other.level &&
                   b;
        }
        // set 通过这比较 find 和 排序
        bool operator<(const LoggerDefine &other) const
        {
            return name < other.name;
        }
    };

    template <>
    class LexicalCast<LoggerDefine, std::string>
    {
    public:
        LoggerDefine operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            LoggerDefine define;
            if (!node["name"].IsDefined())
            {
                std::cerr << "logger config error: logger name is null" << std::endl;
                throw std::logic_error("logger config error: logger name is null");
            }
            define.name = node["name"].as<std::string>();
            define.level = node["level"].IsDefined() ? LogLevel::formString(node["level"].as<std::string>())
                                                     : LogLevel::Level::Debug;
            if (node["format"].IsDefined())
            {
                define.format = node["format"].as<std::string>();
            }
            if (node["appenders"].IsDefined())
            {
                for (size_t i = 0; i < node["appenders"].size(); ++i)
                {
                    auto aNode = node["appenders"][i];
                    LogAppenderDefine appender;
                    if (!aNode["type"].IsDefined())
                    {
                        std::cerr << "logger config error: appender type name is null" << std::endl;
                        throw std::logic_error("logger config error: appender type name is null");
                    }
                    std::string type = aNode["type"].as<std::string>();
                    if (type == "ConsoleAppender")
                    {
                        appender.type = AppenderType::ConsoleAppender;
                    }
                    else if (type == "FileAppender")
                    {
                        appender.type = AppenderType::FileAppender;
                    }
                    else if (type == "DailyRollingFileAppender")
                    {
                        appender.type = AppenderType::DailyRollingFileAppender;
                        appender.rolling = aNode["rolling"].IsDefined() ? DailyRollingFileAppender::formString(aNode["rolling"].as<std::string>())
                                                                        : DailyRollingFileAppender::Rolling::DAY;
                    }
                    else if (type == "AsyncAppender")
                    {
                        appender.type = AppenderType::DailyRollingFileAppender;
                    }
                    else
                    {
                        std::cerr << "logger config error: appender type name (" << type << ") is invalid, please check up name" << std::endl;
                        throw std::logic_error("logger config error: appender type name (" + type + ")  is invalid, please check up name");
                    }

                    if (appender.type == AppenderType::FileAppender || appender.type == AppenderType::DailyRollingFileAppender)
                    {
                        if (!aNode["filename"].IsDefined())
                        {
                            std::cerr << "logger config error: file appender filename is null" << std::endl;
                            throw std::logic_error("logger config error: file appender filename is null");
                        }
                        appender.filename = aNode["filename"].as<std::string>();
                    }
                    // 可以匿名，但不推荐
                    if (aNode["name"].IsDefined())
                    {
                        appender.name = aNode["name"].as<std::string>();
                    }
                    if (aNode["format"].IsDefined())
                    {
                        appender.format = aNode["format"].as<std::string>();
                    }
                    define.appenderSet.emplace(appender);
                }
            }
            return define;
        }
    };

    template <>
    class LexicalCast<std::string, LoggerDefine>
    {
    public:
        std::string operator()(const LoggerDefine &define)
        {
            YAML::Node node;
            node["name"] = define.name;
            node["level"] = LogLevel::toString(define.level);
            node["format"] = define.format;
            for (auto &&i : define.appenderSet)
            {
                YAML::Node aNode;
                if (!i.name.empty())
                {
                    aNode["name"] = i.name;
                }
                if (define.format != i.format)
                {
                    node["format"] = i.format;
                }

                if (i.type == AppenderType::ConsoleAppender)
                {

                    node["type"] = "ConsoleAppender";
                }
                else if (i.type == AppenderType::FileAppender)
                {
                    node["type"] = "FileAppender";
                    node["filename"] = i.filename;
                }
                else if (i.type == AppenderType::DailyRollingFileAppender)
                {
                    node["type"] = "DailyRollingFileAppender";
                    node["filename"] = i.filename;
                    node["rolling"] = DailyRollingFileAppender::toString(i.rolling);
                }
                else if (i.type == AppenderType::AsyncAppender)
                {
                    node["type"] = "AsyncAppender";
                }
                node["appenders"].push_back(aNode);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    static ConfigVar<std::set<LoggerDefine>>::ptr logsConfig = Configurator::lookup("logs", std::set<LoggerDefine>(),
                                                                                          "Log configuration. The root logger exists by default,"
                                                                                          " and to modify it, a declaration needs to be displayed");
    struct LogInit
    {
        LogInit()
        {
            logsConfig->addChangeValueListener([](const std::set<LoggerDefine> &oldValue, const std::set<LoggerDefine> &newValue)
                                               {
                Logger::ptr logger;
                std::cout<<"logger config change "<<std::endl;
                for(auto& v:newValue){
                    auto it=oldValue.find(v);
                    if(it!=oldValue.end()){
                        //完全相同的话，就没必要了
                        if(v==*it){
                            continue;
                        }
                    }
                    logger=SRV_LOGGER_NAME(v.name);
                    logger->setLevel(v.level);
                    if(v.format!=""){
                        logger->setLogFormater(v.format);
                    }
                    logger->clearAppender();
                    for (auto &&i : v.appenderSet)
                    {
                        LogAppender::ptr spAppender;
                        switch (i.type)
                        {
                        case AppenderType::ConsoleAppender:
                            spAppender.reset(new ConsoleAppender(i.name));
                            break;
                        case AppenderType::FileAppender:
                            spAppender.reset(new FileAppender(i.filename,i.name));
                            break;
                        case AppenderType::DailyRollingFileAppender:
                            spAppender.reset(new DailyRollingFileAppender(i.filename,i.rolling,i.name));
                            break;
                        case AppenderType::AsyncAppender:
                            spAppender.reset(new AsyncAppender(i.name));
                            break;
                        }
                        if(i.format!=""){
                            spAppender->setFormater(LogFormater::ptr(new LogFormater(i.format)));
                        }
                        logger->addAppender(spAppender);
                    }
                }

                for(auto v:oldValue){
                    if(newValue.find(v)==newValue.end()){
                        //移除logger,但把root置为默认
                        auto logger=SRV_LOGGER_NAME(v.name);
                        logger->clearAppender();
                        if(v.name=="root"){
                            logger->addAppender(LogAppender::ptr(new ConsoleAppender()));
                            logger->setLevel(LogLevel::Debug);
                            continue;
                        }
                        logger->setLevel(LogLevel::Off);
                    }
                } });
        }
    };

    static LogInit __LogInit;
} // namespace WebSrv
