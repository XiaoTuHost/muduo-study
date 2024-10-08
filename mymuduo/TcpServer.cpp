#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <functional>
#include <string.h>

static EventLoop* checkLoopNotNull(EventLoop *loop){
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
    for(auto&item : connections_){
        TcpConnectionPtr conn(item.second); // 这个局部的智能指针对象出作用域右括号，自动释放new出来的TcpConnction对象资源
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
    }
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

void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr){
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;std::string connName = name_+buf;
    
    LOG_INFO("TcpServer::newConnection [%s] - newConnection [%s] from %s \n",
        name_.c_str(), connName.c_str(),peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            connName,
                            sockfd, // Socket Channel
                            localAddr,
                            peerAddr)
    );

    connections_[connName] = conn;
    // 下面调用都是用户设置给TcpServer=>TcpConnction=>Channel=>Poller=>notify channel调用回调
    conn->setConnectionCallBack(connectionCallBack_);
    conn->setMessageCallBack(messageCallBack_);
    conn->setWriteCompleteCallBack(writeCompleteCallBack_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallBack(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );
    ioLoop->runInLoop(std::bind(&TcpConnection::connectedEstablished,conn));


}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn)
    );
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
    LOG_INFO("TcpServer::reomveConnectionInLoop [%s] - conncetion\n",name_.c_str(),conn->name().c_str());

    connections_.erase(conn->name());

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );
}


