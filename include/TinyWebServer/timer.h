#pragma once
#include <memory>
#include <functional>
#include "mutex.h"
#include <set>
namespace WebSrv
{
    class TimerManager;

    /**
     * @brief 定时器
     *
     */
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;
    public:
        using ptr=std::shared_ptr<Timer>;
        /**
         * @brief 取消定时器
         * 
         */
        bool cannel();
        /**
         * @brief 立刻刷新设置的执行时间
         * 
         */
        bool refresh();

        /**
         * @brief 重置定时器时间
         * 
         * @param ms 定时器执行间隔
         * @param fromNow 是否从当前时间算起
         * @return true 
         * @return false 
         */
        bool reset(uint64_t ms,bool fromNow);
    private:
        //只允许TimerManager 创建Timer
        /**
         * @brief Construct a new Timer object
         * 
         * @param ms 定时器执行间隔
         * @param cb 回调函数
         * @param recurring 是否循环执行
         * @param manager 定时器管理
         */
        Timer(uint64_t ms,std::function<void()> cb, bool recurring,TimerManager* manager);
        
        /**
         * @brief 用来取超时部分用的
         * 
         * @param next 
         */
        Timer(uint64_t next);

        /**
         * @brief set比较仿函数
         * 
         */
        struct Comparator
        {
            bool operator()(const Timer::ptr& lhs,const Timer::ptr& rhs) const;
        };
        
        //结束所有循环任务
    private:
        uint64_t _ms;
        std::function<void()> _cb; 
        // 是否循环执行
        bool _recurring = false;
        TimerManager *_manager = nullptr;
        // 时间戳
        uint64_t _next;
    };


    class TimerManager{
    friend class Timer;
    public:
        TimerManager()=default;
        virtual ~TimerManager()=default;
        /**
         * @brief Construct a new Timer object
         *
         * @param ms 定时器执行间隔
         * @param cb 回调函数
         * @param recurring 是否循环执行
         */
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring=false);

        /**
         * @brief 条件定时器
         * 
         * @param ms 定时器执行间隔
         * @param cb 回调函数
         * @param recurring 是否循环执行
         * @param cond 条件回调
         * @return Timer::ptr 
         */
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::function<bool()> condCb,bool recurring=false);

        /**
         * @brief 获取超时定时器列表
         * 
         * @param cbs 超时定时器列表
         */
        void listExpiredCallback(std::vector<std::function<void()>>& cbs);

        /**
         * @brief 取出一个超时定时器
         * 
         * @return std::function<void()> 
         */
        std::function<void()> getExpiredCallback();
        /**
         * @brief 是否还有定时器
         * 
         * @return true 
         * @return false 
         */
        bool hasTimer();
        /**
         * @brief 下一个定时器执行时间
         * 为0超时 为最大时为空
         */
        uint64_t getNextTimer();
        /**
         * @brief 关闭循环定时器,结束后用的
         *
         */
        void closeRecurringTimers();
    protected:
        /**
         * @brief 如果定时插入在顶部时触发操作
         * 
         */
        virtual void onTimerInsertedAtFront()=0;
    private:
        std::atomic_bool _closeRecurring=false;
        std::shared_mutex _rWMutex;
        std::set<Timer::ptr,Timer::Comparator> _timerSet;
        //触发一次后就要使用getNextTimer重新读时间，所以不用反复触发
        bool _tickled=false;
        //std::mutex _mutex;
    };
} // namespace WebSrv


