#include "hook.h"

#include <dlfcn.h>
#include "log.h"
#include "configurator.h"
#include "iomanager.h"
#include "fdmanager.h"
#include <functional>
#include <fcntl.h>
#include <cstdarg>
#include <sys/ioctl.h>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("system");

namespace WebSrv
{
    static thread_local bool t_hookEnable = false;

    static ConfigVar<int>::ptr g_tcpConnectTimeout =
        Configurator::lookup<int>("tcp.connect.timeout", 5000, "tcp connect timeout");

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

    void hook_init()
    {
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX)
#undef XX
    }

    static uint64_t s_connectTimeout = -1;

    struct _HookInit
    {
        _HookInit()
        {
            hook_init();
            s_connectTimeout = g_tcpConnectTimeout->getValue();
            g_tcpConnectTimeout->addChangeValueListener([](const int &oldValue, const int &newValue)
                                                        { s_connectTimeout = newValue; });
        }
    };

    static _HookInit s_hookInit;

    bool isHookEnable()
    {
        return t_hookEnable;
    }

    void setHookEnable(bool flag)
    {
        t_hookEnable = flag;
    }

    struct TimerInfo
    {
        int cannel=0;
    };

    template <typename OriginFun, typename... Args>
    static ssize_t doIO(int fd, OriginFun fun, const char *hook_fun, uint32_t event, int so_timeout, Args &&...args)
    {
        // 未设置，或阻塞时，默认的
        if (!t_hookEnable)
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        FdCtx::ptr ctx = FdManager::getFdManger()->get(fd);

        // 不在管理范畴
        if (!ctx)
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        if (ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket() || ctx->getUserNonblock())
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        uint64_t to = ctx->getTimeout(so_timeout);
        std::shared_ptr<TimerInfo> timerInfo(new TimerInfo);
        do
        {
            ssize_t n = fun(fd, std::forward<Args>(args)...);
            // 被中断
            while (n == -1 && errno == EINTR)
            {
                n = fun(fd, std::forward<Args>(args)...);
            }
            // 非阻塞，投递事件
            if (n == -1 && errno == EAGAIN)
            {
                IOManager *ioManager = IOManager::getThis();
                Timer::ptr timer;
                std::weak_ptr<TimerInfo> weakInfo(timerInfo);
                //设置超时
                if (to != -1)
                {
                    timer = ioManager->addConditionTimer(
                        to,
                        [weakInfo, fd, ioManager, event]()
                        {
                            auto p = weakInfo.lock();
                            if (!p || p->cannel)
                            {
                                return;
                            }
                            p->cannel = ETIMEDOUT;
                            ioManager->cancelEvent(fd, (IOManager::Event)(event));
                        },
                        [ctx]()
                        {
                            return !ctx->isClose();
                        });
                }
                int ret = ioManager->addEvent(fd, (IOManager::Event)(event));
                if (ret == -1)
                {
                    if (timer)
                    {
                        timer->cannel();
                    }
                    return -1;
                }
                else
                {
                    Fiber::yieldToSuspend();

                    if (timer)
                    {
                        timer->cannel();
                    }
                    if (timerInfo->cannel)
                    {
                        errno = timerInfo->cannel;
                        return -1;
                    }
                }
            }
            else
            {
                return n;
            }
        } while (true);
    }
    extern "C"
    {
#define XX(name) name##_fun name##_f = nullptr;
        HOOK_FUN(XX)
#undef XX

        unsigned int sleep(unsigned int seconds)
        {
            if (!t_hookEnable)
            {
                return sleep_f(seconds);
            }

            Fiber::ptr fiber = Fiber::getThis();
            IOManager *ioManager = IOManager::getThis();
            ioManager->addTimer(seconds * 1000, [ioManager, fiber]()
                                { ioManager->schedule(fiber, std::thread::id()); });
            Fiber::yieldToSuspend();
            return 0;
        }

        int usleep(useconds_t usec)
        {
            if (!t_hookEnable)
            {
                return usleep(usec);
            }
            Fiber::ptr fiber = Fiber::getThis();
            IOManager *ioManager = IOManager::getThis();
            ioManager->addTimer(usec / 1000, [ioManager, fiber]()
                                { ioManager->schedule(fiber, std::thread::id()); });
            Fiber::yieldToSuspend();
            return 0;
        }

        int socket(int domain, int type, int protocol)
        {
            if (!t_hookEnable)
            {
                return socket_f(domain, type, protocol);
            }
            int fd = socket_f(domain, type, protocol);
            if (fd == -1)
            {
                return fd;
            }
            FdManager::getFdManger()->get(fd, true);
            return fd;
        }

        int connectWithTimeout(int fd, const sockaddr *addr, socklen_t addrlen, int64_t timeout_ms)
        {
            if (!t_hookEnable)
            {
                return connect_f(fd, addr, addrlen);
            }
            FdCtx::ptr ctx = FdManager::getFdManger()->get(fd);
            if (!ctx || ctx->isClose())
            {
                errno = EBADF;
                return -1;
            }

            if (!ctx->isSocket())
            {
                return connect_f(fd, addr, addrlen);
            }

            if (ctx->getUserNonblock())
            {
                return connect_f(fd, addr, addrlen);
            }

            int n = connect_f(fd, addr, addrlen);
            if (n == 0)
            {
                return 0;
            }
            else if (n != -1 || errno != EINPROGRESS)
            {
                return n;
            }
            IOManager *ioManager = IOManager::getThis();
            Timer::ptr timer;
            std::shared_ptr<TimerInfo> timerInfo(new TimerInfo);
            std::weak_ptr<TimerInfo> weakInfo(timerInfo);

            if (timeout_ms != -1)
            {
                timer = ioManager->addTimer(
                    timeout_ms,
                    [weakInfo, fd, ioManager]()
                    {
                        auto p = weakInfo.lock();
                        if (!p || p->cannel)
                        {
                            return;
                        }
                        p->cannel = ETIMEDOUT;
                        ioManager->cancelEvent(fd, IOManager::Event::WRITE);
                    });
            }

            int ret = ioManager->addEvent(fd, IOManager::Event::WRITE);
            if (ret == -1)
            {
                if (timer)
                {
                    timer->cannel();
                }
                SRV_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
            }
            else
            {
                Fiber::yieldToSuspend();
                if (timer)
                {
                    timer->cannel();
                }
                // 超时退出
                if (timerInfo->cannel)
                {
                    errno = timerInfo->cannel;
                    return -1;
                }
            }

            int error = 0;
            socklen_t len = sizeof(int);
            // 获取socket错误类型
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
            {
                return -1;
            }

            if (!error)
            {
                return 0;
            }
            else
            {
                errno = error;
                return -1;
            }
        }

        int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        {
            return connectWithTimeout(sockfd, addr, addrlen, s_connectTimeout);
        }

        int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
        {
            int fd = doIO(s, accept_f, "accept", IOManager::READ, SO_RCVTIMEO, addr, addrlen);
            if (fd >= 0)
            {
                FdManager::getFdManger()->get(fd, true);
            }
            return fd;
        }

        ssize_t read(int fd, void *buf, size_t count)
        {
            return doIO(fd, read_f, "read", IOManager::READ, SO_RCVTIMEO, buf, count);
        }

        ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
        {
            return doIO(fd, readv_f, "readv", IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
        }

        ssize_t recv(int sockfd, void *buf, size_t len, int flags)
        {
            return doIO(sockfd, recv_f, "recv", IOManager::READ, SO_RCVTIMEO, buf, len, flags);
        }

        ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
        {
            return doIO(sockfd, recvfrom_f, "recvfrom", IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
        }

        ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
        {
            return doIO(sockfd, recvmsg_f, "recvmsg", IOManager::READ, SO_RCVTIMEO, msg, flags);
        }

        ssize_t write(int fd, const void *buf, size_t count)
        {
            return doIO(fd, write_f, "write", IOManager::WRITE, SO_SNDTIMEO, buf, count);
        }

        ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
        {
            return doIO(fd, writev_f, "writev", IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
        }

        ssize_t send(int s, const void *msg, size_t len, int flags)
        {
            return doIO(s, send_f, "send", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
        }

        ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
        {
            return doIO(s, sendto_f, "sendto", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
        }

        ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
        {
            return doIO(s, sendmsg_f, "sendmsg", IOManager::WRITE, SO_SNDTIMEO, msg, flags);
        }
        int close(int fd)
        {
            if (!t_hookEnable)
            {
                return close_f(fd);
            }

            FdCtx::ptr ctx = FdManager::getFdManger()->get(fd);
            if (ctx)
            {
                auto ioManager = IOManager::getThis();
                if (ioManager)
                {
                    ioManager->cancelAll(fd);
                }
                FdManager::getFdManger()->del(fd);
            }
            return close_f(fd);
        }

        int fcntl(int fd, int cmd, ...)
        {
            va_list va;
            va_start(va, cmd);
            switch (cmd)
            {
            case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                FdCtx::ptr ctx = FdManager::getFdManger()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
            case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                FdCtx::ptr ctx = FdManager::getFdManger()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return arg;
                }
                if (ctx->getUserNonblock())
                {
                    return arg | O_NONBLOCK;
                }
                else
                {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
#ifdef F_SETPIPE_SZ
            case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
#ifdef F_GETPIPE_SZ
            case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            case F_GETOWN_EX:
            case F_SETOWN_EX:
            {
                struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
            }
        }

        int ioctl(int d, unsigned long int request, ...)
        {
            va_list va;
            va_start(va, request);
            void *arg = va_arg(va, void *);
            va_end(va);

            if (FIONBIO == request)
            {
                bool user_nonblock = !!*(int *)arg;
                FdCtx::ptr ctx = FdManager::getFdManger()->get(d);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return ioctl_f(d, request, arg);
                }
                ctx->setUserNonblock(user_nonblock);
            }
            return ioctl_f(d, request, arg);
        }
        int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
        {
            return getsockopt_f(sockfd, level, optname, optval, optlen);
        }

        int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
        {
            if (!t_hookEnable)
            {
                return setsockopt_f(sockfd, level, optname, optval, optlen);
            }
            if (level == SOL_SOCKET)
            {
                if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
                {
                    FdCtx::ptr ctx = FdManager::getFdManger()->get(sockfd);
                    if (ctx)
                    {
                        const timeval *v = (const timeval *)optval;
                        ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                    }
                }
            }
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
    }

} // namespace WebSrv
