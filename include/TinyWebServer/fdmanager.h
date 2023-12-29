#pragma once

#include <vector>
#include <memory>
#include "mutex.h"
namespace WebSrv
{
    /**
     * @brief 管理文件句柄上下文类
     *
     */
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        using ptr = std::shared_ptr<FdCtx>;

        FdCtx(int fd);

        ~FdCtx()=default;
        /**
         * @brief 是否初始化
         *
         * @return true
         * @return false
         */
        bool isInit() const { return _isInit; };
        /**
         * @brief 是否是socket
         *
         * @return true
         * @return false
         */
        bool isSocket() { return _isSocket; }
        /**
         * @brief 是否是关闭的
         *
         * @return true
         * @return false
         */
        bool isClose() { return _isClosed; }
        /**
         * @brief 是否是系统的阻塞
         *
         * @return true
         * @return false
         */
        bool getSysNonblock() { return _sysNonblock; }
        /**
         * @brief 是否是用户设置的非阻塞
         *
         * @return true
         * @return false
         */
        bool getUserNonblock() { return _userNonblock; }
        /**
         * @brief 设置系统的非阻塞
         *
         * @param sysNonblock
         */
        void setSysNonblock(bool sysNonblock) { _sysNonblock = sysNonblock; }
        /**
         * @brief 设置用户的非阻塞
         *
         * @return true
         * @return false
         */
        void setUserNonblock(bool userNonblock) { _userNonblock = userNonblock; }
        /**
         * @brief 设置读写超时
         *
         * @param so_timeout SO_RCVTIMEO/SO_SNDTIMEO
         * @param timeout 超时时间ms
         */
        void setTimeout(int so_timeout, uint64_t timeout);
        /**
         * @brief 获取读写超时
         *
         * @param so_timeout SO_RCVTIMEO/SO_SNDTIMEO
         */
        uint64_t getTimeout(int so_timeout);
    private:
        void init();
    private:
        bool _isInit = false;
        bool _isSocket = false;
        bool _sysNonblock = false;
        bool _userNonblock = false;
        bool _isClosed = false;
        // 文件句柄
        int _fd;
        // 读超时
        int64_t _recvTimeout=-1;
        // 写超时
        int64_t _sendTimeout=-1;
    };
    /**
     * @brief 句柄类管理
     * 
     */
    class FdManager{
    public:
        /**
         * @brief 获取/创建文件句柄
         * 
         * @param fd 
         * @param autoCreate 是否自动创建
         * @return FdCtx::ptr 
         */
        FdCtx::ptr get(int fd,bool autoCreate=false);
        /**
         * @brief 删除文件句柄
         * 
         * @param fd 
         */
        void del(int fd);
        /**
         * @brief 单例
         * 
         * @return FdManager* 
         */
        static FdManager* getFdManger();
    private:
        FdManager();
    private:
        RWMutex _mutex;
        std::vector<FdCtx::ptr> _fds;
    };
} // namespace WebSrv
