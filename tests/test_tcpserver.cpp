#include "TinyWebServer/tcpserver.h"
#include "TinyWebServer/bytearray.h"
#include "TinyWebServer/log.h"
#include <unistd.h>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");

class EchoServer: public WebSrv::TcpServer{
public:
    EchoServer(WebSrv::IOManager *worker = WebSrv::IOManager::getThis(),
                  WebSrv::IOManager *ioWorker = WebSrv::IOManager::getThis(),
                  WebSrv::IOManager *acceptWorker = WebSrv::IOManager::getThis())
       :TcpServer(worker,worker,worker)
    {
    }
    void handleClient(WebSrv::Socket::ptr client) override
    {
        SRV_LOG_DEBUG(g_logger)<<"handleClient: "<<client->toString();
        WebSrv::ByteArray::ptr ba(new WebSrv::ByteArray);
        while(true){
            ba->setPosition(0);
            std::vector<iovec> bufs;
            ba->getWriteBuffers(bufs,1024);
            SRV_LOG_DEBUG(g_logger)<<"recv wait ...";
            int rt= client->recv(&bufs[0],bufs.size());
            if(rt<0){
                SRV_LOG_ERROR(g_logger) << "client fail err=" << errno << "err str=" << strerror(errno);
                break;
            }else if(rt==0){
                SRV_LOG_INFO(g_logger) << "client close: "<<client->toString();
                break;
            }

            SRV_LOG_DEBUG(g_logger)<<"new msg:";
            ba->setPosition(ba->getPosition()+rt);
            ba->setPosition(0);
            SRV_LOG_DEBUG(g_logger)<<ba->toString();
        }
    }
};

void test(){
    EchoServer::ptr es(new EchoServer());
    auto addr = WebSrv::Address::lookupAny("0.0.0.0:8080");
    while(!es->listen(addr)) {
        sleep(2);
    }
    es->start();
}

int main(){
    WebSrv::IOManager ioManager(2);
    ioManager.schedule(test);
    return 0;
}
