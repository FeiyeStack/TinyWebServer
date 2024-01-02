#pragma once
#include "noncopyable.h"
#include "address.h"

namespace WebSrv
{
    class Socket : public std::enable_shared_from_this<Socket>, NonCopyable
    {
    public:
        using ptr = std::shared_ptr<Socket>;
        /**
         * @brief socket类型
         *
         */
        enum Type
        {
            TCP = SOCK_STREAM,
            UDP = SOCK_DGRAM,
        };
        /**
         * @brief Socket协议簇
         *
         */
        enum Family
        {
            Ipv4 = AF_INET,
            Ipv6 = AF_INET6,
            UNIX = AF_UNIX,
        };

        static Socket::ptr CreateTCP(Address::ptr address);

        static Socket::ptr CreateUDP(Address::ptr address);

        /**
         * @brief 创建tcp ipv4 TCP socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateTCPSocket();
        /**
         * @brief 创建ucp ipv4 UCP socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateUCPSocket();
        /**
         * @brief 创建ucp ipv6 TCP socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateTCPSocket6();
        /**
         * @brief 创建ucp ipv6 UCP socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateUCPSocket6();
        /**
         * @brief Create a Unix TCP Socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateUnixTCPSocket();
        /**
         * @brief Create a Unix UCP Socket
         *
         * @return Socket::ptr
         */
        static Socket::ptr CreateUnixUCPSocket();

        /**
         * @brief Construct a new Socket object
         *
         * @param family 协议簇
         * @param type 类型
         * @param protocol 协议
         */
        Socket(int family, int type, int protocol = 0);

        virtual ~Socket();

        /**
         * @brief getsockopt
         *
         * @param level
         * @param option
         * @param result
         * @param len
         * @return true
         * @return false
         */
        bool getOption(int level, int option, void *result, socklen_t *len);

        /**
         * @brief getsockopt 模板
         *
         * @param level
         * @param option
         * @param result
         * @param len
         * @return true
         * @return false
         */
        template <typename T>
        bool getOption(int level, int option, T &result)
        {
            socklen_t length = sizeof(T);
            return getOption(level, option, &result, &length);
        }

        /**
         * @brief setsockopt
         *
         * @param level
         * @param option
         * @param value
         * @param len
         * @return true
         * @return false
         */
        bool setOption(int level, int option, const void *value, socklen_t len); // windows

        /**
         * @brief getsockopt 模板
         *
         * @param level
         * @param option
         * @param result
         * @param len
         * @return true
         * @return false
         */
        template <typename T>
        bool setOption(int level, int option, const T &value)
        {
            return setOption(level, option, &value, sizeof(T));
        }

        int64_t getSendTimeout(); 

        /**
         * @brief 设置超时时间
         *
         * @param timeout
         */
        void setSendTimeout(int64_t timeout); 

        int64_t getRecvTimeout(); 

        void setRecvTimeout(int64_t timeout); 

        int getError();

        bool bind(Address::ptr address);

        virtual bool listen(int backlog = SOMAXCONN);

        virtual bool close();

        virtual Socket::ptr accept();

        virtual bool connect(const Address::ptr addr, int64_t timeoutMs = -1);

        virtual bool reconnect(int64_t timeoutMs = -1);


        virtual int send(const void *buffer, size_t length, int flags = 0);
        virtual int send(iovec *buffers, size_t length, int flags = 0);

        virtual int sendTo(const void *buffer, size_t length, const Address::ptr to, int flags = 0);
        virtual int sendTo(iovec *buffers, size_t length, const Address::ptr to, int flags = 0);

        virtual int recv(void *buffer, size_t length,  int flags = 0);
        virtual int recv(iovec *buffers, size_t length, int flags = 0);
        virtual int recvFrom(void *buffer, size_t length,  Address::ptr from, int flags = 0);
        virtual int recvFrom(iovec *buffers, size_t length,  Address::ptr from, int flags = 0);
        /**
         * @brief 获取远端地址
         *
         * @return Address::ptr
         */

        Address::ptr getRemoteAddress() const{return _remoteAddress;}

        /**
         * @brief 获取本地地址
         *
         * @return Address::ptr
         */
        Address::ptr getLocalAddress() const{return _localAddress;}

        /**
         * @brief 获取协议簇
         */
        int getFamily() const{return _family;}

        /**
         * @brief 获取类型
         */
        int getType() const{return _type;}

        /**
         * @brief 获取协议
         */
        int getProtocol() const{return _protocol;}


        /**
         * @brief socket是否有效
         *
         * @return true
         * @return false
         */
        bool isValid() const{return _socket != -1;}

        /**
         * @brief 是否是连接状态
         * 
         * @return true 
         * @return false 
         */
        bool isConnected() const{return _isConnected;}

        virtual std::ostream &dump(std::ostream &os) const;

        virtual std::string toString() const;
        // socket
        operator int() const { return _socket; }

    protected:

        void newSocket();
        void initLocalAddress();
        void initSocket();
        void initRemoteAddress();
        bool init(int socket);
    protected:
        int _socket;
        // 协议簇
        int _family;
        // 类型
        int _type;
        // 协议
        int _protocol;
        // 是否连接
        bool _isConnected;
        // 本地地址
        Address::ptr _localAddress;
        // 远端地址
        Address::ptr _remoteAddress;
    };
} // namespace WebSrv