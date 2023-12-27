#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <string>
#include <iostream>
#include <cxxabi.h>
#include <filesystem>
#include <vector>
#include <list>

namespace WebSrv
{
    using SystemClock = std::chrono::system_clock;

    uint32_t getSystemTheadId();

    uint64_t GetProcessStartTime();
    /**
     * @brief 文件处理系统工具
     * 
     * @warning 注意部分成员函数处理使用 c++17 std::filesystem 废弃
     * 
     */
    class FileSystemUtil
    {
    public:
        /**
         * @brief  多级创建文件夹，如果文件夹不存在将创建
         *
         * 
         * @param path 路径
         * @return true 创建成功\文件夹以存在
         * @return false 创建失败\路径存在但不是目录
         */
        static bool createDirectory(const std::string &path);

        static bool openFileForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode);
        /**
         * @brief 获取文件第一个后缀开始位置（第一个.开始）和parent_path的末位置（相当于获取文件名的前后位置）
         *  
         * 
         * @return first 后缀 为-1没有后缀，second  父路径位置没有为0
         */
        static std::pair<int, int> getFileExistsAndParentPos(const std::string &filename);

        /**
         * @brief 获取文件后缀
         * 
         * @param dirpath 
         * @param extension 
         * @return std::list<std::filesystem::path> 
         */
        static std::list<std::filesystem::path> getFileWithSuffix(const std::string& dirpath,const std::string& extension);
    };

    template <typename T>
    const char *TypeToName()
    {   //设置静态，放置重复解析带来资源的消耗
        static const char *typeName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
        return typeName;
    }

    /**
     * @brief std::string 转换成基础类型
     *
     * @tparam T
     * @param str
     * @return T
     */
    template <typename T>
    T fromString(const std::string &str)
    {
        try
        {
            std::istringstream iss(str);
            T res;
            iss >> res;

            if (iss.fail() || !iss.eof())
            {
                throw std::invalid_argument(std::string("Failed to convert string to type ") + TypeToName<T>());
            }

            return res;
        }
        catch (const std::exception &e)
        {
            throw std::invalid_argument(std::string("Failed to convert string to type ") + TypeToName<T>());
        }
    }

    /**
     * @brief std::string 转换成基础类型bool，不区分大小写
     *
     * @tparam  特化版本bool
     * @param str
     * @return true
     * @return false
     */
    template <>
    inline bool fromString<bool>(const std::string &str)
    {
        std::string res = str;
        // 转小写
        for (auto &c : res)
        {
            c = std::tolower(c);
        }
        if (res == "true" || res == "1")
        {
            return true;
        }
        else if (res == "false" || res == "0")
        {
            return false;
        }
        else
        {
            throw std::invalid_argument("Failed to convert string to type bool");
        }
    }

    /**
     * @brief 将S类型转换成T类型
     * 
     * @tparam T
     * @tparam S
     * @param src
     * @return T
     */
    template <typename T, typename S>
    T lexicalCast(const S &src)
    {
        std::stringstream ss;
        T res;
        ss << src;
        ss >> res;

        if (ss.fail() || !ss.eof())
        {
            throw std::runtime_error(std::string("Failed to perform lexical_cast ") + TypeToName<T>() + " from " + TypeToName<S>());
        }

        return res;
    }

    template <>
    inline bool lexicalCast(const std::string &src)
    {
        return  fromString<bool>(src);
    }
    //获取毫秒级时间戳
    uint64_t getCurrentMilliseconds();


    /**
     * @brief 获取当前调用栈
     * 
     * @param bt 保存调用栈
     * @param size 最多返回层数
     * @param skip 跳过栈顶的层数
     * @warning mingw + windows下暂时无法使用，因为它无法直接解析栈信息，需要通过addr2line，我对cmake理解很浅，暂时无法将它的信息转为addr2line解析
     * 实现可参考https://cloud.tencent.com/developer/article/1173442
     */
    void backtrace(std::vector<std::string>& bt,int size=64,int skip=1);

    /**
     * @brief 将时间戳转成特定格式字符串
     *
     * @param time
     * @param fmt
     * @return std::string
     */
    std::string timeToStr(time_t time, const char *fmt = "%Y-%m-%d %H:%M:%S");

    /**
     * @brief 将时间戳转成特定格式字符串 (GMT时间)
     * 
     * @param time 
     * @param fmt 
     * @return std::string 
     */
    std::string time2ToStr(time_t time,const char* fmt="%Y-%m-%d %H:%M:%S");


    /**
     * @brief 将特定格式的字符串转成时间戳
     * 
     * @param str 
     * @param fmt 
     * @return time_t 
     */
    time_t timeFromStr(const char* str,const char* fmt);

        /**
     * @brief 将特定格式的字符串(GMT时间)转成时间戳(UTC)
     * 
     * @param str 
     * @param fmt 
     * @return time_t 
     */
    time_t time2FromStr(const char* str,const char* fmt);
    /**
     * @brief 使用指定分隔符，分割字符串
     * 
     * @param s 待分割字符(会添加\0导致字符串被破坏)
     * @param res 返回分割字符串数组
     * @param split 分割符
     */
    void splitStr(char* s,std::vector<std::string>& res,const char* delimiters);


    /**
     * @brief 使用指定分隔符，分割字符串
     * 
     * @param s 待分割字符(会添加\0导致字符串被破坏)
     * @param res 返回分割字符串的头指针数组
     * @param split 分割符
     */
    void split2Str(char* s,std::vector<char*>& res,const char* delimiters);

    /**
     * @brief 使用指定分隔符，分割字符串
     * 
     * @param s 待分割字符(不会破坏字符串结构)
     * @param res 返回分割字符串数组
     * @param split 分割符
     */
    void split3Str(const char* s,std::vector<std::string>& res,const char* delimiters);
    
    struct URL{
        std::string protocol;
        std::string host;
        std::string path;
        std::string query;
        std::string fragment;
    };

    /**
     * @brief 解析url
     * 
     * @param url 
     * @param res 
     * @return true 
     * @return false 
     */
    bool parseURL(const std::string& url,URL& res);
    /**
     * @brief 解析url相对路径（不包含主机名和协议）
     * 
     * @param url 
     * @param res 
     * @return true 
     * @return false 
     */
    bool parse2URL(const std::string& url,URL& res);
} // namespace WebSrv
