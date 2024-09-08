#include "EpollPoller.h"
#include "Logger.h"

#include <unistd.h>
#include <string.h>
#include <error.h>

// channel未添加到poller中
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if(epollfd_ < 0 ){
        LOG_FATAL("epoll_create error:%d \n",errno) ;
    }
}

EpollPoller::~EpollPoller(){
    ::close(epollfd_);
}

// poller中添加channel
void EpollPoller::updateChannel(Channel* channel){
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d event=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);

    if(index == kNew || index== kDeleted){
        if(index==kNew){
            int fd = channel->fd();
            channels_[fd]=channel;
        }    
            channel->set_index(kAdded);
            update(EPOLL_CTL_ADD,channel);
        
    }else{  // channel已经在chnnel上注册过
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }else{
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

// poller中删除channel
void EpollPoller::removeChannel(Channel* channel){

    LOG_INFO("func=%s fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),channel->index());

    int fd = channel->fd();
    channels_.erase(fd);

    int index = channel->index();
    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents,ChannelList *activeChannels) const{
    for(int i=0;i<numEvents;++i){
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // Eventloop拿到了它的poller给它的所有发生事件的channel列表了
    }
}

// 根据operation操作poller监听的fd和相应的事件 对epoll_ctl的抽象
void EpollPoller::update(int operation,Channel *channel){
    epoll_event event;
    memset(&event,0,sizeof event);
    event.events = channel -> events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl_del error:%d \n",errno);
        }else{
            LOG_FATAL("epoll_ctl add/mod error:%d \n",errno);
        }
    }
}

Timestamp EpollPoller::poll(int timeoutMs,ChannelList *activeChannels){
    LOG_INFO("func=%s => fd total count:%d \n",__FUNCTION__,channels_.size());

    int numEvents = ::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;

    Timestamp now(Timestamp::now());

    if(numEvents > 0){

        LOG_INFO("%d events happened \n",numEvents);
        fillActiveChannels(numEvents,activeChannels);
        if(numEvents == events_.size()){
            events_.resize(events_.size()*2);
        }

    }else if(numEvents == 0){
        
        LOG_DEBUG("%s timeout! \n "__FUNCTION__);
    
    }else{

        if(saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() error!\n");
        }

    }
}