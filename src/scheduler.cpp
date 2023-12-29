#include "scheduler.h"
#include "log.h"
#include "hook.h"
namespace WebSrv
{

    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");
    static thread_local Scheduler *t_scheduler = nullptr;
    static thread_local Fiber *t_schedulerFiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
        : _name(name), _threadNum(threads)
    {
        if (threads <= 0)
        {
            threads = 1;
        }
        // 当前线程调度，切换到其他线程就不会执行
        if (use_caller)
        {
            // 初始化，主协程
            Fiber::getThis();
            --threads;
            t_scheduler = this;
            _rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0,true));

            t_schedulerFiber = _rootFiber.get();
            _rootThreadid = std::this_thread::get_id();
            _threadIds.emplace_back(_rootThreadid);
        }
    }

    Scheduler::~Scheduler()
    {
        if (getThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::getThis()
    {
        return t_scheduler;
    }

    Fiber *WebSrv::Scheduler::getMainFiber()
    {
        return t_schedulerFiber;
    }

    void Scheduler::start()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_stopping)
        {
            return;
        }
        _stopping = false;
        _threads.resize(_threadNum);
        for (size_t i = 0; i < _threadNum; i++)
        {
            _threads[i] = std::thread(std::bind(&Scheduler::run, this));
            _threadIds.emplace_back(_threads[i].get_id());
        }
    }

    void Scheduler::stop()
    {
        if (_rootFiber &&
            _threadNum == 0 &&
            (_rootFiber->getState() == Fiber::DONE ||
             _rootFiber->getState() == Fiber::INIT))
        {
            _stopping = true;
            // 可能已经关闭
            if (stopping())
            {
                return;
            }
        }

        _stopping = true;

        // 发出唤醒任务
        for (size_t i = 0; i < _threadNum; ++i)
        {
            tickle();
        }

        if (_rootFiber)
        {
            _rootFiber->call();
        }

        std::vector<std::thread> threads;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            threads.swap(_threads);
        }
        
        for (auto &thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void Scheduler::switchTo(std::thread::id threadid)
    {
        if (Scheduler::getThis() == this)
        {
            if (threadid == std::thread::id() || threadid == std::this_thread::get_id())
            {
                return;
            }
        }
        schedule(Fiber::getThis(), threadid);
        Fiber::yieldToSuspend();
    }

    void Scheduler::tickle()
    {
        SRV_LOG_INFO(g_logger) << __func__;
    }

    void Scheduler::run()
    {
        SRV_LOG_DEBUG(g_logger) << _name << " " << __func__;
        setThis();
        setHookEnable(true);
        if (_rootThreadid != std::this_thread::get_id())
        {
            // 构建每个线程的主协程
            t_schedulerFiber = Fiber::getThis().get();
        }
        // 空闲协程
        Fiber::ptr idleFiber(new Fiber(std::bind(&Scheduler::idle, this)));
        SRV_LOG_DEBUG(g_logger)<<"idleFiber: "<<idleFiber->getId();
        // 回调函数协程
        Fiber::ptr cbFiber;
        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool active = false;
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _fibers.begin();
            while (it != _fibers.end())
            {
                // 不是期望执行的线程，等待其他线程获取
                if (it->_threadid != std::thread::id() && it->_threadid != std::this_thread::get_id())
                {
                    ++it;
                    tickle_me = true;
                    continue;
                }
                // 可能是由外部线程的主协程调度的忽略（一般不会出现）
                if (it->_fiber && it->_fiber->getState() == Fiber::RUNNING)
                {
                    ++it;
                    continue;
                }

                ft = *it;
                _fibers.erase(it++);
                ++_activeThreadCount;
                active = true;
                break;
            }
            // 没有期望在该线程执行的协程时为假
            tickle_me |= it != _fibers.end();
            lock.unlock();
            // 通知其他线程，有处理不了的任务,无实装时不执行
            if (tickle_me)
            {
                tickle();
            }

            if (ft._fiber &&
                (ft._fiber->getState() != Fiber::DONE &&
                 ft._fiber->getState() != Fiber::EXCEPT))
            {
                ft._fiber->swapIn();
                --_activeThreadCount;

                if (ft._fiber->getState() == Fiber::READY)
                { // 就绪状态，重新加入
                    schedule(ft._fiber);
                }
                ft.reset();
            }
            else if (ft._cb)
            { // 函数任务
                if (cbFiber)
                {
                    cbFiber->reset(ft._cb);
                }
                else
                {
                    cbFiber.reset(new Fiber(ft._cb));
                }
                ft.reset();
                cbFiber->swapIn();
                --_activeThreadCount;

                if (cbFiber->getState() == Fiber::READY)
                { // 就绪状态，重新加入
                    schedule(cbFiber);
                    cbFiber.reset();
                }
                else if (cbFiber->getState() == Fiber::SUSPEND)
                {
                    // 暂停分离协程
                    cbFiber.reset();
                }
            }
            else
            {
                // 如果无效协程和任务，将减少活动线程，执行idle协程任务，默认挂起，相当于唤醒，什么都不做，挂起
                if (active)
                {
                    // 还有任务
                    --_activeThreadCount;
                    continue;
                }
                if (idleFiber->getState() == Fiber::DONE)
                {
                    SRV_LOG_DEBUG(g_logger) << _name << "idle fiber done";
                    break;
                }

                ++_idleThreadCount;
                idleFiber->swapIn();
                --_idleThreadCount;
            }
        }
    }

    bool Scheduler::stopping()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _stopping && _fibers.empty() && _activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        SRV_LOG_INFO(g_logger) << __func__;
        while (!stopping())
        {
            Fiber::yieldToSuspend();
        }
        SRV_LOG_INFO(g_logger) << __func__<<"endl";
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

} // namespace WebSrv
