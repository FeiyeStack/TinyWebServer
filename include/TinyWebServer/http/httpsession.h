#pragma once

#include <memory>
#include "socket.h"
#include "http.h"
#include "streams/socketstream.h"
#include "httpsession.h"
#include "httpparser.h"
namespace WebSrv::http
{
    class HttpSession : public SocketStream
    {
    public:
        using ptr = std::shared_ptr<HttpSession>;

        HttpSession(Socket::ptr socket, bool owner = true);
        /**
         * @brief 解析请求头
         *
         * @param request 返回请求，null为解析失败
         */
        HttpRequest::ptr recvRequest();
        /**
         * @brief 发送HTTP响应
         *
         * @param response
         * @param ret 成功返回发送长度，失败返回-1
         */
        int sendResponse(HttpResponse::ptr response);

    };
} // namespace WebSrv
