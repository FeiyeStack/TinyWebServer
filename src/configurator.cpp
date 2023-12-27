#include "configurator.h"
#include <list>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <filesystem>
namespace WebSrv
{

    // a:
    //    b:10
    //    c:30
    // e: str

    // 递归拆解root成员
    void toAllLisMember(const YAML::Node &root, std::list<std::pair<std::string, YAML::Node>> &out_nodes, const std::string &prefix = "")
    {
        //检查命名是否规范，第一个前缀是空的所以可以允许空字符
        std::string pattern = "^[a-zA-Z1-9._]*$";
        std::regex regexPattern(pattern);
        if (!std::regex_match(prefix, regexPattern))
        {
            SRV_LOG_ERROR(SRV_LOGGER_ROOT())<<"Improper naming \""<<prefix<<"\"";
            return;
        }
        out_nodes.push_back({prefix, root});
        // prefix 为判断位于节点树的位置信息  a.b  a.c e
        // 如果是map取出成员，一层层加入配置节点
        if (root.IsMap())
        {
            for (auto it = root.begin(); it != root.end(); it++)
            {
                toAllLisMember(it->second, out_nodes, prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar());
            }
        }
    }

    void Configurator::loadConfigYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, YAML::Node>> all_node;
        toAllLisMember(root, all_node);
        for (auto [key, value] : all_node)
        {
            // 忽略根节点（也是忽略非map）
            if (key.empty())
            {
                continue;
            }
            
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr it = lookupBase(key);
            if(it==nullptr){
                continue;
            }
            
            if (value.IsScalar())
            {
                it->formString(value.Scalar());
            }
            else
            {
                std::stringstream ss;
                ss << value;
                it->formString(ss.str());
            }
        }
    }

    std::unordered_map<std::string, std::filesystem::file_time_type> fileLastWTimeMap;

    void Configurator::loadConfigDir(const std::string &dirpath, bool force)
    {
        auto filepathList = FileSystemUtil::getFileWithSuffix(dirpath, "yaml");
        for (auto filepath : filepathList)
        {
            // 转成绝对路径
            filepath = std::filesystem::absolute(filepath);
            auto lastWriteTime = std::filesystem::last_write_time(filepath);
            if (force && lastWriteTime == fileLastWTimeMap[filepath.string()])
            {
                loadConfigFile(filepath.string());
                fileLastWTimeMap[filepath.string()] = lastWriteTime;
            }
        }
    }

    void Configurator::loadConfigFile(const std::string &filepath)
    {
        try
        {
            YAML::Node node = YAML::LoadFile(filepath);
            loadConfigYaml(node);
            SRV_LOG_INFO(SRV_LOGGER_ROOT()) << "load config file " << filepath << " success";
        }
        catch (...)
        {
            SRV_LOG_ERROR(SRV_LOGGER_ROOT()) << "open config file " << filepath << " fail";
        }
    }

    typename ConfigVarBase::ptr Configurator::lookupBase(const std::string &name)
    {
        ReadMutex lock(getRWMutex());
        auto it = getConfigVarMap().find(name);
        return it != getConfigVarMap().end() ? it->second : nullptr;
    }

    void Configurator::visit(std::function<void(ConfigVarBase::ptr)> callback)
    {
        ReadMutex lock(getRWMutex());
        for(auto&& [key,value]:getConfigVarMap()){
            callback(value);
        }
    }

    Configurator::ConfigVarMap &Configurator::getConfigVarMap()
    {
        static ConfigVarMap configVarMap;
        return configVarMap;
    }

    RWMutex &WebSrv::Configurator::getRWMutex()
    {
        static RWMutex rwMutex;
        return rwMutex;
    }

} // namespace name
