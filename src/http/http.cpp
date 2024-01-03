#include "http/http.h"
#include <cstring>
#include <string.h>
#include "util.h"
namespace WebSrv::http
{
    HttpMethod HttpMethodFromString(const std::string &method)
    {
#define XX(num, name, string)                 \
    if (strcmp(#string, method.c_str()) == 0) \
    {                                         \
        return HttpMethod::HTTP_##name;       \
    }                                         
    HTTP_METHOD_MAP(XX);
#undef XX
        return HttpMethod::HTTP_INVALID_METHOD;
    }

    HttpMethod HttpMethodFromChars(const char *method)
    {
#define XX(num, name, string)                           \
    if (strncmp(#string, method, sizeof(#string)) == 0) \
    {                                                   \
        return HttpMethod::HTTP_##name;                 \
    }                                                   
    HTTP_METHOD_MAP(XX);
#undef XX
        return HttpMethod::HTTP_INVALID_METHOD;
    }

    static const char *s_methodStr[] = {
#define XX(num, name, string) #string,
        HTTP_METHOD_MAP(XX)
#undef XX
    };

    const char *HttpMethodToString(const HttpMethod &method)
    {
        int index = (int)method;
        if (index >= (int)(sizeof(s_methodStr) / sizeof(s_methodStr[0])))
        {
            return "Unkown Method";
        }
        return s_methodStr[index];
    }

    const char *HttpStateToString(const HttpStatus &status)
    {
        switch (status)
        {
#define XX(num, name, string)            \
    case HttpStatus::HTTP_STATUS_##name: \
        return #string;
            HTTP_STATUS_MAP(XX);
#undef XX
        default:
            break;
        }
        return "Unkown Status";
    }

    bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const
    {
        return strcasecmp(lhs.c_str(), rhs.c_str())<0;
    }

    HttpRequest::HttpRequest(uint8_t version)
        : _version(version)
    {
    }

    std::string HttpRequest::getHeader(const std::string &key, const std::string &def) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return it->second;
        }
        return def;
    }

    std::string HttpRequest::getParam(const std::string &key, const std::string &def) const
    {
        auto it = _params.find(key);
        if (it != _params.end())
        {
            return it->second;
        }
        return def;
    }

    std::string HttpRequest::getCookie(const std::string &key, const std::string &def) const
    {
        auto it = _cookies.find(key);
        if (it != _cookies.end())
        {
            return it->second;
        }
        return def;
    }

    bool HttpRequest::hasHeader(const std::string &key, std::string *result) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            if (result)
            {
                *result = it->second;
            }
            return true;
        }
        return false;
    }

    bool HttpRequest::hasParam(const std::string &key, std::string *result) const
    {
        auto it = _params.find(key);
        if (it != _params.end())
        {
            if (result)
            {
                *result = it->second;
            }
            return true;
        }

        return false;
    }

    bool HttpRequest::hasCookie(const std::string &key, std::string *result) const
    {
        auto it = _cookies.find(key);
        if (it != _cookies.end())
        {
            if (result)
            {
                *result = it->second;
            }
            return true;
        }

        return false;
    }

    void HttpRequest::setHeader(const std::string &key, const std::string &value)
    {
        _headers[key] = value;
    }

    void HttpRequest::setParam(const std::string &key, const std::string &value)
    {
        _params[key] = value;
    }

    void HttpRequest::setCookie(const std::string &key, const std::string &value)
    {
        _cookies[key] = value;
    }

    void HttpRequest::delHeader(const std::string &key)
    {
        _headers.erase(key);
    }

    void HttpRequest::delParam(const std::string &key)
    {
        _headers.erase(key);
    }

    void HttpRequest::delCookie(const std::string &key)
    {
        _headers.erase(key);
    }

    std::ostream &HttpRequest::dump(std::ostream &os) const
    {
        addLine(os);
        addHeaders(os);
        addBody(os);
        return os;
    }

