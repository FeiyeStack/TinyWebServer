#pragma once
#include <memory>
#include <unordered_set>
#include "http.h"
namespace WebSrv::http
{
    /**
     * @brief http响应报文解析
     * 
     */
    class HttpRequestParser
    {
    public:
        using ptr = std::shared_ptr<HttpRequestParser>;
        HttpRequestParser(uint8_t maxVersion=0x11);
        /**
         * @brief 解析协议(不解析请求体)
         * 
         * @param data 协议文本内存(会添加'\0'破坏原有的结构)
         * @param len 协议文本内存长度
         * @return size_t 成功返回解析长度，不是完整报文返回0，其他错误返回-1，错误码对应状态码
         */
        size_t execute(char* data,size_t len);
        /**
         * @brief 获取http响应报文储存结构
         * 
         * @return HttpRequest::ptr 
         */
        HttpRequest::ptr getData() const{return _data;};
        /**
         * @brief 获取消息体长度
         * 
         * @return uint64_t 
         */
        uint64_t getContentLength();
        /**
         * @brief 设置支持的http方法（默认支持所有）
         * 
         * @param methods 
         */
        void setMethods(std::shared_ptr<std::unordered_set<HttpMethod>> methods) { _methods = methods; }
        /**
         * @brief 获取当前http支持的方法
         * 
         * @return std::shared_ptr<std::unordered_set<HttpMethod>>
         */
        std::shared_ptr<std::unordered_set<HttpMethod>> getMethods() { return _methods; }
        /**
         * @brief 错误码对应http状态码
         * 
         * @return int 
         */
        int getError(){return _error;}
        /**
         * @brief 返回http 请求协议解析的缓存大小
         *
         * @return uint64_t
         */
        static uint64_t getHttpRequestBufferSize();
        /**
         * @brief 返回http 请求协议解析的最大消息体大小
         *
         * @return uint64_t
         */
        static uint64_t getHttpRequestMaxBodySize();

    private:
        int parseHeaders(std::vector<char *> &headers);
    private:
        /// @brief 错误码
        int _error=0;
        /// @brief http响应报文储存结构
        HttpRequest::ptr _data;
        /// @brief 支持最大版本
        uint8_t _maxVersion;
        /// @brief 支持方法
        std::shared_ptr<std::unordered_set<HttpMethod>> _methods;
    };

    class HttpResponseParser
    {
    public:
        using ptr = std::shared_ptr<HttpResponseParser>;
        HttpResponseParser();
        /**
         * @brief 解析协议(不解析请求体)
         * 
         * @param data 协议文本内存(会添加'\0'破坏原有的结构)
         * @param len 协议文本内存长度
         * @return int 成功返回解析长度，不是完整报文返回0，错误返回-1
         */
        size_t execute(char* data,size_t len);
        /**
         * @brief 获取http响应报文储存结构
         * 
         * @return HttpResponse::ptr 
         */
        HttpResponse::ptr getData() const{return _data;};
        /**
         * @brief 获取消息体长度
         * 
         * @return uint64_t 
         */
        uint64_t getContentLength();
        /**
         * @brief 返回http 响应协议解析的缓存大小
         *
         * @return uint64_t
         */
        static uint64_t getHttpResponseBufferSize();
        /**
         * @brief 返回http 响应协议解析的最大消息体大小
         *
         * @return uint64_t
         */
        static uint64_t getHttpResponseMaxBodySize();

    private:
        bool parseHeaders(std::vector<char*> headers); 
    private:
        /// @brief http响应报文储存结构
        HttpResponse::ptr _data;
    };
} // namespace WebSrv::http
