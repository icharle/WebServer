//
// Created by Icharle on 2020/5/16.
//

#ifndef WEBNGINX_CURRENTTHREAD_H
#define WEBNGINX_CURRENTTHREAD_H

#include <stdint.h>

namespace CurrentThread {
    extern __thread int t_cacheTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;

    void cacheTid();

    inline int tid() {
        if (__builtin_expect(t_cacheTid == 0, 0)) {
            cacheTid();
        }
        return t_cacheTid;
    }

    inline const char *tidString() {
        return t_tidString;
    }

    inline int tidStringLength() {
        return t_tidStringLength;
    }

    inline const char *name() {
        return t_threadName;
    }
}

#endif //WEBNGINX_CURRENTTHREAD_H
