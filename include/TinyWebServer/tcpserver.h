#pragma once

#include <memory>
#include "socket.h"
#include "noncopyable.h"
#include "iomanager.h"

namespace WebSrv
{

    // 可以用于实现静态文件服务
    struct TcpServerConf
    {
        using ptr = std::shared_ptr<TcpServerConf>;
    };

    class TcpServer : public std::enable_shared_from_this<TcpServer>, NonCopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;
        TcpServer(IOManager *worker = IOManager::getThis(),
                  IOManager *ioWorker = IOManager::getThis(),
                  IOManager *acceptWorker = IOManager::getThis());

        virtual ~TcpServer();
        /**
         * @brief (绑定地址)监听地址
         *
         * @param addr
         * @return true
         * @return false
         */
        virtual bool listen(Address::ptr addr);
        /**
         * @brief (绑定地址)监听多组地址
         *
         * @param addrs 预监听的地址
         * @param fails 失败的地址
         * @return true
         * @return false
         */
        virtual bool listen(const std::vector<Address::ptr> &addrs, std::vector<Address::ptr> &fails);
        /**
         * @brief 开始服务
         *
         */
        virtual void start();
        /**
         * @brief 停止服务
         *
         */
        virtual void stop();
        /**
         * @brief 是否停止
         *
         * @return true
         * @return false
         */
        bool isStop() const { return _stop; }

        /**
         * @brief 获取接收超时时间(ms)
         *
         * @return uint64_t
         */
        uint64_t getRecvTimeout() const { return _recvTimeout; }
        /**
         * @brief 设置接收超时时间(ms)
         *
         * @param recvTimeout
         */
        void setRecvTimeout(uint64_t recvTimeout) { _recvTimeout = recvTimeout; }

        /**
         * @brief 获取发送超时时间(ms)
         *
         * @return uint64_t
         */
        uint64_t getSendTimeout() const { return _sendTimeout; }
        /**
         * @brief 设置接收超时时间(ms)
         *
         * @param recvTimeout
         */
        void setSendTimeout(uint64_t sendTimeout) { _sendTimeout = sendTimeout; }
        /**
         * @brief 获取服务器名
         *
         * @return std::string
         */
        std::string getName() const { return _name; }
        /**
         * @brief 设置服务器名
         *
         * @param name
         */
        virtual void setName(const std::string &name) { _name = name; }
        /**
         * @brief 设置配置
         *
         * @param conf
         */
        void setConfig(TcpServerConf::ptr conf) { _conf = conf; }
        /**
         * @brief 获取配置
         *
         * @return TcpServerConf::ptr
         */
        TcpServerConf::ptr getConfig() { return _conf; }
        /**
         * @brief 打印配置信息
         *
         * @param prefix
         * @return std::string
         */
        virtual std::string toString(const std::string &prefix);

    protected:
        /**
         * @brief 处理新连接的socket
         *
         * @param client
         */
        virtual void handleClient(Socket::ptr client);
        /**
         * @brief 开始接受连接
         *
         * @param sock
         */
        virtual void startAccept(Socket::ptr sock);

    protected:
        /// @brief 监听socket
        std::vector<Socket::ptr> _socks;
        /// @brief 新连接工作调度
        IOManager *_worker;
        IOManager *_ioWorker;
        /// @brief 接收连接工作调度
        IOManager *_acceptWorker;
        /// @brief 接收超时时间(ms)
        uint64_t _recvTimeout;
        /// @brief 发送超时(ms)
        uint64_t _sendTimeout;
        /// @brief 是否停止服务
        bool _stop;
        /// @brief 服务器名称
        std::string _name;
        /// @brief 服务器类型
        std::string _type = "tcp";
        /// @brief 配置
        TcpServerConf::ptr _conf;
    };
} // namespace WebServer
