#include "TinyWebServer/address.h"
#include "TinyWebServer/log.h"
#include <vector>
#include <cstring>
#include <map>
#include <netinet/in.h>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");


void test()
{
    std::vector<WebSrv::Address::ptr> res;
    WebSrv::Address::lookup(res, "www.baidu.com", AF_UNSPEC, SOCK_STREAM);
    WebSrv::Address::lookup(res, "127.0.0.1:80");
    WebSrv::Address::lookup(res, "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:80", AF_INET6);

    for (auto &&i : res)
    {
        SRV_LOG_DEBUG(g_logger) << *i;
    }
    WebSrv::UnixAddress::ptr un(new WebSrv::UnixAddress(std::string("\0my_abstract_path", 17)));
    WebSrv::UnixAddress::ptr un2(new WebSrv::UnixAddress("my_path"));
    SRV_LOG_DEBUG(g_logger)  << "abstract_path" << *un << "------ path:" << *un2;

    WebSrv::UnknownAddress::ptr unknown(new WebSrv::UnknownAddress(AF_NETBEUI));
    SRV_LOG_DEBUG(g_logger)  << *unknown;

    WebSrv::IPAddress::ptr p = WebSrv::IPv4Address::create("192.168.120.1", 80);
    SRV_LOG_DEBUG(g_logger)  << *p;
    SRV_LOG_DEBUG(g_logger)  << *(p->broadcastAddress(24));
    SRV_LOG_DEBUG(g_logger)  << *(p->networdAddress(24));
    SRV_LOG_DEBUG(g_logger)  << *(p->subnetMask(24));
    WebSrv::IPAddress::ptr p2 = WebSrv::IPv6Address::create("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 80);
    SRV_LOG_DEBUG(g_logger)  << *p2;
    SRV_LOG_DEBUG(g_logger)  << *(p2->broadcastAddress(64));
    SRV_LOG_DEBUG(g_logger)  << *(p2->networdAddress(64));
    SRV_LOG_DEBUG(g_logger)  << *(p2->subnetMask(64));
}

void test2()
{
    std::multimap<std::string, std::pair<WebSrv::Address::ptr, uint32_t>> res;
    WebSrv::Address::getInterFaceAddresses(res, AF_UNSPEC);

    for (auto &&[adapter, p] : res)
    {
        SRV_LOG_DEBUG(g_logger)  << "adapter:" << adapter << "," << *p.first << "|" << p.second;
    }
}

int main()
{
    test();
    test2();
}