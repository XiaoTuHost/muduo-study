#include "TcpServer.h"
#include "Logger.h"

#include <functional>

EventLoop* checkLoopNotNull(EventLoop *loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mainloop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
                    const InetAddress& listenAddr,
                    const std::string &name,
                    Option option)
    :   loop_(checkLoopNotNull(loop))
    ,   ipPort_(listenAddr.toIpPort())
    ,   name_(name)
    ,   acceptor_(new Acceptor(loop,listenAddr,option == kReusePort))
    ,   threadPool_(new EventLoopThreadPool(loop,name_))
    ,   connectionCallBack_()
    ,   messageCallBack_()
    ,   nextConnId_(1)
{
    // 用户有新连接时会执行new connection回调
    acceptor_->setNewConnectionCallBack(std::bind(&TcpServer::newConnection,this,
                    std::placeholders::_1,std::placeholders::_2));
    

}

TcpServer::~TcpServer(){

}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    
}

void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(){
    // 防止一个TcpServer对象被start多次
    if(started_++==0){
        threadPool_->start(threadInitCallBack_); // 启动底层的线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}  
