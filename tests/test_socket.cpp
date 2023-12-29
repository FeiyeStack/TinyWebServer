#include "TinyWebServer/socket.h"
#include "TinyWebServer/log.h"
#include "TinyWebServer/iomanager.h"
#include <unistd.h>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");

void infoClient(WebSrv::Socket::ptr clientSocket)
{
    while (true)
    {
        const char sendBuff[] = "hello world!";
        SRV_LOG_DEBUG(g_logger) << "send wait ...";
        int ret = clientSocket->send(sendBuff, sizeof(sendBuff));
        if (ret <= 0)
        {
            SRV_LOG_DEBUG(g_logger) << errno << " err str=" << strerror(errno);
            return;
        }
        SRV_LOG_DEBUG(g_logger) << " send bytes " << ret;

        std::string recvBuff;
        recvBuff.resize(1024);
        SRV_LOG_DEBUG(g_logger) << "recv wait ...";
        int ret2 = clientSocket->recv(&recvBuff[0], recvBuff.size());
        if (ret2 <= 0)
        {
            SRV_LOG_DEBUG(g_logger) <<ret2<<"," << errno << " err str=" << strerror(errno)<<recvBuff;
            return;
        }
        recvBuff.resize(ret2);
        SRV_LOG_DEBUG(g_logger) << "recv bytes " << ret2 << ":" << recvBuff;
    }
    clientSocket->close();
}

void acceptClient(WebSrv::Socket::ptr serverSocket,WebSrv::IOManager* ioManage)
{
    while (true)
    {
        SRV_LOG_DEBUG(g_logger) << "accept wait ...";
        WebSrv::Socket::ptr clientSocket = serverSocket->accept();
        SRV_LOG_DEBUG(g_logger) << "new accept";
        if (clientSocket != nullptr)
        {
            ioManage->schedule(std::bind(&infoClient, clientSocket));
        }
    }
}

void testServer(WebSrv::IOManager* ioManage)
{
    WebSrv::Socket::ptr serverSocket = WebSrv::Socket::CreateTCPSocket();
    WebSrv::IPv4Address::ptr addr(new WebSrv::IPv4Address(INADDR_ANY, 8080));
    serverSocket->bind(addr);
    serverSocket->listen();
    SRV_LOG_DEBUG(g_logger) << "listen start";
    ioManage->schedule(std::bind(&acceptClient, serverSocket,ioManage));
}

void testClient(){
    WebSrv::IPAddress::ptr addr= WebSrv::Address::lookupAnyIPAddress("www.baidu.com");
    if(addr){
        SRV_LOG_DEBUG(g_logger)<<"get address: "<<addr->toString();
    }else{
        SRV_LOG_DEBUG(g_logger)<<"address fail";
        return;
    }

    WebSrv::Socket::ptr socket = WebSrv::Socket::CreateTCP(addr);
    addr->setPort(80);
    SRV_LOG_DEBUG(g_logger)<<"address: "<<addr->toString();
    bool ret =  socket->connect(addr);
    if(!ret){
        SRV_LOG_DEBUG(g_logger)<<"connect "<<addr->toString()<<" fail";
        return;
    }

    int ret2;
    const char sendBuff[] = "GET / HTTP/1.0\r\n\r\n";
    SRV_LOG_DEBUG(g_logger)<<"send wait ...";
    ret2 = socket->send(sendBuff, sizeof(sendBuff));
    SRV_LOG_DEBUG(g_logger)<<"send bytes "<<ret2;
    
    std::string recvBuff;
    recvBuff.resize(4096);
    SRV_LOG_DEBUG(g_logger)<<"recv wait ...";
    ret2 = socket->recv(&recvBuff[0],recvBuff.size());
    recvBuff.resize(ret2);
    SRV_LOG_DEBUG(g_logger)<<"recv bytes "<<ret2<<":"<<recvBuff;
    socket->close();
}

