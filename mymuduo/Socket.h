#pragma once
#ifndef __SOCKET__H__
#define __SOCKET__H__

#include "nocopyable.h"

class InetAddress;

class Socket:nocopyable{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const {return sockfd_;}

    void bindAdress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();
    
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void seteusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};

#endif