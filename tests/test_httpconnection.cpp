#include "TinyWebServer/http/httpconnection.h"
#include "TinyWebServer/log.h"
#include "TinyWebServer/iomanager.h"
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");


// void test_pool() {
//     WebSrv::http::HttpConnectionPool::ptr pool(new WebSrv::http::HttpConnectionPool(
//                 "www.baidu.com", "", 80, false, 10, 1000 * 30, 5));

//     WebSrv::IOManager::getThis()->addTimer(1000, [pool](){
//             auto r = pool->doGet("/", 300);
//             SRV_LOG_INFO(g_logger) << r->toString();
//     }, true);
// }

// void run() {
//     WebSrv::Address::ptr addr = WebSrv::Address::lookupAnyIPAddress("www.sylar.top:80");
//     if(!addr) {
//         SRV_LOG_INFO(g_logger) << "get addr error";
//         return;
//     }

//     WebSrv::Socket::ptr sock = WebSrv::Socket::CreateTCP(addr);
//     bool rt = sock->connect(addr);
//     if(!rt) {
//         SRV_LOG_INFO(g_logger) << "connect " << *addr << " failed";
//         return;
//     }

//     WebSrv::http::HttpConnection::ptr conn(new WebSrv::http::HttpConnection(sock));
//     WebSrv::http::HttpRequest::ptr req(new WebSrv::http::HttpRequest);
//     req->setPath("/blog/");
//     req->setHeader("host", "www.sylar.top");
//     SRV_LOG_INFO(g_logger) << "req:" << std::endl
//         << *req;

//     conn->sendRequest(req);
//     auto rsp = conn->recvResponse();

//     if(!rsp) {
//         SRV_LOG_INFO(g_logger) << "recv response error";
//         return;
//     }
//     SRV_LOG_INFO(g_logger) << "rsp:" << std::endl
//         << *rsp;

//     std::ofstream ofs("rsp.dat");
//     ofs << *rsp;

//     SRV_LOG_INFO(g_logger) << "=========================";

//     auto r = WebSrv::http::HttpConnection::doGet("http://www.sylar.top/blog/", 300);
//     SRV_LOG_INFO(g_logger) << "result=" << r->error
//         << " error=" << r->errorStr
//         << " rsp=" << (r->response ? r->response->toString() : "");

//     SRV_LOG_INFO(g_logger) << "=========================";
//     test_pool();
// }

void testHttp() {
    auto r = WebSrv::http::HttpConnection::doGet("http://www.baidu.com/", 300, {
                        {"Connection", "keep-alive"},
                        {"User-Agent", "curl/7.29.0"}
            });
    SRV_LOG_INFO(g_logger) << "result=" << r->error
        << " error=" << r->errorStr
        << " rsp=" << (r->response ? r->response->toString() : "");

    //sylar::http::HttpConnectionPool::ptr pool(new sylar::http::HttpConnectionPool(
    //            "www.baidu.com", "", 80, false, 10, 1000 * 30, 5));
    auto pool = WebSrv::http::HttpConnectionPool::Create(
                    "http://www.baidu.com", "", 10, 1000 * 30, 5);
    WebSrv::IOManager::getThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 3000, {
                        {"User-Agent", "curl/7.29.0"}
                    });
            SRV_LOG_INFO(g_logger) << r->toString();
    }, true);
}

int main(){
    WebSrv::IOManager ioManager(2);

    ioManager.schedule(testHttp);
    return 0;
}