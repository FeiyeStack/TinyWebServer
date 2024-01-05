#pragma once
#include "streams/socketstream.h"
#include "http.h"
#include <mutex>
#include <atomic>
#include "util.h"

namespace WebSrv::http
{
    struct HttpResult
    {
        using ptr = std::shared_ptr<HttpResult>;
        enum class Error
        {
            /// @brief 正常
            OK = 0,
            /// @brief 非法URL
            INVALID_URL = 1,
            /// @brief 无法解析HOST
            INVALID_HOST = 2,
            /// @brief 连接失败
            CONNECT_FAIL = 3,
            /// @brief 连接被对端关闭
            SEND_CLOSE_BY_PEER = 4,
            /// @brief 发送请求产生Socket错误
            SEND_SOCKET_ERROR = 5,
            /// @brief 超时
            TIMEOUT = 6,
            /// @brief 创建Socket失败
            CREATE_SOCKET_ERROR = 7,
            /// @brief 从连接池中取连接失败
            POOL_GET_CONNECTION = 8,
            /// @brief 无效的连接
            POOL_INVALID_CONNECTION = 9,
        };

        HttpResult(int res, HttpResponse::ptr rep, const std::string &err)
            : error(res), response(rep), errorStr(err) {}

        int error;

        HttpResponse::ptr response;
        /// @brief  错误描述
        std::string errorStr;

        std::string toString() const;
    };

    class HttpConnection : public SocketStream
    {
        friend class HttpConnectionPool;

    public:
        using ptr = std::shared_ptr<HttpConnection>;
        /**
         * @brief 发送Http的Get请求
         *
         * @param url 请求url,需要使用绝对地址
         * @param timeout
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        static HttpResult::ptr doGet(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        static HttpResult::ptr doGet(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        /**
         * @brief 发送Http的Post请求
         *
         * @param url 请求url,需要使用绝对地址
         * @param timeout
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        static HttpResult::ptr doPost(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        static HttpResult::ptr doPost(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        /**
         * @brief 发送HTTP请求
         *
         * @param method
         * @param url 请求url,需要使用绝对地址
         * @param timeout_ms
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        static HttpResult::ptr doRequest(HttpMethod method, const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        static HttpResult::ptr doRequest(HttpMethod method, const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        
       static  HttpResult::ptr doRequest(HttpRequest::ptr req, const URL &url, uint64_t timeout_ms);
        
        /**
         * @brief 构造函数
         * @param sock Socket类
         * @param[ owner 是否掌握所有权
         */
        HttpConnection(Socket::ptr sock, bool owner = true);

        ~HttpConnection();

        /**
         * @brief 接收HTTP响应
         *
         * @return HttpResponse::ptr
         */
        HttpResponse::ptr recvResponse();

        /**
         * @brief 发送HTTP请求
         *
         * @param response
         * @return int
         */
        int sendRequest(HttpRequest::ptr response);

    private:
        uint64_t _createTime = 0;
        uint64_t _request = 0;
    };

    class HttpConnectionPool
    {
    public:
        using ptr = std::shared_ptr<HttpConnectionPool>;

        static HttpConnectionPool::ptr Create(const std::string &url, const std::string &vhost, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request);

        HttpConnectionPool(const std::string &host, const std::string &vhost, uint32_t port, bool is_https, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request);

        HttpConnection::ptr getConnection();
        /**
         * @brief 发送Http的Get请求
         *
         * @param url 请求url,需要使用绝对地址（后续调整）
         * @param timeout
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        HttpResult::ptr doGet(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        HttpResult::ptr doGet(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        /**
         * @brief 发送Http的Post请求
         *
         * @param url 请求url,需要使用绝对地址
         * @param timeout
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        HttpResult::ptr doPost(const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        HttpResult::ptr doPost(const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        /**
         * @brief 发送HTTP请求
         *
         * @param method
         * @param url 请求url,需要使用绝对地址
         * @param timeout_ms
         * @param headers
         * @param body
         * @return HttpResult::ptr
         */
        HttpResult::ptr doRequest(HttpMethod method, const std::string &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");
        HttpResult::ptr doRequest(HttpMethod method, const URL &url, uint64_t timeout, const std::map<std::string, std::string> &headers = {}, const std::string &body = "");

        HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout);

    private:
        static void releasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

    private:
        std::string _host;
        std::string _vhost;
        uint32_t _port;
        uint32_t _maxSize;
        uint32_t _maxAliveTime;
        uint32_t _maxRequest;
        bool _isHttps;

        std::mutex _mutex;
        std::list<HttpConnection *> _conns;
        std::atomic<int32_t> _total = {0};
    };
} // namespace WebSrv::http