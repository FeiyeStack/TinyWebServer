#include <iostream>
#include "TinyWebServer/http/httpserver.h"
#include "log.h"

std::string filepath="/home/ubuntu/work/TinyWebServer/examples/http";


void run(WebSrv::http::HttpServer::ptr httpServer)
{
    auto addr = WebSrv::Address::lookupAny("0.0.0.0:8080");
    while (!httpServer->listen(addr))
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    auto servletDispatch= httpServer->getServletDispatch();
    servletDispatch->addGlobServlet("/*",
                                [&](WebSrv::http::HttpRequest::ptr request,
                                   WebSrv::http::HttpResponse::ptr response,
                                   WebSrv::http::HttpSession::ptr session)
                                {
                                    auto s=request->getPath();
                                    if(s=="/"){
                                        std::string file=filepath+"\\index.html";
                                        if(std::filesystem::exists(file)){
                                            std::ifstream fileStream(file, std::ios::binary);
                                            if (fileStream.is_open())
                                            {
                                                std::ostringstream oss;
                                                oss << fileStream.rdbuf();
                                                response->setStatus(WebSrv::http::HttpStatus::HTTP_STATUS_OK);
                                                response->setHeader("Server", httpServer->getName());
                                                response->setHeader("Content-Type","text/html");
                                                response->setBody(oss.str());
                                                return 1;
                                            }
                                        }
                                    }else{
                                        std::string file=filepath+s;
                                        if(std::filesystem::exists(file)){
                                            std::filesystem::path path(file);
                                            if (path.extension().string() == ".html")
                                            {
                                                response->setHeader("Content-Type","text/html");
                                            }
                                            else if (path.extension().string() == ".css")
                                            {
                                                response->setHeader("Content-Type","text/css");
                                            }
                                            else if (path.extension().string() == ".js")
                                            {
                                                response->setHeader("Content-Type","application/javascript");
                                            }
                                            else if (path.extension().string() == ".png")
                                            {
                                                response->setHeader("Content-Type","image/png");
                                            }
                                            else if (path.extension().string() == ".jpg" || path.extension().string() == ".jpeg")
                                            {
                                                response->setHeader("Content-Type","image/jpeg");
                                            }
                                            else if (path.extension().string() == ".mp4")
                                            {
                                                response->setHeader("Content-Type","video/mp4");
                                            }
                                            else
                                            {
                                                response->setHeader("Content-Type","application/octet-stream");
                                            }
                                            std::ifstream fileStream(file, std::ios::binary);
                                            if (fileStream.is_open())
                                            {
                                                std::ostringstream oss;
                                                oss << fileStream.rdbuf();
                                                response->setStatus(WebSrv::http::HttpStatus::HTTP_STATUS_OK);
                                                response->setHeader("Server", httpServer->getName());
                                                response->setBody(oss.str());
                                                return 1;
                                            }
                                        }
                                    }
                                    response->setStatus(WebSrv::http::HttpStatus::HTTP_STATUS_NOT_FOUND);
                                            response->setHeader("Server", httpServer->getName());
                                            response->setHeader("Content-Type", "text/html");
                                            response->setBody("<html><head><title>404 Not Found"
                                                              "</title></head><body><center><h1>404 Not Found</h1></center>"
                                                              "<hr><center>" +
                                                              httpServer->getName() + "</center></body></html>");
                                    return 0;
                                });
    httpServer->start();                                
}



int main(){
    WebSrv::IOManager scheduler(2);
    WebSrv::http::HttpServer::ptr httpServer(new WebSrv::http::HttpServer(&scheduler, &scheduler, &scheduler, true));
    httpServer->setName("http server/1.0.0");
    scheduler.schedule(std::bind(run,httpServer));

}