#pragma once
#ifndef __CURRENTTHREAD__H__
#define __CURRENTTHREAD__H__

namespace CurrentThread{
    __thread int t_cachedTid = 0;


    void cacheTid();

    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0,0)){
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif