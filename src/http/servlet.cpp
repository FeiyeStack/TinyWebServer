#include "http/servlet.h"
#include "http/http.h"
#include <fnmatch.h>
namespace WebSrv::http
{
    FunctionServlet::FunctionServlet(callback cb)
        : Servlet("FunctionServlet"), _cb(cb)
    {
    }

    int32_t FunctionServlet::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session)
    {
        return _cb(request, response, session);
    }

    ServletDispatch::ServletDispatch()
        : Servlet("ServletDispatch"), _default(new NotFoundServlet("server/1.0.0"))
    {
    }

    int32_t ServletDispatch::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session)
    {
        auto servlet = getMatchedServlet(request->getPath());
        if (servlet)
        {
            servlet->handle(request, response, session);
        }
        return 0;
    }

    void ServletDispatch::addServlet(const std::string &uri, Servlet::ptr servlet)
    {
        WriteMutex lock(_mutex);
        _datas[uri] = servlet;
    }

    void ServletDispatch::addServlet(const std::string &uri, FunctionServlet::callback cb)
    {
        WriteMutex lock(_mutex);
        _datas[uri] = Servlet::ptr(new FunctionServlet(cb));
    }

    void ServletDispatch::addGlobServlet(const std::string &uri, Servlet::ptr slt)
    {
        WriteMutex lock(_mutex);
        for (auto it = _globs.begin(); it !=_globs.end(); ++it)
        {
            if (it->first == uri)
            {
                _globs.erase(it);
                break;
            }
        }
        _globs.emplace(uri, slt);
    }

    void ServletDispatch::addGlobServlet(const std::string &uri, FunctionServlet::callback cb)
    {
        addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
    }

    void ServletDispatch::delServlet(const std::string &uri)
    {
        WriteMutex lock(_mutex);
        _datas.erase(uri);
    }

    void ServletDispatch::delGlobServlet(const std::string &uri)
    {
        WriteMutex lock(_mutex);
        for (auto it = _globs.begin(); it !=_globs.end(); ++it)
        {
            if (it->first == uri)
            {
                _globs.erase(it);
                break;
            }
        }
    }

    Servlet::ptr ServletDispatch::getServlet(const std::string &uri)
    {
        ReadMutex lock(_mutex);
        auto it = _datas.find(uri);
        return it == _datas.end() ? nullptr : it->second;
    }

    Servlet::ptr ServletDispatch::getGlobServlet(const std::string &uri)
    {
        WriteMutex lock(_mutex);
        for (auto it = _globs.begin(); it != _globs.end(); ++it)
        {
            if (it->first == uri)
            {
                return it->second;
            }
        }
        return nullptr;
    }

    Servlet::ptr ServletDispatch::getMatchedServlet(const std::string &uri)
    {
        WriteMutex lock(_mutex);
        auto mit = _datas.find(uri);
        if (mit != _datas.end())
        {
            return mit->second;
        }
        for (auto it = _globs.begin();
             it != _globs.end(); ++it)
        {
            // 通配符匹配
            if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
        }
        return _default;
    }

    NotFoundServlet::NotFoundServlet(const std::string &name)
        : Servlet("NotFoundServlet"),
          _name(name)
    {
        _content = "<html><head><title>404 Not Found"
                   "</title></head><body><center><h1>404 Not Found</h1></center>"
                   "<hr><center>" +
                   name + "</center></body></html>";
    }

    int32_t NotFoundServlet::handle(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session)
    {
        response->setStatus(http::HttpStatus::HTTP_STATUS_NOT_FOUND);
        response->setHeader("Server", _name);
        response->setHeader("Content-Type", "text/html");
        response->setBody(_content);
        return 0;
    }

} // namespace WebSrv
