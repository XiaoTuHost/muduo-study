#pragma once
#ifndef __CALLBACKS__H__
#define __CALLBACKS__H__

#include "Timestamp.h"

#include <memory>
#include <functional>


class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallBack = std::function<void(const TcpConnectionPtr&)>;
using CloseCallBack = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallBack = std::function<void(const TcpConnectionPtr&)>;

using MessageCallBack = std::function<void (
    const TcpConnectionPtr&,
    Buffer*,
    Timestamp
)>;


#endif