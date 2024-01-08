#include <TinyWebServer/http/httpserver.h>
#include <TinyWebServer/log.h>

static WebSrv::Logger::ptr g_logger = SRV_LOGGER_ROOT();
const int MAX_RANGE= 500*1024*1024;

struct Location
{
    using ptr = std::shared_ptr<Location>;
    std::string path;    // 请求资源位置
    std::string root;    // 请求目录
    std::string index;   // 根目录引索
    bool isGlob = false; // 是否模糊匹配
};

class HttpStaticFile
{
public:
    using ptr = std::shared_ptr<HttpStaticFile>;
    // using HeaderMethod = std::function<int(const std::string& value,
    //                                        WebSrv::http::HttpResponse::ptr response)>;
    HttpStaticFile(WebSrv::http::HttpServer::ptr httpServer)
        : _httpServer(httpServer)
    {
    }

    void addLocation(Location::ptr location)
    {
        if (!location || location->path.empty())
        {
            return;
        }
        auto servletDispatch = _httpServer->getServletDispatch();
        if (!location->isGlob)
        {
            servletDispatch->addServlet(location->path,
                                        [this, location, name = _httpServer->getName()](WebSrv::http::HttpRequest::ptr request,
                                                                                        WebSrv::http::HttpResponse::ptr response,
                                                                                        WebSrv::http::HttpSession::ptr session)
                                        {
                                            int ret = handlePath(location, request, response);
                                            if (ret > 0)
                                            {
                                                WebSrv::http::HttpStatus status = (WebSrv::http::HttpStatus)ret;
                                                response->setStatus(status);
                                                response->setHeader("Server", name);
                                                response->setHeader("Content-Type", "text/html");
                                                std::stringstream ss;
                                                ss << "<html><head><title>"
                                                   << ret << " " << WebSrv::http::HttpStateToString(status)
                                                   << "</title></head><body><center><h1>404 Not Found</h1></center>"
                                                   << "<hr><center>" << name << "</center></body></html>";
                                                response->setBody(ss.str());
                                            }
                                            return 0;
                                        });
        }
        else
        {
            servletDispatch->addGlobServlet(location->path + "*",
                                            [this, location, name = _httpServer->getName()](WebSrv::http::HttpRequest::ptr request,
                                                                                            WebSrv::http::HttpResponse::ptr response,
                                                                                            WebSrv::http::HttpSession::ptr session)
                                            {
                                                int ret = handlePath(location, request, response);
                                                if (ret > 0)
                                                {
                                                    WebSrv::http::HttpStatus status = (WebSrv::http::HttpStatus)ret;
                                                    response->setStatus(status);
                                                    response->setHeader("Server", name);
                                                    response->setHeader("Content-Type", "text/html");
                                                    std::stringstream ss;
                                                    ss << "<html><head><title>"
                                                       << ret << " " << WebSrv::http::HttpStateToString(status)
                                                       << "</title></head><body><center><h1>404 Not Found</h1></center>"
                                                       << "<hr><center>" << name << "</center></body></html>";
                                                    response->setBody(ss.str());
                                                }
                                                return 0;
                                            });
        }
        std::lock_guard lock(_mutex);
        locations[location->path] = location;
    }

    void delLocation(const std::string &location)
    {
        if (location.empty())
        {
            return;
        }
        auto servletDispatch = _httpServer->getServletDispatch();

        std::lock_guard lock(_mutex);

        auto it = locations.find(location);
        if (it == locations.end())
        {
            return;
        }

        if (it->second->isGlob)
        {
            servletDispatch->delGlobServlet(it->first);
        }
        else
        {
            servletDispatch->delServlet(it->first);
        }

        locations.erase(it);
    }

