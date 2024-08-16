#pragma once
#ifndef __POLLER__H__
#define __POLLER__H__

#include "Channel.h"
#include "nocopyable.h"

#include <vector>
#include <unordered_map>

class EventLoop;

class Poller : nocopyable{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop * loop);
    virtual ~Poller();

    // 给所有IO复用保留统一接口
    virtual Timestamp poll(int timeoutMs,ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel * channel) = 0;
    virtual void removeChannel(Channel * channel) = 0;

    // 判断channel是否在poller中
    bool hasChannel(Channel * channel) const ;

    // eventloop可以通过该接口获取poller对象
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // key->sockfd  value->sockfd所属的channel
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;
};

#endif