#pragma once
#ifndef __TIMESTAMP__H__
#define __TIMESTAMP__H__

#include <stdint.h>
#include <string>

class Timestamp{
public:
    Timestamp();
    Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;

};

#endif