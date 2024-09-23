#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking(){
    int sockfd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    if( sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__ ,__FUNCTION__,__LINE__,errno);
    }
        
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop,acceptSocket_.fd())
    , listenning_(false) 
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.seteusePort(true);
    acceptSocket_.bindAdress(listenAddr);
    
    acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.eableReading(); // acceptChannel => Poller
}

void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);

    if(connfd >=0 ){
        if(newConnectionCallBack_){
            newConnectionCallBack_(connfd,peerAddr); // 轮询寻找subLoop 唤醒分发新客户端的Channel
        }
        else{
            ::close(connfd);    
        }
    }
    else{
        LOG_ERROR("%s:%s:%d accept error:%d \n",__FILE__ ,__FUNCTION__,__LINE__,errno);
        if(errno == EMFILE){
            LOG_ERROR("%s:%s:%d accept error:%d \n",__FILE__ ,__FUNCTION__,__LINE__,errno);
        }
    }
}