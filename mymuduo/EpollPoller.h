#pragma once
#ifndef __EPOLLPOLLER__H__
#define __EPOLLPOLLER__H__

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

/*
Epoll使用:
1. epoll_create
2. epoll_ctl
3. epoll_wait
*/


class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop * loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs ,ChannelList * activeChannels) override;
    void updateChannel(Channel * channel) override;
    void removeChannel(Channel * channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    // 这里是epoll_wait实际发生的事件 使用vector方便扩容
    EventList events_;
};

#endif