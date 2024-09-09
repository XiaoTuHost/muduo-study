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


// 退出事件循环
// mainloop ---threadpool--- subloop
void EventLoop::quit(){

    quit_ = true;

    if(!isInLoopThread()){
        wakeUp();
    }
    
}

// 在当前线程中执行cb
void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){ //在当前的loop线程中，执行cb
        cb();
    }
    else{   // 不在当前线程中执行cb，就需要唤醒loop所在的线程
        /*code*/
        queueInLoop(cb);
    }
}

// 将cb放入队列中，唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb){

    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的loop的线程了
    // || calling...表示当前loop正在执行回调，但是loop又有了新的回调
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeUp();
    }

}

// 用来唤醒loop所在的线程 向wakeupfd_写一个数据 wakeupChannel就发生读事件
void EventLoop::wakeUp(){
    uint64_t one = 1;
    ssize_t n = write(wakeUpFd_,&one,sizeof one);
    if(n!=sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors(){

    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors){
        functor();  //执行当前loop需要执行的回调操作 
    }

    callingPendingFunctors_ = false;
}