    Location::ptr getLocation(const std::string &location)
    {
        std::lock_guard lock(_mutex);
        auto it = locations.find(location);
        return it != locations.end() ? it->second : nullptr;
    }

private:
    int handlePath(Location::ptr location,
                   WebSrv::http::HttpRequest::ptr request,
                   WebSrv::http::HttpResponse::ptr response)
    {

        std::string path = location->root + (request->getPath() == location->path ? "/index." + location->index : request->getPath());
        std::filesystem::path srcpath(path);
        if (!std::filesystem::exists(srcpath))
        {
            return 404;
        }

        static std::map<std::string, std::string> mimeTypes = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".mp4", "video/mp4"},
        };

        auto it = mimeTypes.find(srcpath.extension().string());

        if (it != mimeTypes.end())
        {
            response->setHeader("Content-Type", it->second);
        }
        else
        {
            response->setHeader("Content-Type", "application/octet-stream");
        }

        std::string str;
        int ret = 0;

        ret = headerMethod(srcpath, request, response);
        if (ret > 1)
        {
            return ret;
        }

        if (ret == 0)
        {
            std::ifstream is(srcpath.c_str(), std::ios_base::binary);
            if (!is.is_open())
            {
                return 404;
            }
            std::stringstream ss;
            ss << is.rdbuf();
            is.close();
            response->setBody(ss.str());
        }

        return 0;
    }

    int headerMethod(const std::filesystem::path &path,
                     WebSrv::http::HttpRequest::ptr request,
                     WebSrv::http::HttpResponse::ptr response)
    {
        std::string val;
        int ret = 0;

        if (request->hasHeader("Range", &val))
        {
            RangeHeader(path, val, response);
            ret = 1;
        }
        return ret;
    }

    int RangeHeader(const std::filesystem::path &path, const std::string &range, WebSrv::http::HttpResponse::ptr response)
    {
        std::ifstream is(path.c_str(), std::ios::binary | std::ios::ate);

        if (is.is_open())
        {
            // 获取文件大小
            size_t fileSize = is.tellg();

            if (!range.empty())
            {
                size_t start = range.find("bytes=");
                if (start != std::string::npos)
                {
                    start += 6; // 跳过"bytes="

                    size_t hyphenPos = range.find('-', start);
                    if (hyphenPos != std::string::npos)
                    {
                        std::string rangeStr = range.substr(start, hyphenPos - start);
                        size_t rangeStart = std::stoul(rangeStr);
                        rangeStr = range.substr(hyphenPos + 1);
                        if (rangeStart >= fileSize)
                        {
                            rangeStart = fileSize;
                        }

                        size_t rangeEnd = rangeStr.empty() ? fileSize : std::stoul(rangeStr);


                        if (rangeEnd >= fileSize)
                        {
                            rangeEnd = fileSize;
                        }

                        

                        size_t readSize = rangeEnd - rangeStart;
                        if(readSize>MAX_RANGE){
                            readSize=MAX_RANGE;
                            rangeEnd=rangeStart+readSize;
                        }
                        std::string content;
                        content.resize(readSize);

                        is.seekg(rangeStart, std::ios::beg);
                        is.read(content.data(), readSize);
                        is.close();
                        response->setBody(content);
                        response->setStatus((WebSrv::http::HTTP_STATUS_PARTIAL_CONTENT));
                        response->setHeader("Content-Range", "bytes " + std::to_string(rangeStart) + "-" + std::to_string(rangeEnd - 1) + "/" + std::to_string(fileSize));
                        //SRV_LOG_DEBUG(g_logger)<<fileSize<<": bytes " + std::to_string(rangeStart) + "-" + std::to_string(rangeEnd - 1) + "/" + std::to_string(fileSize);
                        
                        return 1;
                    }
                }
            }
            is.close();
        }
        return 0;
    }

private:
    WebSrv::http::HttpServer::ptr _httpServer; // 执行服务器
    std::map<std::string, Location::ptr> locations;
    std::mutex _mutex;
};

void run(WebSrv::http::HttpServer::ptr httpServer, HttpStaticFile::ptr httpStaticFile)
{
    auto addr = WebSrv::Address::lookupAny("0.0.0.0:8080");
    while (!httpServer->listen(addr))
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    Location::ptr location(new Location);
    location->path = "/";
    location->root = "/home/ubuntu/work/TinyWebServer/examples/httpstaticfile/html";
    location->isGlob = true;
    location->index = "html";
    httpStaticFile->addLocation(location);
    httpServer->start();
}

int main()
{
    g_logger->setLevel(WebSrv::LogLevel::Info);
    WebSrv::IOManager ioscheduler(6);
    WebSrv::IOManager scheduler(12);


    WebSrv::http::HttpServer::ptr httpServer(new WebSrv::http::HttpServer(&scheduler, &ioscheduler, &scheduler, true));
    HttpStaticFile::ptr httpStaticFile(new HttpStaticFile(httpServer));

    httpServer->setName("http server/1.0.0");
    scheduler.start();
    ioscheduler.start();
    scheduler.schedule(std::bind(run, httpServer, httpStaticFile));
}
