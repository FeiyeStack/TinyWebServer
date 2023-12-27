#include <iostream>
#include "TinyWebServer/log.h"
#include "TinyWebServer/iomanager.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");

int sock = 0;

void testFiber()
{
    SRV_LOG_DEBUG(g_logger) << "test_fiber sock=" << sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "124.223.0.94", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        SRV_LOG_DEBUG(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        WebSrv::IOManager::getThis()->addEvent(sock, WebSrv::IOManager::READ, []()
                                               { SRV_LOG_DEBUG(g_logger) << "read callback"; });
        WebSrv::IOManager::getThis()->addEvent(sock, WebSrv::IOManager::WRITE, []()
                                               {
            SRV_LOG_DEBUG(g_logger) << "write callback";
            WebSrv::IOManager::getThis()->cancelEvent(sock, WebSrv::IOManager::READ);
            close(sock); });
    }
    else
    {
        SRV_LOG_DEBUG(g_logger) << "else " << errno << " " << strerror(errno);
    }
}
void test1()
{
    SRV_LOG_DEBUG(g_logger) << "EPOLLIN=" << EPOLLIN
                            << " EPOLLOUT=" << EPOLLOUT << std::endl;
    WebSrv::IOManager ioManager(2, false);
    ioManager.schedule(&testFiber);
}

void testTimeTask()
{
    WebSrv::IOManager ioManage(2,false);
    ioManage.start();
    ioManage.addTimer(300, []()
                        { SRV_LOG_DEBUG(g_logger)<< "task =" << 300 << " ms"; });
    auto timer1 = ioManage.addTimer(2000, []()
                                      { SRV_LOG_DEBUG(g_logger)<< "task =" << 2000 << " ms"; });
    ioManage.addTimer(
        500, []()
        { SRV_LOG_DEBUG(g_logger)<< "task r=" << 500 << " ms"; },
        true);

    ioManage.addTimer(
        300, []()
        { SRV_LOG_DEBUG(g_logger)<< "task r=" << 300 << " ms"; },
        true);
    bool ok = true;
    bool *p=&ok;
    auto okCb = [&p]() -> bool
    {
        if(p!=nullptr)
            return *p;
        else
            return false;
    };
    ioManage.addConditionTimer(
        400,
        []()
        {
            SRV_LOG_DEBUG(g_logger)<< "task r=" << 400 << " ms";
        },
        okCb,
        true);
    ioManage.addTimer(200, []()
                        { SRV_LOG_DEBUG(g_logger)<< "task =" << 200 << " ms"; });
    ioManage.addTimer(800, []()
                        { SRV_LOG_DEBUG(g_logger)<< "task =" << 800 << " ms"; });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    timer1->reset(100, false);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    p=nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char **argv)
{
    //test1();
    testTimeTask();
    return 0;
}
