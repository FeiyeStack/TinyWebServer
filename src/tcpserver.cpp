#include "tcpserver.h"
#include "configurator.h"
#include "log.h"
#include "socket.h"
namespace WebSrv
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");

    static ConfigVar<uint64_t>::ptr g_tcpServerReadTimeout=Configurator::lookup("tcp_server.read_timeout",(uint64_t)(60*1000*2),"tcp server read timeout");
    static ConfigVar<uint64_t>::ptr g_tcpServerSendTimeout=Configurator::lookup("tcp_server.send_timeout",(uint64_t)(60*1000*2),"tcp server send timeout");

    TcpServer::TcpServer(IOManager *worker, IOManager *ioWorker,
                         IOManager *acceptWorker)
        : _worker(worker), _ioWorker(ioWorker), _acceptWorker(acceptWorker),
          _recvTimeout(g_tcpServerReadTimeout->getValue()),
          _sendTimeout(g_tcpServerSendTimeout->getValue()),
          _name("TcpServer/1.0.0"), _stop(true)
    {
    }

    TcpServer::~TcpServer()
    {
        for(auto& s:_socks){
            s->close();
        }
        _socks.clear();
    }

    bool TcpServer::listen(Address::ptr addr)
    {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.emplace_back(addr);
        return listen(addrs,fails);
    }

    bool TcpServer::listen(const std::vector<Address::ptr> &addrs, std::vector<Address::ptr> &fails)
    {
        for (auto &addr : addrs)
        {
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock->bind(addr))
            {
               SRV_LOG_ERROR(g_logger) << "bind fail "
                                       << "addr=[" << addr->toString() << "]";
                fails.emplace_back(addr);
                continue;
            }
            if (!sock->listen())
            {
               SRV_LOG_ERROR(g_logger) << "listen fail " << "addr=[" << sock->toString() << "]";
                fails.emplace_back(addr);
                continue;
            }
            _socks.emplace_back(sock);
        }
        if (!fails.empty())
        {
            _socks.clear();
            return false;
        }

        for (auto &i : _socks)
        {
            SRV_LOG_INFO(g_logger) << "type=" << _type
                                  << " name=" << _name
                                  << " server bind success: " << i->toString();
        }
        return true;
    }

    void TcpServer::start()
    {
        if (!_stop)
        {
            return;
        }
        _stop = false;
        for (auto &sock : _socks)
        {
            _acceptWorker->schedule(std::bind(&TcpServer::startAccept,shared_from_this(),sock));
        }
    }

    void TcpServer::stop()
    {
        _stop=false;
        _acceptWorker->schedule([this](){
            for(auto& sock:_socks){
                IOManager::getThis()->cancelAll(*sock);
                sock->close();
            }
            _socks.clear();
        });
    }

    std::string TcpServer::toString(const std::string &prefix)
    {
        std::stringstream ss;
        ss << prefix << "[type=" << _type
           << " name=" << _name 
           << " worker=" << (_worker ? _worker->getName() : "")
           << " accept=" << (_acceptWorker ? _acceptWorker->getName() : "")
           << " recv_timeout=" << _recvTimeout << "]" << std::endl;
        std::string pfx = prefix.empty() ? "    " : prefix;
        for (auto &i : _socks)
        {
            ss << pfx << pfx << i->toString() << std::endl;
        }
        return ss.str();
    }

    void TcpServer::handleClient(Socket::ptr client)
    {
        SRV_LOG_INFO(g_logger)<<"handleClient: "<<client->toString();
    }

    void TcpServer::startAccept(Socket::ptr sock)
    {   
        while (!_stop)
        {
            Socket::ptr client =  sock->accept();
            if (client)
            {
                client->setRecvTimeout(_recvTimeout);
                _ioWorker->schedule(std::bind(&TcpServer::handleClient,shared_from_this(),client));
            }
            else
            {
               SRV_LOG_ERROR(g_logger) << "accept errno=" << errno << " errno str=" << strerror(errno);
            }
        }
    }

} // namespace WebServer
