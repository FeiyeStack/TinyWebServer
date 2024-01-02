#include <iostream>
#include "TinyWebServer/http/httpserver.h"


void run(WebSrv::http::HttpServer::ptr httpServer)
{
    auto addr = WebSrv::Address::lookupAny("0.0.0.0:8080");
    while (!httpServer->listen(addr))
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    auto servletDispatch= httpServer->getServletDispatch();
    servletDispatch->addServlet("/home/xx",
                                [](WebSrv::http::HttpRequest::ptr request,
                                   WebSrv::http::HttpResponse::ptr response,
                                   WebSrv::http::HttpSession::ptr session)
                                {
                                    response->setBody(request->toString());
                                    return 0;
                                });
    servletDispatch->addGlobServlet("/home/*",
                                [](WebSrv::http::HttpRequest::ptr request,
                                   WebSrv::http::HttpResponse::ptr response,
                                   WebSrv::http::HttpSession::ptr session)
                                {
                                    response->setBody("Glob:\r\n" + request->toString());
                                    return 0;
                                });
    httpServer->start();
}



int main(){
    WebSrv::IOManager scheduler(2);
    WebSrv::http::HttpServer::ptr httpServer(new WebSrv::http::HttpServer(&scheduler,&scheduler,&scheduler,true));
    httpServer->setName("http server/1.0.0");
    scheduler.start();
    scheduler.schedule(std::bind(run,httpServer));
}