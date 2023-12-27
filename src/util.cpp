#include "util.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <ctime>
#include <regex>
namespace WebSrv
{
    uint32_t getSystemTheadId()
    {
        return syscall(SYS_gettid);
    }

    uint64_t GetProcessStartTime()
    {
        pid_t pid = getpid();
        std::string path = "/proc/" + std::to_string(pid) + "/stat";

        std::ifstream statFile(path);
        if (statFile.is_open())
        {
            // 读取文件内容
            std::string line;
            std::getline(statFile, line);

            // 关闭文件
            statFile.close();

            // 解析启动时间
            std::istringstream iss(line);
            std::string token;

            // 跳过前21个字段，直到第22个字段是启动时间
            for (int i = 0; i < 21; ++i)
            {
                iss >> token;
            }
            // 将starttime的单位从时钟（clock ticks）转换为毫秒
            // 读取启动时间
            std::time_t startTime = std::stol(token) / sysconf(_SC_CLK_TCK);

            return getCurrentMilliseconds()-startTime;
        }
        else
        {
            std::cerr << "get process start time is failed" << std::endl;
            return 0;
        }
    }

    uint64_t getCurrentMilliseconds()
    {
        auto now = SystemClock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }


    void backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        const int maxFrames = size;
        void *callStack[maxFrames];
        int frames = ::backtrace(callStack, maxFrames);
        char **symbols = backtrace_symbols(callStack, frames);

        for (int i = skip; i < frames; ++i)
        {
            bt.push_back(symbols[i]);
        }