    std::string HttpRequest::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    void HttpRequest::addLine(std::ostream &os) const
    {
        os << HttpMethodToString(_method) << " " << _path;
        if (!_params.empty() && _method == HttpMethod::HTTP_GET)
        {
            auto it = _params.begin();
            os << "?";
            for (;;)
            {
                os << it->first << "=" << it->second;
                it++;
                if (it == _params.end())
                {
                    break;
                }
                os << "&";
            }
        }
        os << " HTTP/"
           << ((uint32_t)(_version >> 4))
           << "."
           << ((uint32_t)(_version & 0x0F))
           << "\r\n";
    }
    void HttpRequest::addHeaders(std::ostream &os) const
    {
        for (auto &&[key, value] : _headers)
        {
            os << key << ": " << value << "\r\n";
        }
        if (!_cookies.empty())
        {
            auto it = _cookies.begin();
            os << "Cookie: ";
            for (;;)
            {
                os << it->first << "=" << it->second;
                it++;
                if (it == _cookies.end())
                {
                    os<<"\r\n";
                    break;
                }
                os << "; ";
            }
        }
    }
    void HttpRequest::addBody(std::ostream &os) const
    {
        auto it = _headers.find("content-type");
        if (it == _headers.end())
        {
            os << "\r\n";
            return;
        }

        auto type = it->second;
        std::stringstream ss;
        if (strstr(type.c_str(), "application/x-www-form-urlencoded") != nullptr)
        {
            if (!_params.empty())
            {
                auto it = _params.begin();
                for (;;)
                {
                    ss << it->first << "=" << it->second;
                    it++;
                    if (it == _params.end())
                    {
                        break;
                    }
                    ss << "&";
                }
                os << "content-length: " << ss.str().size() << "\r\n\r\n"
               << ss.str();
            }
            os << "\r\n";
            return;
        }

        // 其他直接使用body
        // 其中json格式不能用简单的map表达所以也忽略(后续考虑要不要储存json结构)
        if (!_body.empty())
        {
            os << "content-length: " << _body.size() << "\r\n\r\n"
               << _body;
        }
        else
        {
            os << "\r\n";
        }
    }
    HttpResponse::HttpResponse(uint8_t version)
        :_status(HttpStatus::HTTP_STATUS_OK),_version(version)
    {
    }
    
    std::string HttpResponse::getHeader(const std::string &key, const std::string &def) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return it->second;
        }
        return def;
    }
    bool HttpResponse::hasHeader(const std::string &key, std::string *result) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            if (result)
            {
                *result = it->second;
            }
            return true;
        }
        return false;
    }
    void HttpResponse::setHeader(const std::string &key, const std::string &value)
    {
        _headers[key] = value;
    }
    void HttpResponse::delHeader(const std::string &key)
    {
        _headers.erase(key);
    }

    void HttpResponse::setCookie(const std::string &key, const std::string &value, const CookieOptions &options)
    {
        _cookies[key]={value,options};
    }

    std::pair<std::string, HttpResponse::CookieOptions> HttpResponse::getCookie(const std::string &key)
    {
        auto it=_cookies.find(key);
        if(it!=_cookies.end()){
            return it->second;
        }
        return std::pair<std::string, HttpResponse::CookieOptions>();
    }

    std::ostream &HttpResponse::dump(std::ostream &os) const
    {
        os << " HTTP/"
           << ((uint32_t)(_version >> 4))
           << "."
           << ((uint32_t)(_version & 0x0F))
           << " "
           << (uint32_t)_status
           << " "
           << (_reason.empty() ? HttpStateToString(_status) : _reason)
           << "\r\n";

        for (auto &&[key, value] : _headers)
        {
            os << key << ": " << value << "\r\n";
        }
        addCookie(os);
        if (!_body.empty())
        {
            os << "content-length: " << _body.size() << "\r\n\r\n"
               << _body;
        }
        else
        {
            os << "\r\n";
        }
        return os;
    }

    std::string HttpResponse::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    void HttpResponse::addCookie(std::ostream &os) const
    {
        for (auto &&[key, value] : _cookies)
        {
            auto &&[val, options] = value;
            os << "Set-Cookie: " << key << "=" << val;
            if (!options.domain.empty())
            {
                os << "; domain=" << options.domain;
            }
            if (options.expiration > 0)
            {
                os << "; expires=" << time2ToStr(options.expiration, "%a, %d %b %Y %H:%M:%S") << " GMT";
            }
            if (options.httpOnly)
            {
                os << "; HttpOnly";
            }
            if (options.maxAge.has_value())
            {
                os << "; Max-Age=" << options.maxAge.value();
            }
            if (options.partitioned)
            {
                os << "; partitioned";
            }
            if (!options.path.empty())
            {
                os << "; path=" << options.path;
            }
            if (options.sameSite.has_value())
            {
                switch (options.sameSite.value())
                {
                case CookieOptions::SameSite::Strict:
                    os << "; SameSite=strict";
                    break;
                case CookieOptions::SameSite::Lax:
                    os << "; SameSite=lax";
                    break;
                case CookieOptions::SameSite::None:
                    os << "; SameSite=none";
                    break;
                default:
                    break;
                }
            }
            if (options.secure)
            {
                os << "; secure";
            }
            os<< "\r\n";
        }
    }

    std::ostream &operator<<(std::ostream &os, const HttpRequest &other)
    {
        return other.dump(os);
    }

    std::ostream &operator<<(std::ostream &os, const HttpResponse &other)
    {
        return other.dump(os);
    }
} // namespace WebSrv::http
