#pragma once
#ifndef __TCPSERVER__H__
#define __TCPSERVER__H__

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "nocopyable.h"
#include "EventLoopThreadPool.h"
#include "CallBacks.h"

#include <functional>
#include <string>
#include <atomic>
#include <unordered_map>

class TcpServer : nocopyable{
public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    enum Option{
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
                    const InetAddress& listenAddr,
                    const std::string &name,
                    Option option = kNoReusePort);

    ~TcpServer();

    void setTreadInitcallBack(const ThreadInitCallBack &cb){ threadInitCallBack_ = cb;}
    void setConnectionCallBack(const ConnectionCallBack &cb){ connectionCallBack_ = cb;}
    void setMessageCallBack(const MessageCallBack &cb){ messageCallBack_ = cb;}
    void setWriteCompleteCallBack(const WriteCompleteCallBack &cb){ writeCompleteCallBack_ = cb;}

    // 设置subloop的数目
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();               
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainLoop 监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallBack connectionCallBack_; // 有新连接时的回调
    MessageCallBack messageCallBack_; // 有消息时的回调
    WriteCompleteCallBack writeCompleteCallBack_; // 消息发送完成后的回调
    ThreadInitCallBack threadInitCallBack_; // 线程初始化回调
    std::atomic_int started_;

    int  nextConnId_;

    ConnectionMap connections_;
};


#endif