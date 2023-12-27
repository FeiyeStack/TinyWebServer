#pragma once
#include "scheduler.h"
#include "timer.h"
namespace WebSrv
{
    class IOManager : public Scheduler, public TimerManager
    {
    public:
        using ptr = std::shared_ptr<IOManager>;
        enum Event
        {
            NONE = 0x0,
            // 读事件（EPOLLIN）
            READ = 0x1,
            // 写事件（EPOLLOUT）
            WRITE = 0x4,
        };

    private:
        struct FdContext
        {
            struct EventContext
            {
                Scheduler *scheduler = nullptr;
                Fiber::ptr fiber;
                std::function<void()> cb;
            };

            EventContext &getContext(Event event);
            void resetEventContext(EventContext &ctx);
            void triggerEvent(Event event);
            EventContext read;
            EventContext write;
            Event events = NONE;
            std::mutex mutex;
            int fd;
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        ~IOManager();
        /**
         * @brief 添加事件
         *
         * @param fd
         * @param event
         * @param cb
         * @return int
         */
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

        /**
         * @brief 取消事件,如果存在的话,立刻触发事件
         *
         * @param fd
         * @param event
         * @return true
         * @return false
         */
        bool cancelEvent(int fd, Event event);

        /**
         * @brief 取消该句柄的所有事件,如果存在的话,立刻触发所有事件
         *
         * @param fd
         * @return true
         * @return false
         */
        bool cancelAll(int fd);

        /**
         * @brief 取消该句柄的事件
         *
         * @param fd
         * @param event
         * @return true
         * @return false
         */
        bool delEvent(int fd, Event event);

        static IOManager *getThis();

        /**
         * @brief 停止运行
         *
         */
        void stop() override;
    protected:
        void tickle() override;
        bool stopping() override;
        bool stopping(uint64_t timeout);
        void idle() override;
        void onTimerInsertedAtFront() override;
        /**
         * @brief socket句柄上下文容器大小
         *
         * @param size
         */
        void contextResize(size_t size);

    private:
        /// @brief epoll文件句柄
        int _epollFd = 0;
        /// @brief pipe文件句柄
        int _tickleFds[2];
        /// @brief 当前等待执行的事件数量
        std::atomic<uint64_t> _pendingEventCount = 0;
        RWMutex _rwMutex;

        std::vector<FdContext *> _fdContexts;
    };
} // namespace WebSrv