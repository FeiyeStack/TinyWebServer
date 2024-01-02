#include "http/httpsession.h"
#include "http/httpparser.h"
#include "log.h"
namespace WebSrv::http
{
    static Logger::ptr g_logger=SRV_LOGGER_NAME("system");

    HttpSession::HttpSession(Socket::ptr socket, bool owner)
        : SocketStream(socket, owner)
    {
    }

    HttpRequest::ptr HttpSession::recvRequest()
    {

        HttpRequestParser::ptr parser(new HttpRequestParser);
        uint64_t buffSize = HttpRequestParser::getHttpRequestBufferSize();
        std::shared_ptr<char> buffer(new char[buffSize], [](char *p)
                                     { delete[] p; });
        char *data = buffer.get();
        int offset = 0;
        // 获取请求头
        do
        {
            u_long outLen;
            int ret = read(data + offset, buffSize - offset);
            if (ret <= 0)
            {
                close();
                return nullptr;
            }
            outLen += offset;
            size_t nParse = parser->execute(data, outLen);
            if (nParse < 0)
            {
                // 解析错误
                close();
                return nullptr;
            }
            // nParse==0 报文不完整(不处理到缓存满或者超时退出)
            offset = outLen - nParse;

            // 解析完成
            if (nParse > 0)
            {
                break;
            }

            if (offset == (int)buffSize)
            {
                close();
                return nullptr;
            }

        } while (true);
        // 获取请求消息体
        int64_t length = parser->getContentLength();
        if (length > 0)
        {
            if (length > HttpRequestParser::getHttpRequestMaxBodySize())
            {
                close();
                return nullptr;
            }
            std::string body;
            body.resize(length);
            int len = 0;
            if (length >= offset)
            {
                memcpy(&body[0], data, offset);
            }
            else
            {
                memcpy(&body[0], data, length);
            }
            length -= offset;
            // 如果消息体没获取完整，继续读消息体
            if (length > 0)
            {
                if (readFixSize(&body[len], length) <= 0)
                {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
        return parser->getData();
    }

    int HttpSession::sendResponse(HttpResponse::ptr response)
    {
        std::string data = response->toString();
        return writeFixSize(data.c_str(), data.size());
    }

} // namespace WebSrv
