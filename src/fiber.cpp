#include "fiber.h"
#include <atomic>
#include "macro.h"
#include "log.h"
#include "scheduler.h"
namespace WebSrv
{
    static Logger::ptr g_logger = SRV_LOGGER_NAME("system");

    static std::atomic<uint64_t> s_fiberId = 0;
    static std::atomic<uint64_t> s_fiberCount = 0;
    /// @brief 当前协程
    static thread_local Fiber *t_fiber = nullptr;
    /// @brief 当前线程协程
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    /**
     * @brief 栈空间配置类
     *
     */
    class StackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    Fiber::Fiber()
    {
        _state = RUNNING;
        setThis(this);
        ++s_fiberCount;
        getcontext(&_ctx);
        SRV_LOG_DEBUG(g_logger) << "main fiber create";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : _id(++s_fiberId),
          _cb(cb)
    {
        ++s_fiberCount;
        _stacksize = 128 * 1024;
        _stack = StackAllocator::Alloc(_stacksize);
        getcontext(&_ctx);
        _ctx.uc_link = nullptr;
        _ctx.uc_stack.ss_sp = _stack;
        _ctx.uc_stack.ss_size = _stacksize;

 
        makecontext(&_ctx, &Fiber::mainFunc, 0); // 主协程调度
 

        SRV_LOG_DEBUG(g_logger) << "fiber create id=" << _id;
    }

    Fiber::~Fiber()
    {
        --s_fiberCount;
        if (_stack)
        {
            StackAllocator::Dealloc(_stack, _stacksize);
        }
        else
        {
            Fiber *cur = t_fiber;
            if (cur == this)
            {
                setThis(nullptr);
            }
        }

        SRV_LOG_DEBUG(g_logger) << "fiber destruction id=" << _id << " total=" << s_fiberCount;
    }

    void Fiber::reset(std::function<void()> cb)
    {
        WebSrvAssert(_stack);
        WebSrvAssert(_state == INIT || _state == EXCEPT || _state == DONE);
        _cb = cb;
        getcontext(&_ctx);
        _ctx.uc_link = nullptr;
        _ctx.uc_stack.ss_sp = _stack;
        _ctx.uc_stack.ss_size = _stacksize;

        makecontext(&_ctx, &Fiber::mainFunc, 0);
        _state = INIT;
    }

    void Fiber::resume()
    {
        setThis(this);
        WebSrvAssert(_state != RUNNING);
        _state = RUNNING;
        swapcontext(&t_threadFiber->_ctx, &_ctx);
    }

    void Fiber::yield()
    {
        // 切换为主协程
        setThis(Scheduler::getMainFiber());
        WebSrvAssert(_state != SUSPEND);
        if (_state == RUNNING)
        {
            _state = SUSPEND;
        }
        swapcontext(&_ctx, &t_threadFiber->_ctx);
    }

    void Fiber::setThis(Fiber *fiber)
    {
        t_fiber = fiber;
    }

    Fiber::ptr Fiber::getThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        // 没有协程就创建一个主协程
        Fiber::ptr mainFiber(new Fiber);
        WebSrvAssert(t_fiber == mainFiber.get());
        t_threadFiber = mainFiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::yieldToReady()
    {
        Fiber::ptr cur = getThis();
        WebSrvAssert2(cur->_state == RUNNING, cur->_state);
        cur->_state = READY;
        cur->yield();
    }

    void Fiber::yieldToSuspend()
    {
        Fiber::ptr cur = getThis();
        WebSrvAssert2(cur->_state == RUNNING, cur->_state);
        cur->yield();
    }

    uint64_t Fiber::totalFibers()
    {
        return s_fiberCount;
    }

    void Fiber::mainFunc()
    {
        Fiber::ptr cur = getThis();
        WebSrvAssert(cur);
        try
        {
            WebSrvAssert(cur->_cb);
            cur->_cb();
            cur->_cb = nullptr;
            cur->_state = DONE;
        }
        catch (const std::exception &e)
        {
            cur->_state = EXCEPT;
            SRV_LOG_ERROR(g_logger) << "Fiber except " << e.what()
                                    << " fiber id=" << cur->getId();
        }
        catch (...)
        {
            cur->_state = EXCEPT;
            SRV_LOG_ERROR(g_logger) << "Fiber except"
                                    << "fiber id=" << cur->getId();
        }

        auto ptr = cur.get();
        cur.reset();
        ptr->yield();
        WebSrvAssert2(false, "never reach fiber_id=" + std::to_string(ptr->getId()));
    }

    uint64_t Fiber::getFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

} // namespace WebSrv
