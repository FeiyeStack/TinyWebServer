#include "address.h"
#include <string>
#include "log.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace WebSrv
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");
    // 生成不同位数的掩码
    template <class T>
    static T createMask(uint32_t bits)
    {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }

    template <class T>
    static uint32_t countBytes(T value)
    {
        uint32_t result = 0;
        for (; value; ++result)
        {
            value &= value - 1;
        }
        return result;
    }

    Address::ptr Address::create(const sockaddr *addr, socklen_t addrlen)
    {
        if (addr == nullptr)
        {
            return nullptr;
        }
        Address::ptr res;
        switch (addr->sa_family)
        {
        case AF_INET:
            res.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            res.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            res.reset(new UnknownAddress(*addr));
            break;
            // unix用路径不用这个
        }
        return res;
    }

    bool Address::lookup(std::vector<Address::ptr> &result, const std::string &host, int family, int type, int protocol)
    {
        addrinfo hints{}, *results, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char *service = nullptr;
        // 检查是否是ipv6 主机名 [ipv6]:port
        if (!host.empty() && host[0] == '[')
        {
            const char *endIpv6 = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1); // 查找第一个为]的字符
            if (endIpv6)
            {
                // 添加端口号
                if (*(endIpv6 + 1) == ':')
                {
                    service = endIpv6 + 2;
                }
                // ipv6
                node = host.substr(1, endIpv6 - host.c_str() - 1);
            }
        }
        // 为空,检查 ipv4:port  / 主机名:port
        if (node.empty())
        {
            service = (const char *)memchr(host.c_str(), ':', host.size());
            if (service)
            {
                // 是不是最后一个:
                if (!memchr(service + 1, ':', host.size() - (service - host.c_str()) - 1))
                {
                    node = host.substr(0, service - host.c_str());
                    ++service; // 端口
                }
            }
        }
        // 可能为域名
        if (node.empty())
        {
            node = host;
        }

        int ret = getaddrinfo(node.c_str(), service, &hints, &results);
        if (ret)
        {
            SRV_LOG_DEBUG(g_logger) << "Address::lookup (" << host << ", " << family << "," << type << ") err=" << ret << "[" << gai_strerror(ret) << "]";
            return false;
        }

        next = results;
        while (next)
        {
            result.emplace_back(create(next->ai_addr, (socklen_t)next->ai_addrlen));
            next = next->ai_next;
        }
        freeaddrinfo(results);
        return !result.empty();
    }

    Address::ptr Address::lookupAny(const std::string &host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (lookup(result, host, family, type))
        {
            return result[0];
        }
        return nullptr;
    }

    std::shared_ptr<IPAddress> Address::lookupAnyIPAddress(const std::string &host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (lookup(result, host, family, type))
        {
            for (auto &addr : result)
            {
                IPAddress::ptr ipaddr = std::dynamic_pointer_cast<IPAddress>(addr);
                if (ipaddr)
                {
                    return ipaddr;
                }
            }
        }
        return nullptr;
    }

    bool Address::getInterFaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result, int family)
    {
        struct ifaddrs *next, *results;
    if(getifaddrs(&results) != 0) {
        SRV_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = countBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += countBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                            std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        SRV_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
    }

    bool Address::getInterFaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family)
    {
        // iface 空 或 * 返回空结果
        if (iface.empty() || iface == "*")
        {
            if (family == AF_INET || family == AF_UNSPEC)
            {
                result.push_back({Address::ptr(new IPv4Address()), 0u});
            }
            if (family == AF_INET6 || family == AF_UNSPEC)
            {
                result.push_back({Address::ptr(new IPv6Address()), 0u});
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> res;
        if (!getInterFaceAddresses(res, family))
        {
            return false;
        }

        auto it = res.equal_range(iface); // 返回该key的第一个值和最后一个值迭代器
        for (; it.first != it.second; ++it.first)
        {
            result.push_back(it.first->second);
        }
        return !result.empty();
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address &other) const
    {
        socklen_t minlen = std::min(getAddrLen(), other.getAddrLen());
        // 比较字节数
        int res = memcmp(getAddr(), other.getAddr(), minlen);
        if (res < 0)
        {
            return true;
        }
        else if (res > 0)
        {
            return false;
        }
        else if (getAddrLen() < other.getAddrLen())
        {
            return true;
        }
        return false;
    }

    bool Address::operator==(const Address &other) const
    {
        return getAddrLen() == other.getAddrLen() && memcmp(getAddr(), other.getAddr(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address &other) const
    {
        return !(*this == other);
    }

    IPAddress::ptr IPAddress::create(const char *address, uint16_t port)
    {
        addrinfo hints{}, *results;
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;
        int ret = getaddrinfo(address, NULL, &hints, &results);
        if (ret)
        {
            SRV_LOG_DEBUG(g_logger) << "IPAddress::Create (" << address << ", " << port << ") err=" << ret << "[" << gai_strerror(ret) << "]";
            return nullptr;
        }

        try
        {
            IPAddress::ptr res = std::dynamic_pointer_cast<IPAddress>(Address::create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if (res)
            {
                res->setPort(port);
            }
            freeaddrinfo(results);
            return res;
        }
        catch (...)
        {
            freeaddrinfo(results);
            return nullptr;
        }
    }

    IPv4Address::ptr IPv4Address::create(const char *address, uint16_t port)
    {
        IPv4Address::ptr ipv4Addr(new IPv4Address);
        ipv4Addr->_addr.sin_port = htons(port);
        int ret = inet_pton(AF_INET, address, &ipv4Addr->_addr.sin_addr);
        if (ret <= 0)
        {
            SRV_LOG_DEBUG(g_logger) << "IPv4Address::Create (" << address << ", " << port << ") err=" << ret << "[" << gai_strerror(ret) << "]";
            return nullptr;
        }
        return ipv4Addr;
    }

    IPv4Address::IPv4Address(const sockaddr_in &addr)
    {
        _addr = addr;
    }

    IPv4Address::IPv4Address(u_long binAddr, uint16_t port)
    {
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        _addr.sin_addr.s_addr = htonl(binAddr);
    }

    const sockaddr *IPv4Address::getAddr() const
    {
        return (sockaddr *)&_addr;
    }

    sockaddr *IPv4Address::getAddr()
    {
        return (sockaddr *)&_addr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(_addr);
    }

    std::ostream &IPv4Address::insert(std::ostream &os) const
    {
        // 转换成字符串表达
        uint32_t addr = ntohl(_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
           << ((addr >> 16) & 0xff) << "."
           << ((addr >> 8) & 0xff) << "."
           << ((addr) & 0xff);
        os << ":" << ntohs(_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefixLen)
    {
        if (prefixLen > 32)
        {
            return nullptr;
        }
        sockaddr_in bAddr(_addr);
        bAddr.sin_addr.s_addr |= htonl(createMask<uint32_t>(prefixLen));
        return IPv4Address::ptr(new IPv4Address(bAddr));
    }

    IPAddress::ptr IPv4Address::networdAddress(uint32_t prefixLen)
    {
        if (prefixLen > 32)
        {
            return nullptr;
        }
        sockaddr_in bAddr(_addr);
        bAddr.sin_addr.s_addr &= ~htonl(createMask<uint32_t>(prefixLen));
        return IPv4Address::ptr(new IPv4Address(bAddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefixLen)
    {
        sockaddr_in subnet{};
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~htonl(createMask<uint32_t>(prefixLen));
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint16_t IPv4Address::getPort() const
    {
        return ntohs(_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t port)
    {
        _addr.sin_port = htons(port);
    }

    IPv6Address::ptr IPv6Address::create(const char *address, uint16_t port)
    {
        IPv6Address::ptr ipv6Addr(new IPv6Address);
        ipv6Addr->_addr.sin6_port = htons(port);
        // 将ip地址转为二进制
        int ret = inet_pton(AF_INET6, address, &ipv6Addr->_addr.sin6_addr);
        if (ret <= 0)
        {
            SRV_LOG_DEBUG(g_logger) << "IPv6Address::Create getaddrinfo(" << address << ", " << port << ") err=" << ret << "[" << gai_strerror(ret) << "]";
            return nullptr;
        }
        return ipv6Addr;
    }

    IPv6Address::IPv6Address(const sockaddr_in6 &addr)
    {
        _addr = addr;
    }

    IPv6Address::IPv6Address()
    {
        _addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const uint8_t binAddr[16], uint16_t port)
    {
        _addr.sin6_family = AF_INET6;
        _addr.sin6_port = htons(port);
        memcpy(&_addr.sin6_addr.s6_addr, binAddr, 16);
    }

    const sockaddr *IPv6Address::getAddr() const
    {
        return (sockaddr *)&_addr;
    }

    sockaddr *IPv6Address::getAddr()
    {
        return (sockaddr *)&_addr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(_addr);
    }

    std::ostream &IPv6Address::insert(std::ostream &os) const
    {
        os << "[";
        uint16_t *addr = (uint16_t *)_addr.sin6_addr.s6_addr;
        bool usedZeros = false;
        for (size_t i = 0; i < 8; ++i)
        {
            if (addr[i] == 0 && !usedZeros)
            {
                continue;
            }
            // 如果前一块为0则说明为0块 用（::）表示 xxxx:（0000）:xxxx 只使用一次，避免歧义
            if (i && addr[i - 1] == 0 && !usedZeros)
            {
                os << ":";
                usedZeros = true;
            }
            if (i)
            {
                os << ":";
            }
            os << std::hex << ntohs(addr[i]) << std::dec;
        }
        // 为true 说明最后两块为0
        if (!usedZeros && addr[7] == 0)
        {
            os << "::";
        }
        os << "]:" << ntohs(_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefixLen)
    {
        if (prefixLen > 128)
        {
            return nullptr;
        }
        sockaddr_in6 bAddr(_addr);
        bAddr.sin6_addr.s6_addr[prefixLen / 8] |= createMask<uint8_t>(prefixLen % 8);
        for (int i = prefixLen / 8 + 1; i < 16; ++i)
        {
            bAddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(bAddr));
    }

    IPAddress::ptr IPv6Address::networdAddress(uint32_t prefixLen)
    {
        if (prefixLen > 128)
        {
            return nullptr;
        }
        sockaddr_in6 bAddr(_addr);
        bAddr.sin6_addr.s6_addr[prefixLen / 8] &= ~createMask<uint8_t>(prefixLen % 8);
        for (int i = prefixLen / 8 + 1; i < 16; ++i)
        {
            bAddr.sin6_addr.s6_addr[i] = 0x00;
        }
        return IPv6Address::ptr(new IPv6Address(bAddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefixLen)
    {
        if (prefixLen > 128)
        {
            return nullptr;
        }
        sockaddr_in6 subnet{};
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefixLen / 8] = ~createMask<uint8_t>(prefixLen % 8);
        for (int i = 0; i < prefixLen / 8; ++i)
        {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint16_t IPv6Address::getPort() const
    {
        return ntohs(_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t port)
    {
        _addr.sin6_port = htons(port);
    }

    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

    UnixAddress::UnixAddress()
    {
        _addr.sun_family = AF_UNIX;
        _length = offsetof(sockaddr_un, sockaddr_un::sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string &path)
    {
        _addr.sun_family = AF_UNIX;
        _length = path.size() + 1;
        if (!path.empty() && path[0] == '\0')
        {
            --_length;
        }

        if (_length > sizeof(_addr.sun_path))
        {
            throw std::logic_error("path too long");
        }
        memcpy(_addr.sun_path, path.c_str(), _length);
        _length += offsetof(sockaddr_un, sockaddr_un::sun_path);
    }

    const sockaddr *UnixAddress::getAddr() const
    {
        return (sockaddr *)&_addr;
    }

    sockaddr *UnixAddress::getAddr()
    {
        return (sockaddr *)&_addr;
    }

    socklen_t UnixAddress::getAddrLen() const
    {
        return sizeof(_addr);
    }

    void UnixAddress::setAddrLen(uint32_t len)
    {
        _length=len;
    }

    std::ostream &UnixAddress::insert(std::ostream &os) const
    {
        if (_length > offsetof(sockaddr_un, sockaddr_un::sun_path) && _addr.sun_path[0] == '\0')
        {
            os << "\\0" << std::string(_addr.sun_path + 1, _length - offsetof(sockaddr_un, sockaddr_un::sun_path) - 1);
        }
        else
        {
            os << _addr.sun_path;
        }
        return os;
    }

    std::string UnixAddress::getPath() const
    {
        std::stringstream ss;
        //_length 大于sockaddr_un::sun_path偏移量并且使用抽象命名空间，忽略\0输出抽象命名空间路径（\0my_abstract_socket），否则直接输出路径 （\my_socket）
        if (_length > offsetof(sockaddr_un, sockaddr_un::sun_path) && _addr.sun_path[0] == '\0')
        {
            ss << "\\0" << std::string(_addr.sun_path + 1, _length - offsetof(sockaddr_un, sockaddr_un::sun_path) - 1);
        }
        else
        {
            ss << _addr.sun_path;
        }
        return ss.str();
    }

    UnknownAddress::UnknownAddress(const sockaddr &addr)
    {
        _addr = addr;
    }

    UnknownAddress::UnknownAddress(int family)
    {
        _addr.sa_family = family;
    }

    const sockaddr *UnknownAddress::getAddr() const
    {
        return (sockaddr *)&_addr;
    }

    sockaddr *UnknownAddress::getAddr()
    {
        return (sockaddr *)&_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const
    {
        return sizeof(_addr);
    }

    std::ostream &UnknownAddress::insert(std::ostream &os) const
    {
        os << "[UnknownAddress family=" << _addr.sa_family << "]";
        return os;
    }

    std::ostream &operator<<(std::ostream &os, const Address &addr)
    {
        return addr.insert(os);
    }

} // namespace WebSrv