#include "iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "macro.h"
namespace WebSrv
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        // 初始化epoll和pipe管道
        _epollFd = epoll_create(5000);
        WebSrvAssert(_epollFd > 0);
        int ret = pipe(_tickleFds);
        WebSrvAssert(ret == 0);

        epoll_event event{};
        // 边缘触发
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = _tickleFds[0];
        // 非阻塞
        ret = fcntl(_tickleFds[0], F_SETFL, O_NONBLOCK);
        WebSrvAssert(ret == 0);

        ret = epoll_ctl(_epollFd, EPOLL_CTL_ADD, _tickleFds[0], &event);
        WebSrvAssert(ret == 0);
        contextResize(32);
        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(_epollFd);
        close(_tickleFds[0]);
        close(_tickleFds[1]);
        for (size_t i = 0; i < _fdContexts.size(); i++)
        {
            if (_fdContexts[i])
            {
                delete _fdContexts[i];
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fdCtx = nullptr;
        ReadMutex readlock(_rwMutex);
        if ((int)_fdContexts.size() > fd)
        {
            fdCtx = _fdContexts[fd];
            readlock.unlock();
        }
        else
        {
            readlock.unlock();
            WriteMutex writelock(_rwMutex);
            contextResize(fd * 1.5);
            fdCtx = _fdContexts[fd];
        }

        std::lock_guard lock(fdCtx->mutex);
        if (fdCtx->events & event)
        { // 重复添加事件
            SRV_LOG_ERROR(g_logger) << "assert fd=" << fd
                                    << " event=" << (EPOLL_EVENTS)event
                                    << " fd_ctx.event=" << (EPOLL_EVENTS)fdCtx->events;
            WebSrvAssert(!(fdCtx->events & event));
        }

        int op = fdCtx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; // 存在事件，修改注册，尚未注册事件，添加新的事件

        epoll_event epollEvent;
        epollEvent.events = EPOLLET | fdCtx->events | event;
        epollEvent.data.ptr = fdCtx;
        int ret = epoll_ctl(_epollFd, op, fd, &epollEvent);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollFd << ", "
                                    << op << ", " << fd << ", " << (EPOLL_EVENTS)epollEvent.events << "):"
                                    << ret << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                    << (EPOLL_EVENTS)fdCtx->events;
            return -1;
        }

        ++_pendingEventCount;

        // 添加执行协程或回调函数
        fdCtx->events = (Event)(fdCtx->events | event);
        FdContext::EventContext &eventCtx = fdCtx->getContext(event);
        eventCtx.scheduler = Scheduler::getThis();
        if (cb)
        {
            eventCtx.cb.swap(cb);
        }
        else
        {
            eventCtx.fiber = Fiber::getThis();
            WebSrvAssert2(eventCtx.fiber->getState() == Fiber::RUNNING, "state=" << eventCtx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        ReadMutex readlock(_rwMutex);
        if ((int)_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fdCtx = _fdContexts[fd];
        readlock.unlock();
        // 无事件
        std::lock_guard lock(fdCtx->mutex);
        if (!(fdCtx->events & event))
        {
            return false;
        }
        // 移除对应事件
        Event newEvents = (Event)(fdCtx->events & ~event);
        int op = newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event eventEvent;
        eventEvent.events = EPOLLET | newEvents;
        eventEvent.data.ptr = fdCtx;
        int ret = epoll_ctl(_epollFd, op, fd, &eventEvent);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollFd << ", "
                                    << op << ", " << fd << ", " << (EPOLL_EVENTS)eventEvent.events << "):"
                                    << ret << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                    << (EPOLL_EVENTS)fdCtx->events;
            return -1;
        }
        fdCtx->triggerEvent(event);
        --_pendingEventCount;
        return true;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        ReadMutex readlock(_rwMutex);
        if ((int)_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fdCtx = _fdContexts[fd];
        readlock.unlock();
        std::lock_guard lock(fdCtx->mutex);
        // 无事件
        if (!(fdCtx->events & event))
        {
            return false;
        }
        // 移除对应事件
        Event newEvents = (Event)(fdCtx->events & ~event);
        int op = newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event eventEvent;
        eventEvent.events = EPOLLET | newEvents;
        eventEvent.data.ptr = fdCtx;
        int ret = epoll_ctl(_epollFd, op, fd, &eventEvent);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollFd << ", "
                                    << op << ", " << fd << ", " << (EPOLL_EVENTS)eventEvent.events << "):"
                                    << ret << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                    << (EPOLL_EVENTS)fdCtx->events;
            return -1;
        }
        --_pendingEventCount;
        fdCtx->events = newEvents;
        FdContext::EventContext &eventCtx = fdCtx->getContext(event);
        fdCtx->resetEventContext(eventCtx);
        return true;
    }
    IOManager *IOManager::getThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::getThis());
    }
    void IOManager::stop()
    {
        closeRecurringTimers();
        Scheduler::stop();
    }
    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        SRV_LOG_DEBUG(g_logger) << __func__;
        int rt = write(_tickleFds[1], "T", 1);
        WebSrvAssert(rt == 1); // 一对一
    }
    bool IOManager::stopping()
    {
        uint64_t timeout;
        return stopping(timeout);
    }
    bool IOManager::stopping(uint64_t timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ll &&
               _pendingEventCount == 0 &&
               Scheduler::stopping();
    }
    void IOManager::idle()
    {
        SRV_LOG_DEBUG(g_logger) << __func__;
        const uint64_t MAX_EVENTS = 256;
        epoll_event *events = new epoll_event[MAX_EVENTS];
        std::shared_ptr<epoll_event> spEvents(events, [](epoll_event *ptr)
                                              { delete[] ptr; });
        while (true)
        {
            uint64_t nextTimeout;
            if (stopping(nextTimeout))
            {
                break;
            }
            int ret;
            int timeout;
            do
            {
                static const int MAX_TIMEOUT = 3000;
                if (nextTimeout != ~0ull)
                {
                    timeout = (int)nextTimeout > MAX_TIMEOUT ? MAX_TIMEOUT : nextTimeout;
                }
                else
                {
                    timeout = MAX_TIMEOUT;
                }
                ret = epoll_wait(_epollFd, events, MAX_EVENTS, timeout);
                if (ret < 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCallback(cbs);
            if (!cbs.empty())
            {
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // 处理io事件
            for (int i = 0; i < ret; i++)
            {
                epoll_event &event = events[i];
                if (event.data.fd == _tickleFds[0])
                {
                    uint8_t tmp[256];
                    // 此处为有超时任务产生的fd，读出后忽略
                    while (read(_tickleFds[0], tmp, sizeof(tmp)) > 0)
                        ;
                    continue;
                }

                FdContext *fdCtx = (FdContext *)event.data.ptr;
                std::lock_guard lock(fdCtx->mutex);
                // 发生错误将事件重新添加
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fdCtx->events;
                }

                int realEvents = NONE;
                // 读事件
                if (event.events & EPOLLIN)
                {
                    realEvents |= READ;
                }
                // 写事件
                if (event.events & EPOLLOUT)
                {
                    realEvents |= WRITE;
                }

                if ((fdCtx->events & realEvents) == NONE)
                {
                    continue;
                }

                // 将未触发的事件重启注册，否则就清空事件
                int leftEvents = (fdCtx->events & ~realEvents);
                int op = leftEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | leftEvents;

                int ret2 = epoll_ctl(_epollFd, op, fdCtx->fd, &event);
                if (ret2)
                {
                    SRV_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollFd << ", "
                                            << op << ", " << fdCtx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                            << ret2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }
                // 将事件添加到池中
                if (realEvents & READ)
                {
                    fdCtx->triggerEvent(READ);
                    --_pendingEventCount;
                }

                if (realEvents & WRITE)
                {
                    fdCtx->triggerEvent(WRITE);
                    --_pendingEventCount;
                }
            }
            Fiber::yieldToSuspend();
        }
    }
    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
    bool IOManager::cancelAll(int fd)
    {
        ReadMutex readlock(_rwMutex);
        if ((int)_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fdCtx = _fdContexts[fd];
        readlock.unlock();
        // 无事件
        std::lock_guard lock(fdCtx->mutex);

        if (!(fdCtx->events))
        {
            return false;
        }
        // 移除所有事件
        int op = EPOLL_CTL_DEL;
        epoll_event eventEvent;
        eventEvent.events = 0;
        eventEvent.data.ptr = fdCtx;
        int ret = epoll_ctl(_epollFd, op, fd, &eventEvent);
        if (ret)
        {
            SRV_LOG_ERROR(g_logger) << "epoll_ctl(" << _epollFd << ", "
                                    << op << ", " << fd << ", " << (EPOLL_EVENTS)eventEvent.events << "):"
                                    << ret << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                    << (EPOLL_EVENTS)fdCtx->events;
            return -1;
        }
        if (fdCtx->events & READ)
        {
            fdCtx->triggerEvent(READ);
            --_pendingEventCount;
        }
        if (fdCtx->events & WRITE)
        {
            fdCtx->triggerEvent(WRITE);
            --_pendingEventCount;
        }
        WebSrvAssert(fdCtx->events == 0);
        return true;
    }
    void IOManager::contextResize(size_t size)
    {
        size_t oldsize = _fdContexts.size();
        if (oldsize == size)
        {
            return;
        }
        else if (oldsize < size)
        {
            _fdContexts.resize(size);
            for (size_t i = 0; i < _fdContexts.size(); i++)
            {
                if (!_fdContexts[i])
                {
                    _fdContexts[i] = new FdContext;
                    _fdContexts[i]->fd = i;
                }
            }
        }
        else
        {
            // 一般只增不减
            for (size_t i = oldsize - 1; i >= size; i--)
            {
                if (_fdContexts[i])
                {
                    delete _fdContexts[i];
                    _fdContexts.pop_back();
                }
            }
        }
    }
    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            break;
        }
        throw std::invalid_argument("getContext invalid event");
    }
    void IOManager::FdContext::resetEventContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(Event event)
    {
        WebSrvAssert(events & event);
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;
    }
} // namespace WebSrv
