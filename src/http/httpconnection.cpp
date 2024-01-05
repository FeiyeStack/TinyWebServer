#include "http/httpconnection.h"
#include <regex>
#include "http/httpparser.h"
#include "log.h"

namespace WebSrv::http
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");

    std::string http::HttpResult::toString() const
    {
        std::stringstream ss;
        ss << "[HttpResult result=" << error
           << " error=" << errorStr
           << " response=" << (response ? response->toString() : "nullptr")
           << "]";
        return ss.str();
    }
    HttpResult::ptr HttpConnection::doGet(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        URL res;
        if (!parseURL(url, res))
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
        }
        return doGet(res, timeout, headers, body);
    }

    HttpResult::ptr HttpConnection::doGet(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        return doRequest(HttpMethod::HTTP_GET, url, timeout, headers, body);
    }

    HttpResult::ptr HttpConnection::doPost(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        URL res;
        if (!parseURL(url, res))
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
        }
        return doPost(res, timeout, headers, body);
    }
    HttpResult::ptr HttpConnection::doPost(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        return doRequest(HttpMethod::HTTP_POST, url, timeout, headers, body);
    }
    HttpResult::ptr HttpConnection::doRequest(HttpMethod method, const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        URL res;
        if (!parseURL(url, res))
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
        }
        return doRequest(method, res, timeout, headers, body);
    }

    HttpResult::ptr HttpConnection::doRequest(HttpMethod method, const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        HttpRequest::ptr req = std::make_shared<HttpRequest>();
        req->setPath(url.path);
        if (!url.query.empty())
        {
            req->setPath(url.path);
            std::vector<std::string> param;
            // 定义查询参数解析的正则表达式
            static std::regex queryRegex("([^&=?]+)=([^&=?]+)");

            auto query_begin = std::sregex_iterator(url.query.begin(), url.query.end(), queryRegex);
            auto query_end = std::sregex_iterator();
            for (std::sregex_iterator it = query_begin; it != query_end; ++it)
            {
                std::smatch match = *it;
                req->setParam(match[1].str(), match[2].str());
            }
        }
        req->setFragment(url.fragment);
        req->setHttpMethod(method);
        bool has_host = false;
        for (auto &i : headers)
        {
            if (!has_host && strcasecmp(i.first.c_str(), "host") == 0)
            {
                has_host = !i.second.empty();
            }

            req->setHeader(i.first, i.second);
        }
        if (!has_host)
        {
            req->setHeader("Host", url.host);
        }
        req->setBody(body);
        return doRequest(req, url, timeout);
    }

    HttpResult::ptr HttpConnection::doRequest(HttpRequest::ptr req, const URL &url, uint64_t timeout_ms)
    {
        if (url.protocol != "http")
        {
            // 暂时只支持http
            return nullptr;
        }

        Address::ptr addr = Address::lookupAny(url.host);

        if (!addr)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + url.host);
        }
        Socket::ptr sock = Socket::CreateTCP(addr);

        if (!sock)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr, "create socket fail: " + addr->toString() + " errno=" + std::to_string(errno) + " errstr=" + std::string(strerror(errno)));
        }

        if (!sock->connect(addr))
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + addr->toString());
        }

        sock->setRecvTimeout(timeout_ms);

        HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);

        int rt = conn->sendRequest(req);

        if (rt == 0)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer: " + addr->toString());
        }

        if (rt < 0)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno=" + std::to_string(errno) + " errstr=" + std::string(strerror(errno)));
        }
        auto rsp = conn->recvResponse();

        if (!rsp)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timeout: " + addr->toString() + " timeout_ms:" + std::to_string(timeout_ms));
        }
        return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
    }

    HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
        : SocketStream(sock, owner)
    {
    }

    HttpConnection::~HttpConnection()
    {
    }

    HttpResponse::ptr HttpConnection::recvResponse()
    {
        HttpResponseParser::ptr parser(new HttpResponseParser);
        uint64_t buffSize = HttpRequestParser::getHttpRequestBufferSize();
        std::shared_ptr<char> buffer(new char[buffSize], [](char *p)
                                     { delete[] p; });
        char *data = buffer.get();
        int offset = 0;
        // 获取请求头
        do
        {
            int len = read(data + offset, buffSize - offset);
            if (len <= 0)
            {
                close();
                return nullptr;
            }
            len += offset;
            size_t nParse = parser->execute(data, len);
            if (nParse < 0)
            {
                // 解析错误
                close();
                return nullptr;
            }
            // nParse==0 报文不完整(不处理到缓存满或者超时退出)
            offset = len - nParse;

            // 解析完成
            if (nParse > 0)
            {
                break;
            }

            if (offset == (int)buffSize)
            {
                close();
                return nullptr;
            }

        } while (true);
        // 获取请求消息体
        u_int64_t length = parser->getContentLength();
        if (length > 0)
        {
            if (length > HttpRequestParser::getHttpRequestMaxBodySize())
            {
                close();
                return nullptr;
            }
            std::string body;
            body.resize(length);
            int len = 0;
            if (length >= (u_int64_t)offset)
            {
                memcpy(&body[0], data, offset);
                len = offset;
            }
            else
            {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            // 如果响应体没获取完整，继续读消息体
            if (length > 0)
            {
                if (readFixSize(&body[len], length) <= 0)
                {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
        // 暂时不处理压缩
        return parser->getData();
    }
    int HttpConnection::sendRequest(HttpRequest::ptr response)
    {
        std::string data = response->toString();
        return writeFixSize(data.c_str(), data.size());
    }
    HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string &url, const std::string &vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request)
    {
        URL res;
        if (!parseURL(url, res))
        {
            SRV_LOG_ERROR(g_logger) << "invalid uri=" << url;
        }
        auto i = res.host.find(":");
        uint32_t port = 0;
        if (i != std::string::npos)
        {
            port = std::stoul(res.host.substr(i + 1));
        }
        return std::make_shared<HttpConnectionPool>(res.host, vhost, port, res.protocol == "https", max_size, max_alive_time, max_request);
    }

    HttpConnectionPool::HttpConnectionPool(const std::string &host, const std::string &vhost, uint32_t port, bool is_https, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request)
        : _host(host), _vhost(vhost), _port(port ? port : (is_https ? 443 : 80)), _maxSize(max_size), _maxAliveTime(max_alive_time), _maxRequest(max_request), _isHttps(is_https)
    {
    }
    HttpConnection::ptr HttpConnectionPool::getConnection()
    {
        // 暂时不支持http
        if (_port == 443)
        {
            return nullptr;
        }
        uint64_t nowMs = getCurrentMilliseconds();
        std::vector<HttpConnection *> invalid_conns;
        HttpConnection *ptr = nullptr;
        std::unique_lock lock(_mutex);
        while (!_conns.empty())
        {
            auto conn = *_conns.begin();
            _conns.pop_front();
            if (!conn->isConnected())
            {
                invalid_conns.push_back(conn);
                continue;
            }
            if ((conn->_createTime + _maxAliveTime) > nowMs)
            {
                invalid_conns.push_back(conn);
                continue;
            }
            ptr = conn;
            break;
        }
        lock.unlock();
        for (auto i : invalid_conns)
        {
            delete i;
        }
        _total -= invalid_conns.size();

        if (!ptr)
        {
            IPAddress::ptr addr = Address::lookupAnyIPAddress(_host);
            if (!addr)
            {
                SRV_LOG_ERROR(g_logger) << "get addr fail: " << _host;
                return nullptr;
            }
            addr->setPort(_port);
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock)
            {
                SRV_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
                return nullptr;
            }
            if (!sock->connect(addr))
            {
                SRV_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
                return nullptr;
            }

            ptr = new HttpConnection(sock);
            ++_total;
        }
        return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::releasePtr, std::placeholders::_1, this));
    }
    HttpResult::ptr HttpConnectionPool::doGet(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        return doRequest(HttpMethod::HTTP_GET, url, timeout, headers, body);
    }
    HttpResult::ptr HttpConnectionPool::doGet(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        std::stringstream ss;
        ss << url.path
           << (url.query.empty() ? "" : "?")
           << url.query
           << (url.fragment.empty() ? "" : "#")
           << url.fragment;
        return doGet(ss.str(), timeout, headers, body);
    }
    HttpResult::ptr HttpConnectionPool::doPost(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        return doRequest(HttpMethod::HTTP_POST, url, timeout, headers, body);
    }
    HttpResult::ptr HttpConnectionPool::doPost(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        std::stringstream ss;
        ss << url.path
           << (url.query.empty() ? "" : "?")
           << url.query
           << (url.fragment.empty() ? "" : "#")
           << url.fragment;
        return doPost(ss.str(), timeout, headers, body);
    }
    HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        HttpRequest::ptr req = std::make_shared<HttpRequest>();
        req->setPath(url);
        req->setHttpMethod(method);
        bool has_host = false;
        for (auto &i : headers)
        {
            if (!has_host && strcasecmp(i.first.c_str(), "host") == 0)
            {
                has_host = !i.second.empty();
            }

            req->setHeader(i.first, i.second);
        }
        if (!has_host)
        {
            if (_vhost.empty())
            {
                req->setHeader("Host", _host);
            }
            else
            {
                req->setHeader("Host", _vhost);
            }
        }
        req->setBody(body);
        return doRequest(req, timeout);
    }
    HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers, const std::string &body)
    {
        std::stringstream ss;
        ss << url.path
           << (url.query.empty() ? "" : "?")
           << url.query
           << (url.fragment.empty() ? "" : "#")
           << url.fragment;
        return doRequest(method, ss.str(), timeout, headers, body);
    }

    HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout)
    {
        auto conn = getConnection();
        if (!conn)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "pool host:" + _host + " port:" + std::to_string(_port));
        }
        auto sock = conn->getSocket();
        if (!sock)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION, nullptr, "pool host:" + _host + " port:" + std::to_string(_port));
        }
        sock->setRecvTimeout(timeout);
        int rt = conn->sendRequest(req);
        if (rt == 0)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
        }
        if (rt < 0)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno=" + std::to_string(errno) + " errstr=" + std::string(strerror(errno)));
        }
        auto rsp = conn->recvResponse();
        if (!rsp)
        {
            return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString() + " timeout_ms:" + std::to_string(timeout));
        }
        return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
    }

    void HttpConnectionPool::releasePtr(HttpConnection *ptr, HttpConnectionPool *pool)
    {
        ++ptr->_request;
        if (!ptr->isConnected() || ((ptr->_createTime + pool->_maxAliveTime) >= getCurrentMilliseconds()) || (ptr->_request >= pool->_maxRequest))
        {
            delete ptr;
            --pool->_total;
            return;
        }
        std::lock_guard lock(pool->_mutex);
        pool->_conns.push_back(ptr);
    }
} // namespace WebSrv::http
