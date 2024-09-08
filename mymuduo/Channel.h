#pragma once
#ifndef __CHANNEL__H__
#define __CHANNEL__H__

#include "nocopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

// channel 封装了sockfd和fd感兴趣的事件
// channel 绑定了poller返回的具体事件
// 由于channel知晓了发生的事件 所以需要事件回调函数来处理事件
class Channel : nocopyable {
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    //设置回调对象
    void setReadCallBack(ReadEventCallBack cb) { readCallBack_ = std::move(cb) ;}
    void setWriteCallBack(EventCallBack cb) { writeCallBack_ = std::move(cb) ;}
    void setCloseCallBack(EventCallBack cb) { closeCallBack_ = std::move(cb) ;}
    void setErrorCallBack(EventCallBack cb) {errorCallBack_ = std::move(cb) ;}

    //防止channel被手动remove掉channel还在执行回调操作

    // Tie this channel to the owner object managed by shared_ptr,
    // prevent the owner object being destroyed in handleEvent.
    void tie(const std::shared_ptr<void>&);

    int fd()const  {return fd_;}
    int events() const {return events_;}
    int set_revents(int revt) { revents_ = revt;}

    // 设置fd当前的事件状态
    void eableReading(){events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void eableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    //返回当前fd事件的状态
    bool isNoneEvent() const {return events_==kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}

    int index() const {return index_;}
    void set_index(int idx) {index_ = idx;}

    // one loop per thread
    EventLoop* onwerLoop() const {return loop_;}
    void remove();

private:

    void update();
    void handleEventWithGuard(Timestamp receivceTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; //事件循环
    const int fd_; // fd , poller所监听的对象
    int events_; // 注册的感兴趣的事件
    int revents_; //poller返回的实际发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallBack readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack errorCallBack_;
};

#endif