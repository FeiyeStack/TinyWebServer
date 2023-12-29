#pragma once
#include <memory>
#include <functional>
#include <ucontext.h>
namespace WebSrv
{
    class Scheduler;
    /**
     * @brief 协程类，每根协程一个栈
     */
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        friend class Scheduler;
    public:
        using ptr=std::shared_ptr<Fiber>;
        enum State{
            //初始
            INIT,
            //就绪
            READY,
            //运行
            RUNNING,
            //暂停
            SUSPEND,
            //结束
            DONE,
            //异常
            EXCEPT,
        };
    private:
        //每个线程第一个协程，主协程（切换子协程任务）
        Fiber();
    public:
        /**
         * @brief Construct a new Fiber object
         * 
         * @param cb 协程执行函数
         * @param stacksize 协程栈大小
         * @param use_caller 是否在MainFiber上调度
         */
        Fiber(std::function<void()> cb, size_t stacksize = 0,bool use_caller=false);
    
        ~Fiber();
        /**
         * @brief 重置协程，将协程设为INIT,作用于已结束（DONE,EXCEPT）或未开始（INIT）
         * 
         * @param cb 
         */
        void reset(std::function<void()> cb);
        /**
         * @brief 将当前线程切换到运行状态(由当前线程的主协程执行)
         * 
         */
        void call();
        /**
         * @brief 将当前线程切出到后台(由当前线程的主协程执行)
         *
         */
        void back();
        /**
         * @brief 将当前协程切换到运行状态
         *
         */
        void swapIn();
        /**
         * @brief 将当前协程切出到后台
         *
         */
        void swapOut();
        /**
         * @brief Get fiber Id
         * 
         * @return uint64_t 
         */
        uint64_t getId() const {return _id;}
        /**
         * @brief Get fiber Status
         * 
         * @return uint64_t 
         */
        uint64_t getState() const {return _state;}
    public:
        /**
         * @brief Set the This current thread running fiber
         * 
         * @param f 
         */
        static void setThis(Fiber* fiber);
        /**
         * @brief 返回当前的协程
         * 
         * @return Fiber::ptr 
         */
        static Fiber::ptr getThis();
        /**
         * @brief 将当前协程切换到后台,并设置为READY状态
         * 
         */
        static void yieldToReady();
        /**
         * @brief 将当前协程切换到后台,并设置为SUSPEND状态
         * 
         */
        static void yieldToSuspend();
        /**
         * @brief 当前协程的总数
         * 
         * @return uint64_t 
         */
        static uint64_t totalFibers();
        /**
         * @brief 协程执行函数，执行完成后到线程主协程
         * 
         */
        static void mainFunc();
        /**
         * @brief 协程执行函数，执行完成后到线程调度器
         * 
         */
        static void callerMainFunc();
        /**
         * @brief 获取当前协程id
         * 
         * @return uint64_t 
         */
        static uint64_t getFiberId();
    private:
        /// @brief 协程id
        uint64_t _id=0;
        /// @brief 协程状态
        State _state=INIT;
        /// @brief 协程上下文
        ucontext_t _ctx;
        /// @brief 运行栈指针
        void* _stack = nullptr;
        /// @brief 运行栈大小
        uint32_t _stacksize=0;
        /// @brief 协程运行函数
        std::function<void()> _cb;
    };

} // namespace WebSrv