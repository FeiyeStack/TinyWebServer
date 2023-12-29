#include <iostream>
#include "TinyWebServer/http/http.h"

void test_request(){
    WebSrv::http::HttpRequest::ptr request(new WebSrv::http::HttpRequest);
    request->setPath("/index.html");
    request->setHeader("Host","www.example.com");
    request->setHeader("User-Agent","Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:98.0) Gecko/20100101 Firefox/98.0");
    request->setHeader("Accept-Language","en-US,en;q=0.5");
    request->setHeader("Connection","keep-alive");
    request->setParam("key1","value1");
    request->setParam("key2","value2");
    request->setCookie("PHPSESSID","298zf09hf012fh2");
    request->setCookie("csrftoken","u32t4o3tb3gg43");
    request->setHttpMethod(WebSrv::http::HttpMethod::HTTP_GET);
    request->dump(std::cout)<<std::endl;
    request->setHttpMethod(WebSrv::http::HttpMethod::HTTP_POST);
    request->setHeader("Content-Type","application/x-www-form-urlencoded");
    request->dump(std::cout)<<std::endl;
}

void test_response(){
    WebSrv::http::HttpResponse::ptr response(new WebSrv::http::HttpResponse);
    response->setStatus(WebSrv::http::HttpStatus::HTTP_STATUS_OK);
    response->setHeader("Content-Type","text/html");
    response->setBody("<!DOCTYPE html>\r\n"
                      "<html>\r\n"
                      "<head>\r\n"
                      "    <title>Sample Page</title>\r\n"
                      "</head>\r\n"
                      "<body>\r\n"
                      "    <h1>Hello, World!</h1>\r\n"
                      "</body>\r\n"
                      "</html>\r\n");
    response->setCookie("sessionId","38afes7a8");
    WebSrv::http::HttpResponse::CookieOptions options;
    options.expiration=1702887566ull;
    options.domain="example.com";
    options.sameSite=WebSrv::http::HttpResponse::CookieOptions::Lax;
    response->setCookie("id","a3fWa",options);
    response->dump(std::cout)<<std::endl;
}

int main(){
    test_request();
    test_response();
}