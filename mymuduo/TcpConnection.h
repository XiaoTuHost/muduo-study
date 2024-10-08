#pragma once
#ifndef __TCPCONNECTION__H__
#define __TCPCONNECTION__H__

#include "nocopyable.h"
#include "InetAddress.h"
#include "CallBacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <atomic>
#include <string>

class EventLoop;
class Channel;
class Socket;

class TcpConnection : nocopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop *loop,
                    const std::string nameArg,
                    int sockfd,
                    const InetAddress& localAddr,
                    const InetAddress& peerAddr);
    
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const { return name_;}
    const InetAddress& localAddr() const {return localAddr_;}
    const InetAddress& peerAddr() const {return peerAddr_;}

    bool connected() const {return state_ == kConnected;}

    // 发送数据
    void send(const std::string buf);
    // 关闭连接
    void shutdown();

    void setConnectionCallBack(const ConnectionCallBack& cb) {connectionCallBack_ = cb;}
    void setMessageCallBack(const MessageCallBack &cb) { messageCallBack_ = cb ;}
    void setWriteCompleteCallBack(const WriteCompleteCallBack &cb){ writeCompleteCallBack_ = cb;}
    void setHighWaterMarkCallBack(const HighWaterMarkCallBack &cb,size_t highWaterMark) { highWaterMarkCallBack_ = cb,highWaterMark_ = highWaterMark;}
    void setCloseCallBack(const CloseCallBack &cb) { closeCallBack_ = cb; }

    // 建立连接
    void connectedEstablished();
    // 销毁连接
    void connectDestroyed();

    

private:
    enum StateE {kDisconnected,kConnecting,kConnected,kDisconnecting};
    void setState(StateE state) { state_ = state;}

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();


    void sendInLoop(const void* data,size_t len);
    
    void shutdownInLoop();



    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallBack connectionCallBack_;
    MessageCallBack messageCallBack_;
    WriteCompleteCallBack writeCompleteCallBack_;
    HighWaterMarkCallBack highWaterMarkCallBack_;
    CloseCallBack closeCallBack_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

};

#endif