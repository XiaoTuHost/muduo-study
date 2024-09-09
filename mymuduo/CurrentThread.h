#pragma once
#ifndef __CURRENTTHREAD__H__
#define __CURRENTTHREAD__H__

namespace CurrentThread{
    extern __thread int t_cachedTid;


    void cacheTid();

    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0,0)){
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif