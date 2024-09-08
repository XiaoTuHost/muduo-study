#pragma once
#ifndef __EVENTLOOP__H__
#define __EVENTLOOP__H__

#include "nocopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 事件循环 事件驱动 主要包含channel 和 poller模块
// eventloop at most one per thread

class EventLoop : nocopyable{
public:
    using  Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();

    // 退出事件循环
    void quit();

    Timestamp pollerReturnTime() const {return pollerReturnTime_;}

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把cb放入队列中唤醒loop执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeUp();

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断eventloop对象是否在自己的线程内
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}
private:
    void handleRead(); //wake up
    void doPendingFunctors(); //执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作，通过cas实现
    std::atomic_bool quit_; // 退出loop循环标志
    const pid_t threadId_; // 记录当前loop所在的线程id
    Timestamp pollerReturnTime_; // poller返回channel事件的时间点
    std::unique_ptr<Poller> poller_;

    int wakeUpFd_; //mainLoop获取到新的channel时通过轮询唤醒subLoop,通过该字段唤醒subLoop处理
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    std::mutex mutex_; //互斥锁用于保护回调操作容器的线程安全操作
    std::atomic_bool callingPendingFunctors_; // 标志当前loop是否需要执行的回调操作
    std::vector<Functor> pendingFunctors_; //存储loop所有需要执行的回调操作
};

#endif