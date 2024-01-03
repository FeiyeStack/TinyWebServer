#include "fdmanager.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "hook.h"
namespace WebSrv
{
    FdCtx::FdCtx(int fd)
        : _fd(fd)
    {
        init();
    }

    void FdCtx::setTimeout(int so_timeout, uint64_t timeout)
    {
        if (so_timeout == SO_RCVTIMEO)
        {
            _recvTimeout = timeout;
        }
        else
        {
            _sendTimeout = timeout;
        }
    }

    uint64_t FdCtx::getTimeout(int so_timeout)
    {
        if (so_timeout == SO_RCVTIMEO)
        {
            return _recvTimeout;
        }
        else
        {
            return _sendTimeout;
        }
    }

    void FdCtx::init()
    {
        // 判断句柄是否有效
        struct stat fdStat;
        if (fstat(_fd, &fdStat) != -1)
        {
            _isInit = true;
            _isSocket = S_ISSOCK(fdStat.st_mode);
        }

        if (_isSocket)
        {
            int flags = fcntl_f(_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK))
            {
                // 设置非阻塞
                fcntl_f(_fd, F_SETFL, flags | O_NONBLOCK);
            }
            _sysNonblock = true;
        }
    }

    FdManager::FdManager()
    {
        _fds.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd, bool autoCreate)
    {
        if (fd == -1)
        {
            return nullptr;
        }
        ReadMutex readlock(_mutex);
        if ((int)_fds.size() <= fd)
        {
            if (autoCreate == false)
            {
                return nullptr;
            }
        }
        else
        {
            
            if (_fds[fd] || !autoCreate)//如果不自动创建不存在则还是返null
            {
                return _fds[fd];
            }
        }
        readlock.unlock();
        WriteMutex writelock(_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if (fd >= (int)_fds.size())
        {
            _fds.resize(fd * 1.5);
        }
        _fds[fd] = ctx;
        return ctx;
    }

    void FdManager::del(int fd)
    {
        WriteMutex writelock(_mutex);
        if(fd>=(int)_fds.size()){
            return;
        }
        _fds[fd].reset();
    }

    FdManager *FdManager::getFdManger()
    {
        static FdManager fdManager;
        return &fdManager;
    }

} // namespace WebSrv
