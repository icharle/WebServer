//
// Created by Icharle on 2020/5/17.
//

#ifndef WEBNGINX_EVENTLOOPTHREAD_H
#define WEBNGINX_EVENTLOOPTHREAD_H

#include "base/noncopyable.h"
#include "base/Thread.h"
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "EventLoop.h"

class EventLoopThread : noncopyable {
public:
    EventLoopThread();

    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop;

    bool existing;

    Thread thread;

    MutexLock mutex;

    Condition condition;
};

#endif //WEBNGINX_EVENTLOOPTHREAD_H
