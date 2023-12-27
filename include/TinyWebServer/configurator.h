#pragma once
#include "configvar.h"
#include "mutex.h"
#include <regex>
#include "log.h"
namespace WebSrv
{

    class Configurator
    {   
        // 对于配置器只需要创建和读取即可，改变在配置值中，防止对预设值的结构意外修改，和不必要的资源开销
        // 当在模板中使用依赖于模板参数的嵌套类型时，有时编译器无法确定是否是一个类型,需要使用 typename 显式声明。
    public:
        using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

        /**
         * @brief 创建或获取ConfigVar,如果存在就获取ConfigVar
         *
         * @tparam T 配置类型
         * @param name 配置项名 命名只允许使用该范围内字符 a-z A-Z 1-9 .
         * @param default_value 默认配置值
         * @param describe 描述
         * @throw std::invalid_argument 用户名使用非法字符
         * @return ConfigVar<T>::ptr 获取配置项
         */
        template <typename T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name, const T &default_value, std::string describe = "")
        {   
            WriteMutex lock(getRWMutex());
            auto it = getConfigVarMap().find(name);
            if (it != getConfigVarMap().end())
            {
                // ConfigVarBase::ptr 转换成ConfigVar::ptr
                auto subPtr = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (subPtr != nullptr)
                {
                    SRV_LOG_INFO(SRV_LOGGER_ROOT()) << "look up name=" << name << "exists, return the name ConfigVar";
                    return subPtr;
                }
                //存在但不是你想要创建的类型
                SRV_LOG_INFO(SRV_LOGGER_ROOT()) << "look up name=" << name << "exists,But it's not the type you want <"
                                                << TypeToName<T>() <<"> . Its type is " << it->second->getTypeName() << ":" << it->second->toString();
                return nullptr;
            }

            std::string pattern = "^[a-zA-Z0-9._]+$";
            std::regex regexPattern(pattern);
            if(!std::regex_match(name,regexPattern)){
                //命名规则非法
                SRV_LOG_ERROR(SRV_LOGGER_ROOT())<<"create name="<<name<<" Illegal naming";
                throw std::invalid_argument(name+" using illegal naming");
            }
            typename ConfigVar<T>::ptr sp(new ConfigVar<T>(name,default_value,describe));
            getConfigVarMap().emplace(name,sp);
            return sp;
        }

        /**
         * @brief 获取ConfigVar
         *
         * @tparam T 配置类型
         * @param name 配置项名
         * @return ConfigVar<T>::ptr  不存在返回nullptr
         */
        template <typename T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name)
        {
            ReadMutex lock(getRWMutex());
            auto it=getConfigVarMap().find(name);
            if(it!=getConfigVarMap().end()){
                return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            }
            return nullptr;
        }

        /**
         * @brief 通过yaml node 加载配置
         *
         * @param node
         * @return * void
         * @warning 配置项存在才会加载,而且只加载map
         */
        static void loadConfigYaml(const YAML::Node &root);
        /**
         * @brief 通过文件夹路径加载配置字符串文件(yaml),配置项存在才会加载
         *
         * @param dirpath 配置文件夹路径，会检索其中包含.yaml的文件
         * @param force 是否强制加载，（发生文件变化时可用）
         */
        static void loadConfigDir(const std::string &dirpath, bool force);

        //

        /**
         * @brief 加载配置字符串文件 (yaml)
         *
         * @param filepath 配置文件路径
         */
        static void loadConfigFile(const std::string &filepath);

        // 获取ConfigVar基类
        static typename ConfigVarBase::ptr lookupBase(const std::string &name);
        /**
         * @brief 回调函数，访问使用配置信息
         * 
         */
        static void visit(std::function<void(ConfigVarBase::ptr)> callback);
    private:
        // 单例配置map
        static ConfigVarMap &getConfigVarMap();
        // 单例读写锁
        static RWMutex &getRWMutex();

    };
} // namespace WebSrv
