#include "timer.h"
#include "util.h"
namespace WebSrv
{
    bool Timer::cannel()
    {
        WriteMutex lock(_manager->_rWMutex);
        //std::unique_lock<std::mutex> lock(_manager->_mutex);
        // 取出后会移除回调函数
        if (_cb)
        {
            _cb = nullptr;
            auto it = _manager->_timerSet.find(shared_from_this());
            if (it != _manager->_timerSet.end())
            {
                _manager->_timerSet.erase(it);
                return true;
            }
        }
        return false;
    }

    bool Timer::refresh()
    {
        WriteMutex lock(_manager->_rWMutex);
        //std::unique_lock<std::mutex> lock(_manager->_mutex);
        if (!_cb)
        {
            return false;
        }
        auto it = _manager->_timerSet.find(shared_from_this());
        if (it == _manager->_timerSet.end())
        {
            return false;
        }
        _manager->_timerSet.erase(it);
        _next = getCurrentMilliseconds() + _ms;
        _manager->_timerSet.emplace(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool fromNow)
    {
        // 啥也一样就不重置了
        if (ms == _ms && !fromNow)
        {
            return true;
        }
        WriteMutex lock(_manager->_rWMutex);
        //std::unique_lock<std::mutex> lock(_manager->_mutex);
        if (!_cb)
        {
            return false;
        }
        auto it = _manager->_timerSet.find(shared_from_this());
        if (it == _manager->_timerSet.end())
        {
            return false;
        }
        _manager->_timerSet.erase(it);
        bool maybeFront = false;
        if (fromNow)
        {
            _next = getCurrentMilliseconds() + ms;
        }
        else
        {
            // 在原来的时间戳上更改
            _next = _next - _ms + ms;
            // 有可能在顶部
            maybeFront = true;
        }
        _ms = ms;
        if (maybeFront)
        {
            auto it = _manager->_timerSet.insert(shared_from_this()).first;
            bool atFront = (it == _manager->_timerSet.begin()) && !_manager->_tickled;
            if (atFront)
            {
                _manager->_tickled = true;
            }
            if (atFront)
            {
                _manager->onTimerInsertedAtFront();
            }
        }
        else
        {
            _manager->_timerSet.emplace(shared_from_this());
        }

        return true;
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : _ms(ms), _cb(cb), _recurring(recurring), _manager(manager), _next(getCurrentMilliseconds() + _ms)
    {
    }

    Timer::Timer(uint64_t next)
        : _next(getCurrentMilliseconds())
    {
    }

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        // 先将空指针取出，然后在比较大小
        if (lhs ==nullptr&& rhs==nullptr)
        {
            return false;
        }
        if (lhs==nullptr)
        {
            return true;
        }
        if (rhs==nullptr)
        {
            return false;
        }
        if(lhs->_next < rhs->_next){
            return true;
        }
        if(lhs->_next > rhs->_next){
            return false;
        }
        //如果直接用_next比较会出现相同_next被忽略，这是糟糕的
        return lhs.get()<rhs.get();
    }

    

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        WriteMutex lock(_rWMutex);
        //std::unique_lock<std::mutex> lock(_mutex);
        // 不能直接比较头部，因为头部可能是失效指针
        auto it = _timerSet.insert(timer).first;
        bool atFront = (it == _timerSet.begin()) && !_tickled;
        if (atFront)
        {
            _tickled = true;
        }
        //_rWMutex.unlock();
        lock.unlock();
        if (atFront)
        {
            onTimerInsertedAtFront();
        }
        return timer;
    }

    // 当条件回调为false是，不执行回调
    static void _onTimer(std::function<bool()> cond, std::function<void()> cb)
    {
        bool isActivity = cond();
        if (isActivity)
        {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::function<bool()> cond, bool recurring)
    {
        return addTimer(ms,[cb, cond]() { _onTimer(cond, cb); },recurring);
    }

    void TimerManager::listExpiredCallback(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now = getCurrentMilliseconds();
        std::vector<Timer::ptr> res;
        ReadMutex readlock(_rWMutex);
        if (_timerSet.empty())
        {
            return;
        }
        readlock.unlock();
        //std::unique_lock<std::mutex> lock(_mutex);
        WriteMutex writelock(_rWMutex);
        // 可能被捷足先登了
        if (_timerSet.empty())
        {
            return;
        }
        Timer::ptr timeoutAfter(new Timer(now));
        // 返回小于timeoutAfter的迭代器位置
        auto it = _timerSet.lower_bound(timeoutAfter);
        while (it != _timerSet.end() && (*it)->_next == now)
        {
            it++;
        }
        // 在头部，未超时，不操作了
        if(it==_timerSet.begin()){
            return;
        }
        res.insert(res.begin(), _timerSet.begin(), it);
        _timerSet.erase(_timerSet.begin(), it);
        cbs.reserve(res.size());
        for (auto &timer : res)
        {
            cbs.emplace_back(timer->_cb);
            if (timer->_recurring&&!_closeRecurring)
            {
                timer->_next = now + timer->_ms;
                _timerSet.emplace(timer);
            }
            else
            {
                timer->_cb = nullptr;
            }
        }
    }

    std::function<void()> TimerManager::getExpiredCallback()
    {
        uint64_t now_ms = getCurrentMilliseconds();
        ReadMutex readlock(_rWMutex);
        if (_timerSet.empty())
        {
            return nullptr;
        }
        readlock.unlock();
        //std::unique_lock<std::mutex> lock(_mutex);
        WriteMutex writelock(_rWMutex);
        if (_timerSet.empty())
        {
            return nullptr;
        }
        if((*_timerSet.begin())->_next<now_ms){
            auto timer= *_timerSet.begin();
            _timerSet.erase(_timerSet.begin());
            auto cb=timer->_cb;
            timer->_cb=nullptr;
            return cb;
        }else{
            return nullptr;
        }
    }

    bool TimerManager::hasTimer()
    {
        ReadMutex rLock(_rWMutex);
        //std::lock_guard<std::mutex> lock(_mutex);
        return !_timerSet.size();
    }

    uint64_t TimerManager::getNextTimer()
    {
        ReadMutex lock(_rWMutex);
        //std::lock_guard<std::mutex> lock(_mutex);

        _tickled = false;
        if (_timerSet.empty())
        {
            return ~0ull;
        }

        const Timer::ptr &next = *_timerSet.begin();
        uint64_t now = getCurrentMilliseconds();
        if (now >= next->_next)
        {
            return 0;
        }
        else
        {
            return next->_next - now;
        }
    }

    void TimerManager::closeRecurringTimers()
    {
        _closeRecurring=true;
    }

} // namespace WebSrv
