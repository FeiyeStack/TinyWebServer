#include "streams/socketstream.h"

namespace WebSrv
{
    SocketStream::SocketStream(Socket::ptr socket, bool owner)
        : _socket(socket), _owner(owner)
    {
    }

    SocketStream::~SocketStream()
    {
        if (_owner && _socket)
        {
            _socket->close();
        }
    }

    int SocketStream::read(void *buffer, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        return _socket->recv(buffer, len);
    }

    int SocketStream::read(ByteArray::ptr ba, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, len);
        int rt = _socket->recv(&iovs[0], iovs.size());
        if (rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    int SocketStream::write(const void *buffer, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        return _socket->send(buffer, len);
    }

    int SocketStream::write(ByteArray::ptr ba, size_t len)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getReadBuffers(iovs, len);
        int rt = _socket->send(&iovs[0], iovs.size());
        if (rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    void SocketStream::close()
    {
        if (_socket)
        {
            _socket->close();
        }
    }

    bool SocketStream::isConnected() const
    {
        return _socket && _socket->isConnected();
    }

    Address::ptr SocketStream::getRemoteAddress()
    {
        if (_socket)
        {
            return _socket->getRemoteAddress();
        }
        return nullptr;
    }

    Address::ptr SocketStream::getLocalAddress()
    {
        if (_socket)
        {
            return _socket->getLocalAddress();
        }
        return nullptr;
    }

    std::string SocketStream::getRemoteAddressString()
    {
        if (_socket)
        {
            auto addr = _socket->getLocalAddress();
            return addr ? addr->toString() : "";
        }
        return "";
    }

    std::string SocketStream::getLocalAddressString()
    {
        if (_socket)
        {
            auto addr = _socket->getLocalAddress();
            return addr ? addr->toString() : "";
        }
        return "";
    }

    int SocketStream::getError() const
    {
        if (_socket)
        {
            return _socket->getError();
        }
        return -1;
    }
} // namespace WebSrv
