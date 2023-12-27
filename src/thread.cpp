#include "thread.h"
#include "util.h"

namespace WebSrv
{
    static thread_local Thread *t_pThread = nullptr;
    static thread_local std::string t_Threadname = "UNKOWN";
    Thread::~Thread()
    {
        if(_thread.joinable()){
            _thread.detach();
        }
    }
    
    void Thread::join()
    {
        _thread.join();
    }

    std::thread::id Thread::getId()
    {
        return _threadId;
    }

    void Thread::setId()
    {
        _threadId = std::thread::id(getSystemTheadId());
    }
    std::string Thread::getName()
    {
        return t_Threadname;
    }
    Thread *Thread::getThis()
    {
        return t_pThread;
    }

    void Thread::setName(const std::string &name)
    {
        if (name == "")
        {
            return;
        }
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
        t_Threadname = name;
    }

    void Thread::setThis(Thread *thread)
    {
        t_pThread = thread;
    }
} // namespace WebSrv
