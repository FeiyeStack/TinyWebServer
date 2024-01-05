#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "http.h"
#include "httpsession.h"
#include "mutex.h"
#include <functional>
namespace WebSrv::http
{
    class Servlet
    {
    public:
        using ptr = std::shared_ptr<Servlet>;

        Servlet(const std::string &name) : _name(name){};

        virtual ~Servlet() = default;

        virtual int32_t handle(http::HttpRequest::ptr request,
                               http::HttpResponse::ptr response,
                               http::HttpSession::ptr session) = 0;

        const std::string &getName() const { return _name; }

    protected:
        std::string _name;
    };

    class FunctionServlet : public Servlet
    {
    public:
        using ptr = std::shared_ptr<FunctionServlet>;
        using callback = std::function<int32_t(http::HttpRequest::ptr request,
                                               http::HttpResponse::ptr response,
                                               http::HttpSession::ptr session)>;

        FunctionServlet(callback cb);
        int32_t handle(http::HttpRequest::ptr request,
                       http::HttpResponse::ptr response,
                       http::HttpSession::ptr session) override;

    private:
        callback _cb;
    };
    /**
     * @brief Servlet分发器
     *
     */
    class ServletDispatch : public Servlet
    {
    public:
        using ptr = std::shared_ptr<ServletDispatch>;

        ServletDispatch();
        int32_t handle(http::HttpRequest::ptr request,
                       http::HttpResponse::ptr response,
                       http::HttpSession::ptr session) override;
        /**
         * @brief 添加Servlet
         *
         * @param uri
         * @param servlet
         */
        void addServlet(const std::string &uri, Servlet::ptr servlet);
        /**
         * @brief 添加Servlet
         *
         * @param uri
         * @param cb
         */
        void addServlet(const std::string &uri, FunctionServlet::callback cb);
        /**
         * @brief 添加模糊匹配Servlet
         *
         * @param uri
         * @param servlet
         */
        void addGlobServlet(const std::string &uri, Servlet::ptr slt);
        /**
         * @brief 添加模糊匹配Servlet
         *
         * @param uri
         * @param cb
         */
        void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

        /**
         * @brief 删除Servlet
         *
         * @param uri
         */
        void delServlet(const std::string &uri);
        /**
         * @brief 删除模糊匹配Servlet
         *
         * @param uri
         */
        void delGlobServlet(const std::string &uri);

        /**
         * @brief 获取Servlet
         *
         * @param uri
         * @return Servlet::ptr
         */
        Servlet::ptr getServlet(const std::string &uri);

        /**
         * @brief 获取模糊匹配Servlet
         *
         * @param uri
         * @return Servlet::ptr
         */
        Servlet::ptr getGlobServlet(const std::string &uri);
        /**
         * @brief 默认servlet
         *
         * @return Servlet::ptr
         */
        Servlet::ptr getDefault() const { return _default; }

        /**
         * @brief 使用uri获取Servlet  优先精准，其次模糊匹配
         *
         * @param uri
         * @return Servlet::ptr
         */
        Servlet::ptr getMatchedServlet(const std::string &uri);

    private:
        RWMutex _mutex;
        // 精准匹配
        std::unordered_map<std::string, Servlet::ptr> _datas;
        // 模糊匹配
        std::unordered_map<std::string, Servlet::ptr> _globs;
        // 默认Servlet
        Servlet::ptr _default;
    };
    /**
     * @brief servlet不存在时（默认返回404）
     *
     */
    class NotFoundServlet : public Servlet
    {
    public:
        using ptr = std::shared_ptr<NotFoundServlet>;
        NotFoundServlet(const std::string &name);
        int32_t handle(http::HttpRequest::ptr request,
                       http::HttpResponse::ptr response,
                       http::HttpSession::ptr session) override;

    private:
        std::string _name;
        std::string _content;
    };
} // namespace WebSrv
