#pragma
#ifndef __EVENTLOOPTHREAD__H__
#define __EVENTLOOPTHREAD__H__

#include "nocopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread : nocopyable{

public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallBack &cb ,
        const std::string &name );

    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallBack callback_;

};


#endif