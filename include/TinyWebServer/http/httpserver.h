#pragma once
#include "tcpserver.h"
#include "servlet.h"
namespace WebSrv::http
{
    class HttpServer : public TcpServer
    {
    public:
        using ptr = std::shared_ptr<HttpServer>;
        HttpServer(IOManager *worker= IOManager::getThis(),
                   IOManager *ioWorker= IOManager::getThis(),
                   IOManager *acceptWorker= IOManager::getThis(),
                   bool keepalive = false);

        void setName(const std::string &name) override;

        void setServletDispatch(ServletDispatch::ptr dispatch) { _dispatch = dispatch; }

        ServletDispatch::ptr getServletDispatch() { return _dispatch; }
    protected:
        virtual void handleClient(Socket::ptr client) override;

    private:
        // 是否支持长连接
        bool _keepalive;
        /// @brief Servlet 分发器
        ServletDispatch::ptr _dispatch;
    };
} // namespace WebSrv::http