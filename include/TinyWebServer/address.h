#pragma once
#include <memory>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
namespace WebSrv
{
    class IPAddress;
    /**
     * @brief 网络地址类型
     *
     */
    class Address
    {
    public:
        using ptr = std::shared_ptr<Address>;
        /**
         * @brief 使用sockaddr 创建
         *
         * @param addr
         * @param addrlen
         * @return Address::ptr
         */
        static Address::ptr create(const sockaddr *addr, socklen_t addrlen);
        /**
         * @brief 通过host返回满足对应条件的所有Address
         *
         * @param result 满足条件Address
         * @param host 域名，服务器名等
         * @param family
         * @param type
         * @param protocol
         * @return true
         * @return false
         */
        static bool lookup(std::vector<Address::ptr> &result, const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);
        /**
         * @brief 通过host返回满足对应条件的任意Address
         *
         * @param host
         * @param family
         * @param type
         * @param protocol
         * @return Address::ptr
         */
        static Address::ptr lookupAny(const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 通过host返回条件的任意IPAddress
         *
         * @param host
         * @param family
         * @param type
         * @param protocol
         * @return std::shared_ptr<IPAddress>
         */
        static std::shared_ptr<IPAddress> lookupAnyIPAddress(const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 返回本机所有网卡的<网卡名，<地址，子网掩码位数>>
         *
         * @param result 保存本机所有地址
         * @param family
         * @return true
         * @return false
         */
        static bool getInterFaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result, int family = AF_INET);

        /**
         * @brief 获取指定网卡的地址和子网掩码位数
         *
         * @param result
         * @param iface
         * @param family
         * @return true
         * @return false
         */
        static bool getInterFaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family = AF_INET);

        virtual ~Address()=default;

        int getFamily() const;
        /**
         * @brief sockaddr* 只读
         *
         * @return const sockaddr*
         */
        virtual const sockaddr *getAddr() const = 0;

        /**
         * @brief sockaddr* 读写
         *
         * @return const sockaddr*
         */
        virtual sockaddr *getAddr() = 0;

        virtual socklen_t getAddrLen() const = 0;
        /**
         * @brief 可读性输出地址
         *
         * @param os
         * @return std::ostream&
         */
        virtual std::ostream &insert(std::ostream &os) const = 0;
        std::string toString() const;
        //std::multimap 等排序
        bool operator<(const Address &other) const;
        bool operator==(const Address &other) const;
        bool operator!=(const Address &other) const;
    };
    /**
     * @brief IP地址基类
     *
     */
    class IPAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<IPAddress>;
        /**
         * @brief 通过域名、ip、服务器名创建
         *
         * @param address 域名、ip、服务器名等
         * @param port 端口号
         * @return IPAddress::ptr
         */
        static IPAddress::ptr create(const char *address, uint16_t port = 0);

        /**
         * @brief 获取该地址的广播地址
         *
         * @param prefixLen 子网掩码位数
         * @return IPAddress::ptr
         */
        virtual IPAddress::ptr broadcastAddress(uint32_t prefixLen) = 0;
        /**
         * @brief 获取地址的网段
         *
         * @param prefixLen 子网掩码位数
         * @return IPAddress::ptr
         */
        virtual IPAddress::ptr networdAddress(uint32_t prefixLen) = 0;

        /**
         * @brief 获取子网掩码地址
         *
         * @param prefixLen 子网掩码位数
         * @return IPAddress::ptr
         */
        virtual IPAddress::ptr subnetMask(uint32_t prefixLen) = 0;

        /**
         * @brief Get the Port object
         *
         * @return uint16_t
         */
        virtual uint16_t getPort() const = 0;

        /**
         * @brief Set the Port object
         *
         * @param port
         */
        virtual void setPort(uint16_t port) = 0;
    };

    /**
     * @brief IPv4地址
     *
     */
    class IPv4Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv4Address>;
        /**
         * @brief 通过点分十进制地址创建
         *
         * @param address 点分十进制地址 （例如：127.0.0.1）
         * @param port 端口号
         * @return IPv4Address::ptr
         */
        static IPv4Address::ptr create(const char *address, uint16_t port = 0);
        /**
         * @brief 使用sockaddr_in 构建
         *
         * @param addr
         */
        IPv4Address(const sockaddr_in &addr);

        /**
         * @brief 使用二进制地址构建
         *
         * @param binAddr
         * @param port
         */
        IPv4Address(u_long binAddr = INADDR_ANY, uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefixLen) override;

        IPAddress::ptr networdAddress(uint32_t prefixLen) override;

        IPAddress::ptr subnetMask(uint32_t prefixLen) override;

        virtual uint16_t getPort() const override;

        virtual void setPort(uint16_t port) override;

    private:
        sockaddr_in _addr{};
    };

    /**
     * @brief IPv6地址
     *
     */
    class IPv6Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv6Address>;
        /**
         * @brief 通过ipv6地址构建
         *
         * @param address ipv6地址字符串
         * @param port 端口号
         * @return IPv6Address::ptr
         */
        static IPv6Address::ptr create(const char *address, uint16_t port = 0);
        /**
         * @brief 使用sockaddr_in6 构建
         *
         * @param addr
         */
        IPv6Address(const sockaddr_in6 &addr);

        IPv6Address();
        /**
         * @brief 使用二进制地址构建
         *
         * @param binAddr
         * @param port
         */
        IPv6Address(const uint8_t binAddr[16], uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefixLen) override;

        IPAddress::ptr networdAddress(uint32_t prefixLen) override;

        IPAddress::ptr subnetMask(uint32_t prefixLen) override;

        virtual uint16_t getPort() const override;

        virtual void setPort(uint16_t port) override;

    private:
        sockaddr_in6 _addr{};
    };

    /**
     * @brief UnixSocket地址
     *
     */
    class UnixAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnixAddress>;

        UnixAddress();

        /**
         * @brief 通过路径构造UnixAddress
         *
         * @param path UnixAddress路径（长度小于UNIX_PATH_MAX）
         */
        UnixAddress(const std::string &path);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        void setAddrLen(uint32_t len);

        std::ostream &insert(std::ostream &os) const override;

        std::string getPath() const;
    private:
        sockaddr_un _addr{};
        socklen_t _length;
    };


     /**
     * @brief 未知地址
     *
     */
    class UnknownAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnknownAddress>;

        UnknownAddress(const sockaddr& addr);
        
        UnknownAddress(int family);
        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;

    private:
        sockaddr _addr{};
    };
    /**
     * @brief 输出Address地址
     *
     * @param os
     * @param addr
     * @return std::ostream&
     */
    std::ostream &operator<<(std::ostream &os, const Address &addr);
} // namespace WebSrv
