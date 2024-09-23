#pragma once 
#ifndef __ACCEPTOR__H__
#define __ACCEPTOR__H__

#include "nocopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class InetAddress;
class EventLoop;

class Acceptor:nocopyable{
public:
    using NewConnectionCallBack = std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallBack(const NewConnectionCallBack& cb){ newConnectionCallBack_ = std::move(cb);}

    bool listenning()const {return listenning_;}
    void listen();

private:
    void handleRead();

    EventLoop* loop_; //acceptor使用的是用户自定义的loop->mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallBack newConnectionCallBack_;
    bool listenning_;
};

#endif