        free(symbols);
    }

    bool FileSystemUtil::createDirectory(const std::string &path)
    {
        struct stat st;
        if (stat(path.c_str(), &st) == 0)
        {
            // 文件或目录存在
            // 判断是文件还是目录
            if (S_ISDIR(st.st_mode))
            {
                return true;
            }
            else
            {
                return false; // 非目录，同名无法创建
            }
        }
        else
        {
            auto pre1 = path.rfind('/');
            auto pre2 = path.rfind('\\');
            auto i = (pre1 != std::string::npos && pre2 != std::string::npos) ? std::max(pre1, pre2) : (pre1 == std::string::npos ? pre2 : pre1);
            if (i != std::string::npos)
            {
                std::string parentPath = path.substr(0, i);
                if (createDirectory(parentPath))
                {
                    // 父目录创建成功，创建当前子目录
                    int status = mkdir(path.c_str(),0777);
                    return (status == 0);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (path == "" || path.rfind(':') != std::string::npos)
                {
                    // 可能为 '/xx/xx'或者为盘符,往上回溯  为/xxx会创建到c盘
                    return true;
                }
                int status = mkdir(path.c_str(),0777);
                return (status == 0);
            }
        }
    }
    bool FileSystemUtil::openFileForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode)
    {
        ofs.open(filename, mode);
        // 如果打开失败，可能是目录未创建，尝试创建目录
        if (!ofs.is_open())
        {
            std::filesystem::path filepath(filename);
            std::filesystem::create_directories(filepath.parent_path());
            ofs.open(filename, mode);
        }
        return ofs.is_open();
    }
    std::pair<int, int> FileSystemUtil::getFileExistsAndParentPos(const std::string &filename)
    {
        auto pre1 = filename.rfind('/');
        auto pre2 = filename.rfind('\\');
        auto i = (pre1 != std::string::npos && pre2 != std::string::npos) ? std::max(pre1, pre2) : (pre1 == std::string::npos ? pre2 : pre1);
        if (i == std::string::npos)
        {
            auto dotPosition = filename.find_first_of('.');
            if (dotPosition != std::string::npos)
            {
                return {dotPosition + 1, 0};
            }
        }
        else
        {
            auto dotPosition = filename.find_first_of('.', filename.size() - i - 1);
            if (dotPosition != std::string::npos)
            {
                return {dotPosition + 1, i};
            }
        }
        return {-1, i != std::string::npos ? i : 0};
    }

    std::list<std::filesystem::path> FileSystemUtil::getFileWithSuffix(const std::string &dirpath, const std::string &extension)
    {
        std::list<std::filesystem::path> pathList;
        for (auto it : std::filesystem::directory_iterator(dirpath))
        {
            if (it.is_regular_file() && it.path().extension() == extension)
            {
                pathList.emplace_back(it.path());
            }
        }
        return pathList;
    }

    std::string time2ToStr(time_t time, const char *fmt)
    {
        tm* timeInfo=std::gmtime(&time);
        std::stringstream ss;
        ss<<std::put_time(timeInfo,fmt);
        return ss.str();
    }

    std::string timeToStr(time_t time, const char *fmt)
    {
        tm* timeInfo=std::localtime(&time);
        std::stringstream ss;
        ss<<std::put_time(timeInfo,fmt);
        return ss.str();
    }

    time_t timeFromStr(const char *str, const char *fmt)
    {
        tm t{};
        std::istringstream input(str);
        input.imbue(std::locale(setlocale(LC_ALL,nullptr)));
        input>>std::get_time(&t,fmt);
        if(input.fail()){
            return 0;
        }
        return mktime(&t);
    }

    static int64_t GMTAndUTCTimeDifference(){
        // 获取当前系统时间点
        auto now = std::chrono::system_clock::now();
        // 将系统时间点转换为 time_t
        time_t currentTime = std::chrono::system_clock::to_time_t(now);
        // 使用 gmtime 获取格林尼治时间
        struct tm *gmtTime = std::gmtime(&currentTime);
        return currentTime- mktime(gmtTime);
    }

    time_t time2FromStr(const char *str, const char *fmt)
    {
        tm t{};
        std::istringstream input(str);
        input.imbue(std::locale(setlocale(LC_ALL,nullptr)));
        input>>std::get_time(&t,fmt);
        if(input.fail()){
            return 0;
        }
        auto difference =GMTAndUTCTimeDifference();
        return difference+mktime(&t);
    }

    void splitStr(char *s, std::vector<std::string> &res, const char *delimiters)
    {
        char *save;
        char *token = strtok_r(s, delimiters, &save);
        if (!save)
        {
            return;
        }
        while (token != nullptr)
        {
            res.emplace_back(token);
            token = strtok_r(nullptr, delimiters, &save);
        }
    }

    void split2Str(char *s, std::vector<char *> &res, const char *delimiters)
    {
        char *save;
        char *token = strtok_r(s, delimiters, &save);
        if (!save)
        {
            return;
        }
        while (token != nullptr)
        {
            res.emplace_back(token);
            token = strtok_r(nullptr, delimiters, &save);
        }
    }

    void split3Str(const char *s, std::vector<std::string> &res, const char *delimiters)
    {
        size_t delimitersLen=strlen(delimiters);
        size_t size=strlen(s);
        if(size==0){
            return;
        }
        size_t skipLen=0;
        size_t len;
        const  char* token=strstr(s,delimiters);
        while(token!=nullptr){
            len=token-s;
            if(len>0){
                res.emplace_back(std::string(s+skipLen,len-skipLen));
            }
            skipLen=len+delimitersLen;
            if(skipLen>=size){
                break;
            }
            token=strstr(s+skipLen,delimiters);
        }
        if (skipLen!=0&&skipLen < size)
        {
            res.emplace_back(s + skipLen, size - skipLen);
        }
    }

    bool parseURL(const std::string &url, URL &res)
    {
        std::regex urlRegex(R"((\w+):\/\/([^\/]+)([^?#]*)(\?([^#]*))?(#(.*))?)");

        std::smatch matches;
        if (std::regex_match(url, matches, urlRegex))
        {
            res.protocol = matches[1].str();
            res.host = matches[2].str();
            res.path = matches[3].str();
            res.query = matches[5].str();
            res.fragment = matches[7].str();
            return true;
        }
        return false;
    }

    bool parse2URL(const std::string &url, URL &res)
    {
        std::regex urlRegex("^([^?#]*)\\??([^#]*)#?(.*)$");

        std::smatch matches;
        if (std::regex_match(url, matches, urlRegex))
        {
            res.path = matches[1].str();
            res.query = matches[2].str();
            res.fragment = matches[3].str();
            return true;
        }
        return false;
    }

} // namespace WebSrv
