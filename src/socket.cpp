#include "socket.h"
#include "log.h"
#include "fdmanager.h"
#include <netinet/tcp.h>
#include <unistd.h>
#include "hook.h"
namespace WebSrv
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");

    Socket::ptr Socket::CreateTCP(Address::ptr address)
    {
        Socket::ptr socket(new Socket(address->getFamily(), TCP, 0));
        return socket;
    }

    Socket::ptr Socket::CreateUDP(Address::ptr address)
    {
        Socket::ptr socket(new Socket(address->getFamily(), UDP, 0));
        socket->newSocket();
        socket->_isConnected = true; // 不关心连接
        return socket;
    }

    Socket::ptr Socket::CreateTCPSocket()
    {
        Socket::ptr socket(new Socket(Ipv4, TCP, 0));
        return socket;
    }

    Socket::ptr Socket::CreateUCPSocket()
    {
        Socket::ptr socket(new Socket(Ipv4, UDP, 0));
        socket->newSocket();
        socket->_isConnected = true; // 不关心连接
        return socket;
    }

    Socket::ptr Socket::CreateTCPSocket6()
    {
        Socket::ptr socket(new Socket(Ipv6, TCP, 0));
        return socket;
    }

    Socket::ptr Socket::CreateUCPSocket6()
    {
        Socket::ptr socket(new Socket(Ipv6, UDP, 0));
        socket->newSocket();
        socket->_isConnected = true; // 不关心连接
        return socket;
    }

    Socket::ptr Socket::CreateUnixTCPSocket()
    {
        Socket::ptr socket(new Socket(UNIX, TCP, 0));
        socket->newSocket();
        return socket;
    }

    Socket::ptr Socket::CreateUnixUCPSocket()
    {
        Socket::ptr socket(new Socket(UNIX, UDP, 0));
        socket->newSocket();
        return socket;
    }

    Socket::Socket(int family, int type, int protocol)
        : _socket(-1),
          _family(family),
          _type(type),
          _protocol(protocol),
          _isConnected(false)
    {
    }

    Socket::~Socket()
    {
        close();
    }

    bool Socket::getOption(int level, int option, void *result, socklen_t *len)
    {
        int ret = getsockopt(_socket, level, option, result, len);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "getsockopt (" << _socket << "," << level << "," << option << ","
                                    << "," << option << "...) error=" << errno << "errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void *value, socklen_t len)
    {
        int ret = setsockopt(_socket, level, option, value, len);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "setsockopt (" << _socket << "," << level << "," << option << ","
                                    << "," << value << "...) error=" << errno << "errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    int64_t Socket::getSendTimeout()
    {
        FdCtx::ptr ctx = FdManager::getFdManger()->get(_socket);
        if (ctx)
        {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        else
        {
            struct timeval tv;
            socklen_t len = sizeof(tv);
            int ret = getOption(SOL_SOCKET, SO_SNDTIMEO, &tv, &len);
            if (ret != -1)
            {
                return tv.tv_sec * 1000 + tv.tv_usec / 1000;
            }
        }
        return -1;
    }

    void Socket::setSendTimeout(int64_t timeout)
    {
        if (timeout < 0)
        {
            return;
        }
        struct timeval tv
        {
            int(timeout / 1000), int(timeout % 1000 * 1000)
        };
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
        FdCtx::ptr ctx = FdManager::getFdManger()->get(_socket);
        if (ctx)
        {
            ctx->setTimeout(SO_SNDTIMEO, timeout);
        }
    }

    int64_t Socket::getRecvTimeout()
    {
        FdCtx::ptr ctx = FdManager::getFdManger()->get(_socket);
        if (ctx)
        {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        else
        {
            struct timeval tv;
            socklen_t len = sizeof(tv);
            int ret = getOption(SOL_SOCKET, SO_RCVTIMEO, &tv, &len);
            if (ret != -1)
            {
                return tv.tv_sec * 1000 + tv.tv_usec / 1000;
            }
        }
        return -1;
    }

    void Socket::setRecvTimeout(int64_t timeout)
    {
        if (timeout < 0)
        {
            return;
        }
        struct timeval tv
        {
            int(timeout / 1000), int(timeout % 1000 * 1000)
        };
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
        FdCtx::ptr ctx = FdManager::getFdManger()->get(_socket);
        if (ctx)
        {
            ctx->setTimeout(SO_RCVTIMEO, timeout);
        }
    }

    int Socket::getError()
    {
        int error = 0;
        if (!getOption(SOL_SOCKET, SO_ERROR, errno))
        {
            error = errno;
        }
        return error;
    }

    bool Socket::bind(Address::ptr address)
    {
        if (!isValid())
        {
            newSocket();
            if (!isValid())
            {
                return false;
            }
        }
        if (address->getFamily() != _family)
        {
            SRV_LOG_ERROR(g_logger) << "bind socket.family= " << _family << ", address.family" << address->getFamily() << "on equal, address=" << address->toString();
            return false;
        }

        if (::bind(_socket, address->getAddr(), address->getAddrLen()))
        {
            SRV_LOG_ERROR(g_logger) << "bind error errno=" << errno << " err str=" << strerror(errno);
            return false;
        }
        // 获取本地地址
        initLocalAddress();
        return true;
    }

    bool Socket::listen(int backlog)
    {
        if (!isValid())
        {
            SRV_LOG_ERROR(g_logger) << "socket not created";
            return false;
        }
        if (::listen(_socket, backlog))
        {
            SRV_LOG_ERROR(g_logger) << "listen error errno=" << errno << "err str=" << strerror(errno);
        }
        return true;
    }

    bool Socket::close()
    {
        if (!_isConnected && !isValid())
        {
            return true;
        }
        _isConnected = false;
        if (isValid())
        {
            if (::close(_socket) == -1)
            {
                return false;
            }
            _socket = -1;
        }
        return true;
    }

    Socket::ptr Socket::accept()
    {
        Socket::ptr sock(new Socket(_family, _type, _protocol));
        int newSock = ::accept(_socket, nullptr, nullptr);
        if (newSock == -1)
        {
            SRV_LOG_ERROR(g_logger) << "accept (" << _socket << ") errno="
                                    << errno << "errno string=" << strerror(errno);
        }
        if (sock->init(newSock))
        {
            return sock;
        }
        return nullptr;
    }

    bool Socket::connect(const Address::ptr addr, int64_t timeoutMs)
    {
        _remoteAddress = addr;
        if (!isValid())
        {
            newSocket();
            if (!isValid())
            {
                return false;
            }
        }
        if (addr->getFamily() != _family)
        {
            SRV_LOG_ERROR(g_logger) << "The connected family and socket family do not match , sock.family="
                                    << _family << " ,addr.family=" << addr->getFamily() << " addr=" << addr->toString();
            return true;
        }

        if (timeoutMs == -1)
        {
            if (::connect(_socket, addr->getAddr(), addr->getAddrLen()) == -1)
            {
                SRV_LOG_ERROR(g_logger) << "connect (" << _socket << "," << addr->toString() << ") errno="
                                        << errno << "errno string=" << strerror(errno);
                close();
                return false;
            }
        }
        else
        {
            if (connectWithTimeout(_socket, addr->getAddr(), addr->getAddrLen(), timeoutMs) == -1)
            {
                SRV_LOG_ERROR(g_logger) << "connect (" << _socket << "," << addr->toString() << "," << timeoutMs << ") errno="
                                        << errno << "errno string=" << strerror(errno);
                close();
                return false;
            }
        }
        _isConnected = true;
        initRemoteAddress();
        initLocalAddress();
        return true;
    }

    bool Socket::reconnect(int64_t timeoutMs)
    {
        if (!_remoteAddress)
        {
            SRV_LOG_ERROR(g_logger) << "not yet connected";
            return false;
        }
        _localAddress.reset();
        return connect(_remoteAddress, timeoutMs);
    }

    int Socket::send(const void *buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::send(_socket, buffer, length, flags);
        }
        return -1;
    }

    int Socket::send(iovec *buffers, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            return ::sendmsg(_socket, &msg, flags);
        }
        return -1;
    }

    int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to, int flags)
    {
        if (isConnected())
        {
            return ::sendto(_socket, buffer, length, flags, to->getAddr(), to->getAddrLen());
        }
        return -1;
    }

    int Socket::sendTo(iovec *buffers, size_t length, const Address::ptr to, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = to->getAddr();
            msg.msg_namelen = to->getAddrLen();
            return ::recvmsg(_socket, &msg, flags);
        }
        return -1;
    }

    int Socket::recv(void *buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::recv(_socket, buffer, length, flags);
        }
        return -1;
    }

    int Socket::recv(iovec *buffers, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            return ::recvmsg(_socket, &msg, flags);
        }
        return -1;
    }

    int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(_socket, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }

    int Socket::recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec *)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrLen();
            return ::recvmsg(_socket, &msg, flags);
        }
        return -1;
    }

    std::ostream &Socket::dump(std::ostream &os) const
    {
        os << "[Socket sock=" << _socket
           << " is_connected=" << _isConnected
           << " family=" << _family
           << " type=" << _type
           << " protocol=" << _protocol;
        if (_localAddress)
        {
            os << " local_address=" << _localAddress->toString();
        }
        if (_remoteAddress)
        {
            os << " remote_address=" << _remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string Socket::toString() const
    {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    void Socket::newSocket()
    {
        _socket = socket(_family, _type, _protocol);
        if (_socket == -1)
        {
            SRV_LOG_ERROR(g_logger) << "socket (" << _family << "," << _type << "," << _protocol << ") error=" << errno << "errstr=" << strerror(errno);
        }
        if (_family == UNIX)
        {
            // 什么都不做
        }
        else if (isValid())
        {
            initSocket();
        }
        else
        {
            SRV_LOG_ERROR(g_logger) << "socket (" << _family << "," << _type << "," << _protocol << ") error=" << errno << "errstr=" << strerror(errno);
        }
    }

    void Socket::initLocalAddress()
    {
        Address::ptr res;
        switch (_family)
        {
        case AF_INET:
            res.reset(new IPv4Address());
            break;
        case AF_INET6:
            res.reset(new IPv6Address());
            break;
        case AF_UNIX:
            res.reset(new UnixAddress());
            break;
        default:
            res.reset(new UnknownAddress(_family));
            break;
        }
        socklen_t addrlen = res->getAddrLen();
        if (getsockname(_socket, res->getAddr(), &addrlen))
        {
            SRV_LOG_ERROR(g_logger) << "getsockname (" << _socket << ") error=" << errno << "errstr=" << strerror(errno);
            return;
        }

        if (_family == AF_UNIX)
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(res);
            addr->setAddrLen(addrlen);
        }
        _localAddress = res;
    }

    void Socket::initSocket()
    {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val); // SOL_SOCKET通用套接字选项 设置SO_REUSEADDR 允许在套接字关闭后立即重新使用之前使用的地址
        if (_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val); // IPPROTO_TCP 通用tcp套接字选项 设置TCP_NODELAY 禁用 Nagle 算法，该算法会在发送数据时进行延迟，以便合并小的数据块。
        }
    }
    void Socket::initRemoteAddress()
    {
        Address::ptr result;
        switch (_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(_family));
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        if (getpeername(_socket, result->getAddr(), &addrlen))
        {
            SRV_LOG_ERROR(g_logger) << "getpeername (" << _socket << ") error=" << errno << "errstr=" << strerror(errno);
            return;
        }
        if (_family == AF_UNIX)
        {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        _remoteAddress = result;
    }
    bool Socket::init(int socket)
    {
        FdCtx::ptr ctx = FdManager::getFdManger()->get(socket);
        if (ctx && ctx->isSocket() && !ctx->isClose())
        {
            _socket = socket;
            _isConnected = true;
            if(_family!=UNIX){
                initSocket();
            }
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }
} // namespace WebSrv
