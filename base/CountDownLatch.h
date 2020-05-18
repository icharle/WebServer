//
// Created by Icharle on 2020/5/16.
//

#ifndef WEBNGINX_COUNTDOWNLATCH_H
#define WEBNGINX_COUNTDOWNLATCH_H

#include "noncopyable.h"
#include "Condition.h"
#include "MutexLock.h"

class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count_);

    void wait();

    void countDown();

private:
    mutable MutexLock mutex;
    Condition condition;
    int count;
};

#endif //WEBNGINX_COUNTDOWNLATCH_H
