#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <unistd.h>

// 防止一个线程创建多个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;


// 定义默认的poller IO复用的超时时间
const int  kPollTimeMs = 10000;

// create a file descriptor for event notification
int createEventFd(){
    int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("eventfd error:%d \n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_ (Poller::newDefaultPoller(this))
    , wakeUpFd_(createEventFd())
    , wakeupChannel_(new Channel(this,wakeUpFd_))
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    }else{
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的时间类型以及发生事件后的回调操作

    // std::bind 第二个参数this???
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead,this));
    // 每一个eventloop都监听wakeupchannel的epollin读事件
    wakeupChannel_->eableReading();

}

EventLoop::~EventLoop()
{   
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeUpFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead(){

    uint64_t one = 1;
    ssize_t n = read(wakeUpFd_,&one,sizeof one);
    if(n !=sizeof one ){
        LOG_ERROR("EventLoop::handRead() reads %d bytes instead of 8 \n",n);
    }

}

void EventLoop::loop(){
    looping_= true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n",this);

    while(!quit_){
        activeChannels_.clear();
        pollerReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for(auto  *channel:activeChannels_){
            // poller监听哪些channel发生事件了，然后上报给eventloop，通知channel处理相应的事件 
            channel ->handleEvent(pollerReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
         IO线程 mainLoop accept fd channel subloop
         mainLoop 实现注册一个回调cb(需要subLoop来执行) wakeup
        */
        doPendingFunctors();

    }

    LOG_INFO("EventLoop %p stop looping. \n",this);
    looping_ = false;

}

void EventLoop::quit(){

    quit_ = true;

    if(!isInLoopThread()){
        wakeUp();
    }
    
}