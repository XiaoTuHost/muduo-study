#include "TcpConnection.h"
#include "Logger.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"

#include <unistd.h>

static EventLoop* checkLoopNotNull(EventLoop *loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mainloop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                    const std::string nameArg,
                    int sockfd,
                    const InetAddress& localAddr,
                    const InetAddress& peerAddr)
        : loop_(checkLoopNotNull(loop))
        , name_(nameArg)
        , state_(kConnecting)
        , reading_(true)
        , socket_(new Socket(sockfd))
        , channel_(new Channel(loop,sockfd))
        , localAddr_(localAddr)
        , peerAddr_(peerAddr)
        , highWaterMark_(64*1024*1024) // 64m

{
    // 设置channel回调函数 poller给channel通知感兴趣的事件发生了，channel执行相应的回调操作
    channel_->setReadCallBack(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1)
    );
    channel_->setWriteCallBack(
        std::bind(&TcpConnection::handleWrite,this)
    );
    channel_->setCloseCallBack(
        std::bind(&TcpConnection::handleClose,this)
    );
    channel_->setErrorCallBack(
        std::bind(&TcpConnection::handleError,this)
    );
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd)
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n",
        name_.c_str(),channel_->fd(),(int)state_);
}    

void TcpConnection::handleRead(Timestamp receiveTime){
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n>0){
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作
        messageCallBack_(shared_from_this(),&inputBuffer_,receiveTime);
    }
    else if( n==0 ){
        handleClose();
    }else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0){
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();
                if(writeCompleteCallBack_){
                    // 唤醒loop对应的thread线程
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallBack_,shared_from_this())
                    );
                }
                if(state_ == kDisconnecting){
                    shutdownInLoop();
                }
            }
        }else{
            LOG_ERROR("TcpConnection::handleWrite");
        }   
    }else{
        LOG_ERROR("TcpConnection fd=%d is done , no more writing \n",channel_->fd());
    }
}

// poller => channel::closeCallBack => TcpConnction::handleClose
void TcpConnection::handleClose(){
    LOG_INFO("fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallBack_(connPtr);
    closeCallBack_(connPtr);
}
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0 ;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)){
        err =  errno;
    }
    else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",name_.c_str(),err);
}   

void TcpConnection::send(const std::string buf){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            
            sendInLoop(buf.c_str(),buf.size());
            
        }else{

            loop_->runInLoop(std::bind(
                    &TcpConnection::sendInLoop,
                    this,
                    buf.c_str(),
                    buf.size()
                )
            );

        }
    }
}

void TcpConnection::sendInLoop(const void* data,size_t len){
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if(state_==kDisconnected){
        LOG_ERROR("disconnected , give up writing!\n");
        return;
    }

    // 表示channel_第一次开始写数据，而缓冲区没有待发送的数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0){
        nwrote = ::write(channel_->fd(),data,len);
        if(nwrote>=0){

            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallBack_){
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(
                        writeCompleteCallBack_,
                        shared_from_this()
                    )
                );
            }
        }

    }else{
        nwrote = 0;
        if(errno!=EWOULDBLOCK){
            LOG_ERROR("TcpConnection::sendInLoop\n");
            if(errno == EPIPE || errno==ECONNRESET){
                faultError= true;
            }
        }
    }

    // 说明当前这一次的 write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中
    // 注册epollout事件，poller会发现tcp的发送缓冲区有空间，会通知相应的sokc->channel回调，调用handlewrite回调
    // 也就是调用handlewrite，把缓冲区数据发送完成
    if(!faultError && remaining>0){
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen+remaining>=highWaterMark_ && oldLen<highWaterMark_ && highWaterMarkCallBack_){
            loop_->queueInLoop(
                std::bind(
                    highWaterMarkCallBack_,
                    shared_from_this(),
                    oldLen+remaining
                )
            );
        }
        outputBuffer_.append((char*)data+nwrote,remaining);
        if(!channel_->isWriting()){
            // 注册channel事件，否则不会调用epollout
            channel_->eableWriting();
        }
    }
}

    // 建立连接
void TcpConnection::connectedEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->eableReading();   // 向poller注册channel的epollin事件

    // 新连接建立，执行回调
    connectionCallBack_(shared_from_this());

}

// 销毁连接
void TcpConnection::connectDestroyed(){
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallBack_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop,this)
        );
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){ // 说明outputbuffer中的数据已经全部发送

        socket_->shutdownWrite(); // 关闭写端

    }
}