#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "mutex.h"
#include "yaml-cpp/yaml.h"
#include "util.h"
#include "log.h"
#include <algorithm>
#include <cctype>
namespace WebSrv
{


    // 配置变量基类
    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;
        ConfigVarBase(std::string name, std::string describe):_name(name),_describe(describe){
            std::transform(_name.begin(), _name.end(), _name.begin(), ::tolower);
        }
        /**
         * @brief 将内部参数变量，转化为对应配置文件字符串接口
         *
         * @return std::string
         */
        virtual std::string toString() = 0;

        /**
         * @brief 配置文件字符串转化成对应内部参数变量
         *
         * @return true
         * @return false
         */
        virtual bool formString(const std::string &str) = 0;
        /**
         * @brief 返回配置参数名
         *
         * @return std::string
         */
        std::string getName()
        {
            return _name;
        }

        /**
         * @brief 返回配置参数描述
         *
         * @return std::string
         */
        std::string GetDescribe()
        {
            return _describe;
        }

        /**
         * @brief 返回参数类型
         * 
         * @return std::string 
         */
        virtual std::string getTypeName() const=0;
    private:
        // 配置参数名
        std::string _name;
        // 配置描述
        std::string _describe;
    };

    // yaml string和各容器之间相互转换

    /**
     * @brief 将类型V转换成类型T
     *
     * @tparam T 转换类型
     * @tparam V 待转换类型
     */
    template <typename T, typename V>
    class LexicalCast
    {
    public:
        T operator()(const V &v)
        {
            return lexicalCast<T, V>(v);
        }
    };

    /**
     * @brief 特化yaml string 转 std::vector<T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (size_t i = 0; i < node.size(); i++)
            {
                // 清空流字符
                ss.str("");
                ss << node[i];
                res.emplace_back(LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::vector<T> 转 yaml string
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<std::string, T>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 特化yaml string 转 std::list<T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (size_t i = 0; i < node.size(); i++)
            {
                // 清空流字符
                ss.str("");
                ss << node[i];
                res.emplace_back(LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::list<T> 转 yaml string
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &it : v)
            {
                node.push_back(YAML::Load(LexicalCast<std::string, T>()(it)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 特化yaml string 转 std::set<T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (size_t i = 0; i < node.size(); i++)
            {
                // 清空流字符
                ss.str("");
                ss << node[i];
                res.emplace(LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::set<T> 转 yaml string
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &it : v)
            {
                node.push_back(YAML::Load(LexicalCast<std::string, T>()(it)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 特化yaml string 转 std::map<std::string,T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (auto it = node.begin(); it != node.end(); it++)
            {
                // 清空流字符
                ss.str("");
                ss << it->second;
                res.emplace(it->first.Scalar(), LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::map<std::string,T> 转 yaml string
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto [key, value] : v)
            {
                node[key] = YAML::Load(LexicalCast<std::string, T>()(value));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 特化yaml string 转 std::unordered_set<T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (size_t i = 0; i < node.size(); i++)
            {
                // 清空流字符
                ss.str("");
                ss << node[i];
                res.emplace(LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::unordered_set<T> 转 yaml string
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &it : v)
            {
                node.push_back(YAML::Load(LexicalCast<std::string, T>()(it)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief 特化yaml string 转 std::unordered_map<std::string,T>
     *
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> res;
            std::stringstream ss;
            // 先转成字符流在转成对应类型
            for (auto it = node.begin(); it != node.end(); it++)
            {
                // 清空流字符
                ss.str("");
                ss << it->second;
                res.emplace(it->first.Scalar(), LexicalCast<T, std::string>()(ss.str()));
            }
            return res;
        }
    };

    /**
     * @brief 特化 std::unordered_map<std::string,T> 转 yaml string ,
     *
     * @warning 因为yaml没有unordered_map,所以key对应唯一值，如果有非唯一值会导致前面的value数据丢失
     * @tparam T 转换类型
     */
    template <typename T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto [key, value] : v)
            {
                node[key] = YAML::Load(LexicalCast<std::string, T>()(value));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief ConfigVarBase 子类，提供设置/修改配置的值，
     *  （通过使用回调函数，对配置项设置/修改）
     *   默认使用yaml作为格式转换器
     * @tparam T 配置值
     * @tparam ToStr   将配置值转换成对应格式的字符串
     * @tparam FormStr 对应格式的字符串转换成格式值
     */
    template <typename T, typename ToStr = LexicalCast<std::string, T>, typename FormStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar>;
        // 当参数值发生变化时，调用，一般用于设置/修改对应配置项的参数值
        using ChangeCallback = std::function<void(const T &oldValue, const T &newValue)>;


        ConfigVar(const std::string &name, const T &defaultVal, const std::string &describe)
            : ConfigVarBase(name, describe), _value(defaultVal)
        {
        }
        /**
         * @brief 将内部参数变量，转化为对应配置文件字符串接口
         *  默认转换成yaml string
         * @return std::string
         */
        virtual std::string toString() override
        {
            try
            {
                ReadMutex lock(_rwLock);
                return ToStr()(_value);
            }
            catch (const std::exception &e)
            {
                SRV_LOG_ERROR(SRV_LOGGER_ROOT()) << e.what() << " name: " << getName() << '\n';
            }
            return "";
        }

        /**
         * @brief 配置文件字符串转化成对应内部参数变量
         * 默认使用yaml string 进行设置
         * @return true 设置成功
         * @return false 设置失败
         */
        bool formString(const std::string &str) override
        {
            try
            {
                setValue(FormStr()(str));
                return true;
            }
            catch (const std::exception &e)
            {
                SRV_LOG_ERROR(SRV_LOGGER_ROOT()) << e.what() << " by ConfigVar name: " << getName() << '\n';
            }
            return false;
        }

        /**
         * @brief 直接通过参数值设置
         *
         * @param val
         */
        void setValue(const T &val)
        {
            {
                ReadMutex lock(_rwLock);
                if (_value == val)
                {
                    return;
                }
            }

            WriteMutex lock(_rwLock);
            for (auto &[id, listener] : _listeners)
            {
                listener(_value, val);
            }

            _value = val;
        }

        const T getValue() 
        {
            ReadMutex lock(_rwLock);
            return _value;
        }

        std::string getTypeName() const{
            return TypeToName<T>();
        }

        /**
         * @brief 添加参数值发生变化监听器，并为它设置唯一id
         *
         */
        void addChangeValueListener(ChangeCallback listener)
        {
            WriteMutex lock(_rwLock);
            // 参数量不大，一般来说运行就定死，不需要搞回收id什么的
            static uint64_t s_funId = 0;
            s_funId++;
            _listeners.emplace(s_funId,listener);
        }

        /**
         * @brief 通过参数删除监听器
         *
         * @param funId
         */
        void delChangeValueListener(uint64_t funId)
        {
            WriteMutex lock(_rwLock);
            auto it=_listeners.find(funId);
            if(it!=_listeners.end()){
                _listeners.erase(it);
            }
        }

        /**
         * @brief 清除所有监听器
         *
         */
        void clearChangeValueListener()
        {
            WriteMutex lock(_rwLock);
            _listeners.clear();
        }

    private:
        // 共享锁（读写锁）
        RWMutex _rwLock;
        T _value;
        // 配置变更事件,储存所有修改配置时调用的回调函数 一个key,对应一个回调（unordered_map可以一对多，不过自分配id到不需考虑）
        std::unordered_map<uint64_t, ChangeCallback> _listeners;
    };

} // namespace WebSrv