void UDPClient(){
    WebSrv::Socket::ptr socket = WebSrv::Socket::CreateUCPSocket();
    WebSrv::IPAddress::ptr addr= WebSrv::IPv4Address::create("127.0.0.1",8080);
    const char sendBuff[] = "hello client";
    SRV_LOG_DEBUG(g_logger)<<"send wait ...";
    int ret =socket->sendTo(sendBuff, sizeof(sendBuff),addr);
    SRV_LOG_DEBUG(g_logger)<<"send bytes "<<ret;
    
    std::string recvBuff;
    recvBuff.resize(4096);
    SRV_LOG_DEBUG(g_logger)<<"recv wait ...";
    ret = socket->recvFrom(&recvBuff[0],recvBuff.size(),addr);
    recvBuff.resize(ret);
    SRV_LOG_DEBUG(g_logger)<<"recv bytes "<<ret<<":"<<recvBuff;
    socket->close();
}


void UnixInfoClient(WebSrv::Socket::ptr clientSocket){
    while (true)
    {
        const char sendBuff[] = "hello Unix Client!";
        SRV_LOG_DEBUG(g_logger) << "send wait ...";
        int ret = clientSocket->send(sendBuff, sizeof(sendBuff));
        if(ret<=0){
            return;
        }
        SRV_LOG_DEBUG(g_logger) << "send bytes " << ret;

        std::string recvBuff;
        recvBuff.resize(1024);
        SRV_LOG_DEBUG(g_logger) << "recv wait ...";
        ret = clientSocket->recv(&recvBuff[0], recvBuff.size(), ret);
        if(ret<=0){
            return;
        }
        recvBuff.resize(ret);
        SRV_LOG_DEBUG(g_logger) << "recv bytes " << ret << ":" << recvBuff;
    }
}

void UnixAcceptClient(WebSrv::Socket::ptr serverSocket){
    SRV_LOG_DEBUG(g_logger) << "accept wait ...";
    WebSrv::Socket::ptr clientSocket = serverSocket->accept();
    SRV_LOG_DEBUG(g_logger) << "new accept";
    if (clientSocket)
    {
        UnixInfoClient(clientSocket);
    }
}

const std::string SERVER_SOCKET = "server.sock";

void UnixServer(){
    WebSrv::Socket::ptr serverSocket = WebSrv::Socket::CreateUnixTCPSocket();
    WebSrv::Address::ptr addr(new WebSrv::UnixAddress(SERVER_SOCKET));
    serverSocket->bind(addr);
    serverSocket->listen();
    UnixAcceptClient(serverSocket);
    SRV_LOG_DEBUG(g_logger)<<"listen close";
    serverSocket->close();
}

void  UnixClient(){
    WebSrv::Address::ptr addr(new WebSrv::UnixAddress(SERVER_SOCKET));
    WebSrv::Socket::ptr socket = WebSrv::Socket::CreateUnixTCPSocket();
    SRV_LOG_DEBUG(g_logger)<<"address: "<<addr->toString();
    bool ret = socket->connect(addr);
    if(!ret){
        SRV_LOG_DEBUG(g_logger)<<"connect "<<addr->toString()<<" fail";
        return;
    }


    const char sendBuff[] = "hello Unix Server";
    SRV_LOG_DEBUG(g_logger)<<"send wait ...";
    int ret2= socket->send(sendBuff, sizeof(sendBuff));
    SRV_LOG_DEBUG(g_logger)<<"send bytes "<<ret2;
    
    std::string recvBuff;
    recvBuff.resize(4096);
    SRV_LOG_DEBUG(g_logger)<<"recv wait ...";
    ret2 = socket->recv(&recvBuff[0],recvBuff.size());
    recvBuff.resize(ret2);
    SRV_LOG_DEBUG(g_logger)<<"recv bytes "<<ret2<<":"<<recvBuff;
}


int main()
{
    WebSrv::IOManager ioManage(2);
    //ioManage.schedule(std::bind(&testServer,&ioManage));
    //ioManage.schedule(&testClient);
    //ioManage.schedule(&UDPClient);
    unlink(SERVER_SOCKET.c_str());
    ioManage.schedule(&UnixServer);
    ioManage.schedule(&UnixClient);
    return 0;
}