#include "http/httpparser.h"
#include <string>
#include <vector>
#include <regex>
#include "util.h"
#include "configurator.h"
namespace WebSrv::http
{
    static ConfigVar<uint64_t>::ptr g_httpRequestBufferSize =
        Configurator::lookup("http.request.buffer_size", (uint64_t)(4 * 1024), "http request buffer size");

    static ConfigVar<uint64_t>::ptr g_httpRequestMaxBodySize =
        Configurator::lookup("http.request.max_body_size", (uint64_t)(64 * 1024 * 1024), "http request max body size");

    static ConfigVar<uint64_t>::ptr g_httpResponseBufferSize =
        Configurator::lookup("http.response.buffer_size", (uint64_t)(4 * 1024), "http response buffer size");

    static ConfigVar<uint64_t>::ptr g_httpResponseMaxBodySize =
        Configurator::lookup("http.response.max_body_size", (uint64_t)(64 * 1024 * 1024), "http response max body size");

    static uint64_t s_httpRequestBufferSize=0;
    static uint64_t s_httpRequestMaxBodySize=0;
    static uint64_t s_httpResponseBufferSize=0;
    static uint64_t s_httpResponseMaxBodySize=0;

    struct _HttpParserSizeInit
    {
        _HttpParserSizeInit(){
            s_httpRequestBufferSize=g_httpRequestBufferSize->getValue();
            s_httpRequestMaxBodySize=g_httpRequestMaxBodySize->getValue();
            s_httpResponseBufferSize=g_httpResponseBufferSize->getValue();
            s_httpResponseMaxBodySize=g_httpResponseMaxBodySize->getValue();

            g_httpRequestBufferSize->addChangeValueListener(
                [](const uint64_t &oldValue, const uint64_t &newValue)
                {
                    s_httpRequestBufferSize = newValue;
                });
            g_httpRequestMaxBodySize->addChangeValueListener(
                [](const uint64_t &oldValue, const uint64_t &newValue)
                {
                    s_httpRequestMaxBodySize = newValue;
                });
            g_httpResponseBufferSize->addChangeValueListener(
                [](const uint64_t &oldValue, const uint64_t &newValue)
                {
                    s_httpResponseBufferSize = newValue;
                });
            g_httpResponseMaxBodySize->addChangeValueListener(
                [](const uint64_t &oldValue, const uint64_t &newValue)
                {
                    s_httpResponseMaxBodySize = newValue;
                });
        }
    };
    
    static _HttpParserSizeInit _init;

    HttpRequestParser::HttpRequestParser(uint8_t maxVersion)
        : _data(new HttpRequest), _maxVersion(maxVersion)
    {
    }

    static bool isValidHTTPVersion(const std::string &version)
    {
        std::regex versionRegex("^HTTP/(0*[1-9]\\d*)(\\.([01]))?$");
        return std::regex_match(version, versionRegex);
    }

    size_t HttpRequestParser::execute(char *data, size_t len)
    {
        _error=0;
        // 判断是不是数据接收不完整
        if (len <= 4)
        {
            return 0;
        }

        char* c=strstr(data,"\r\n\r\n");
        if (!c)
        {
            return 0;
        }

        for(int i=0;i<4;i++,c++){
            *c='\0';
        }

        // 解析提取行
        std::vector<char *> lines;
        split2Str(data, lines, "\r\n");
        char *oldData = data;
        data = c;
        if (lines.size() < 1)
        {
            _error=(int)HttpStatus::HTTP_STATUS_BAD_REQUEST;
            return -1;
        }

        // 判断请求行
        std::vector<char *> chunk;
        split2Str(lines[0], chunk, " ");
        if (chunk.size() < 3)
        {
            _error=(int)HttpStatus::HTTP_STATUS_BAD_REQUEST;
            return -1;
        }

        auto method = HttpMethodFromString(chunk[0]);

        if (method == HttpMethod::HTTP_INVALID_METHOD || (_methods && _methods->count(method) < 0))
        {
            _error=(int)HttpStatus::HTTP_STATUS_METHOD_NOT_ALLOWED;
            return -1;
        }

        _data->setHttpMethod(method);

        if (strlen(chunk[2]) != 8 || !isValidHTTPVersion(chunk[2]))
        {
            _error=(int)HttpStatus::HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;
            return -1;
        }

        uint8_t version = chunk[2][5] - '0';
        version <<= 4;
        version += chunk[2][7] - '0';

        if (version > _maxVersion)
        {
            _error=(int)HttpStatus::HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;
            return -1;
        }

        _data->setVersion(version);

        // 解析URL
        URL url;
        if (!parse2URL(chunk[1], url))
        {
            _error = (int)HttpStatus::HTTP_STATUS_BAD_REQUEST;
            return -1;
        }

        _data->setPath(url.path);
        if (!url.query.empty())
        {
            _data->setPath(url.path);
            std::vector<std::string> param;
            // 定义查询参数解析的正则表达式
            static std::regex queryRegex("([^&=?]+)=([^&=?]+)");

            auto query_begin = std::sregex_iterator(url.query.begin(), url.query.end(), queryRegex);
            auto query_end = std::sregex_iterator();
            for (std::sregex_iterator it = query_begin; it != query_end; ++it)
            {
                std::smatch match = *it;
                _data->setParam(match[1].str(), match[2].str());
            }
        }

        auto err = parseHeaders(lines);
        if (err != 0)
        {
            return err;
        }
        return data-oldData;
    }

