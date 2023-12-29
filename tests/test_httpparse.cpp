#include <iostream>
#include <cstring>
#include "TinyWebServer/http/httpparser.h"
int main(){
    WebSrv::http::HttpRequestParser parse;
    char req[] = "GET /path/to/resource HTTP/1.1\r\n"
                 "Host: www.example.com\r\n"
                 "User-Agent: Mozilla/5.0\r\n"
                 "Accept: text/html\r\n\r\nxxx";
    int len = parse.execute(req, strlen(req));
    std::cout << (req + len) << std::endl;
    parse.getData()->dump(std::cout);
    std::cout << std::endl;
    WebSrv::http::HttpResponseParser parse2;
    char res[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n";
    len = strlen(res);
    std::cout << len << std::endl
              << parse2.execute(res, len) << std::endl;
    
    parse2.getData()->dump(std::cout);
    std::cout << std::endl;
}