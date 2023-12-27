#pragma once
#include <atomic>
#include "noncopyable.h"
#include <shared_mutex>
#include <mutex>
// 添加而外的锁类型

namespace WebSrv
{   
    //读写锁

    using RWMutex=std::shared_mutex;
    using WriteMutex = std::unique_lock<std::shared_mutex>;
    using ReadMutex = std::shared_lock<std::shared_mutex>;
    /**
     * @brief 自旋锁
     * 
     */
    class SpinLock : NonCopyable
    {
    public:
        void lock() noexcept
        {
            for (;;)
            {
                // 如果第一次尝试成功，它立即返回，表示成功获取锁
                if (!_lock.exchange(true, std::memory_order_acquire))
                {
                    return;
                }
                // 在不生成缓存失效的情况下等待锁的释放
                while (_lock.load(std::memory_order_relaxed))
                {
                    // 通过发出X86 PAUSE或ARM YIELD指令来减少超线程之间的竞争
                    __builtin_ia32_pause();
                }
            }
        }

        bool try_lock() noexcept
        {   
            //它首先进行松散的加载操作以检查锁的状态，以防止不必要的缓存失效。
            //如果锁处于空闲状态，尝试上锁
            return !_lock.load(std::memory_order_acquire) &&
                   !_lock.exchange(true, std::memory_order_acquire);
        }

        void unlock() noexcept
        {
            _lock.exchange(false, std::memory_order_acquire);
        }
    private:
        std::atomic<bool> _lock = {0};
    };
} // namespace WebSrv
