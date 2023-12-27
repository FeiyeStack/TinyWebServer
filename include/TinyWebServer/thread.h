#pragma once
#include <thread>
#include <memory>
#include <tuple>
#include <functional>
#include "mutex.h"
#include <condition_variable>
#include "noncopyable.h"
// 有bug在这[](){}表达式中出现bug
//  对std::thread封装，添加线程名,修改线程id为系统id
namespace WebSrv
{
    class Thread : NonCopyable
    {
    public:
        using ptr = std::shared_ptr<Thread>;
        template <typename Function, typename... Args>
        explicit Thread(const std::string &name, Function &&fn, Args &&...args)
            : _name(name)
        {
            std::function<decltype(fn(args...))()> func = std::bind(std::forward<Function>(fn), std::forward<Args>(args)...);
            _work = [func]()
            {
                func();
            };
            _thread = std::thread([this]()
                                  {
            setName(_name);
            setId();
            setThis(this);
            _work(); });
        }

        ~Thread();
        /**
         * @brief 等待线程结束
         *
         */
        void join();

        /**
         * @brief 获取线程名
         *
         * @return std::string
         */
        static std::string getName();
        /**
         * @brief 获取线程中的this指针
         *
         * @return Thread*
         */
        static Thread *getThis();
        /**
         * @brief 获取系统层线程id
         *
         * @return std::thread::id
         */
        std::thread::id getId();
        /**
         * @brief 设置线程名
         * 
         * @param name 
         */
        static void setName(const std::string &name);
    private:
        void setId();
        void setThis(Thread *thread);

    private:
        std::string _name;
        std::function<void()> _work;
        std::thread::id _threadId;
        std::thread _thread;
        std::mutex _mutex;
    };

} // namespace Srv