    uint64_t HttpRequestParser::getContentLength()
    {
        return _data->getHeaderAs("content-length", 0);
    }

    uint64_t HttpRequestParser::getHttpRequestBufferSize()
    {
        return s_httpRequestBufferSize;
    }

    uint64_t HttpRequestParser::getHttpRequestMaxBodySize()
    {
        return s_httpRequestMaxBodySize;
    }

    int HttpRequestParser::parseHeaders(std::vector<char *> &headers)
    {
        size_t len;
        for (int i = 1; i < headers.size(); i++)
        {
            char *v = strchr(headers[i], ':');
            if (v == nullptr)
            {
                _error = (int)HttpStatus::HTTP_STATUS_BAD_REQUEST;
                return -1;
            }
            len = strlen(v);
            *v = '\0';
            v++;
            for (size_t j = 1; j < len && *v == ' '; j++, v++)
                ;
            _data->setHeader(headers[i], v);
        }
        return 0;
    }

    HttpResponseParser::HttpResponseParser()
        : _data(new HttpResponse)
    {
    }

    size_t HttpResponseParser::execute(char *data, size_t len)
    {
        // 判断是不是数据接收不完整
        if (len <= 4)
        {
            return 0;
        }
        
        char* c=strstr(data,"\r\n\r\n");
        if (!c)
        {
            return 0;
        }
        
        for(int i=0;i<4;i++,c++){
            *c='\0';
        }
        // 解析提取行
        std::vector<char *> lines;
        split2Str(data, lines, "\r\n");
        char *oldData = data;
        data = c;
        if (lines.size() < 1)
        {
            return -1;
        }

        // 判断响应行
        std::vector<char *> chunk;
        split2Str(lines[0], chunk, " ");
        if (chunk.size() < 2)
        {
            return -1;
        }

        if (strlen(chunk[0]) != 8 || !isValidHTTPVersion(chunk[0]))
        {
            return -1;
        }

        uint8_t version = chunk[0][5] - '0';
        version <<= 4;
        version += chunk[0][7] - '0';
        _data->setVersion(version);

        if (strlen(chunk[1]) != 3)
        {
            return -1;
        }

        try
        {
            int status = lexicalCast<int>(chunk[1]);
            _data->setStatus((HttpStatus)status);
        }
        catch (...)
        {
            return -1;
        }

        if (chunk.size() == 3)
        {
            _data->setReason(chunk[2]);
        }

        if (!parseHeaders(lines))
        {
            return -1;
        }
        return data-oldData;
    }

    uint64_t WebSrv::http::HttpResponseParser::getContentLength()
    {
        return _data->getHeaderAs<uint64_t>("content-length", 0);
    }

    uint64_t HttpResponseParser::getHttpResponseBufferSize()
    {
        return s_httpResponseBufferSize;
    }
    uint64_t HttpResponseParser::getHttpResponseMaxBodySize()
    {
        return s_httpResponseMaxBodySize;
    }
    bool HttpResponseParser::parseHeaders(std::vector<char *> headers)
    {
        size_t len;
        for (int i = 1; i < headers.size(); i++)
        {
            char *v = strchr(headers[i], ':');
            if (v == nullptr)
            {
                return false;
            }
            len = strlen(v);
            *v = '\0';
            v++;
            for (size_t j = 1; j < len && *v == ' '; j++, v++)
                ;
            _data->setHeader(headers[i], v);
        }
        return true;
    }
} // namespace WebSrv::http
