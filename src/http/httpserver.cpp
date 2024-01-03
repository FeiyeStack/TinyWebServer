#include "http/httpserver.h"
#include "http/httpsession.h"
#include "log.h"

namespace WebSrv::http
{
    static Logger::ptr g_logger=SRV_LOGGER_NAME("system");
    HttpServer::HttpServer(IOManager *worker,
                           IOManager *ioWorker,
                           IOManager *acceptWorker,
                           bool keepalive)
        : TcpServer(worker, ioWorker, acceptWorker),
          _keepalive(keepalive)
    {
        _type = "http";
        _dispatch.reset(new ServletDispatch);
    }

    void HttpServer::setName(const std::string &name)
    {
        TcpServer::setName(name);
    }
    void HttpServer::handleClient(Socket::ptr client)
    {
        SRV_LOG_DEBUG(g_logger) << "handleClient" << client->toString();
        HttpSession::ptr session(new HttpSession(client));
        bool isClose = false; // 判定长短连接
        bool first = false;   // 判定是不是第一次判定
        do
        {
            auto request = session->recvRequest();
            
            if (!request)
            {
                SRV_LOG_DEBUG(g_logger) << "recv http request fail, errno=" << errno << " errstr=" << strerror(errno);
                break;
            }
            SRV_LOG_DEBUG(g_logger)<<(int)*client<<"recv request:\n"<<request->toString();
            //  判断是否是长连接
            if (!first)
            {
                std::string conn = request->getHeader("connection");
                if (!conn.empty() && _keepalive)
                {
                    if (strcasecmp(conn.c_str(), "keep-alive") == 0)
                    {
                        isClose = false;
                    }
                    else
                    {
                        isClose = true;
                    }
                }
                else if (!_keepalive)
                {
                    isClose = false;
                }
                else
                {
                    if (request->getVersion() == 0x10)
                    {
                        isClose = true;
                    }
                    else
                    {
                        isClose = false;
                    }
                }
                first = true;
            }

            HttpResponse::ptr response(new HttpResponse(request->getVersion()));
            response->setHeader("Server", getName());
            if (isClose)
            {
                response->setHeader("connection", "close");
            }
            else
            {
                response->setHeader("connection", "keep-alive");
            }

            response->setHeader("Server", getName());
            _dispatch->handle(request, response, session);


            int ret=session->sendResponse(response);

            if(ret<=0){
                SRV_LOG_DEBUG(g_logger) <<(int)*client<< "sendResponse http request fail, errno=" << errno << " errstr=" << strerror(errno);
                break;
            }

            if (isClose)
            {
                break;
            }

        } while (true);
        SRV_LOG_DEBUG(g_logger) <<(int)*client<<"handleClient end";
        client->close();
    }
} // namespace WebSrv
