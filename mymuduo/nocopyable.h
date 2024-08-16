#pragma once 
#ifndef __NOCOPYABLE__H__
#define __NOCOPYABLE__H__

class nocopyable{
public:
    nocopyable(const nocopyable&) = delete;
    nocopyable& operator=(const nocopyable&) = delete;
protected:
    nocopyable() = default;
    ~nocopyable() = default;
};

#endif