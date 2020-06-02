//
// Created by Icharle on 2020/5/17.
//

#ifndef WEBNGINX_EVENTLOOPTHREADPOOL_H
#define WEBNGINX_EVENTLOOPTHREADPOOL_H

#include "../base/noncopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <vector>
#include <memory>
#include <functional>

class EventLoopThreadPool : noncopyable {
public:
    EventLoopThreadPool(EventLoop *baseloop, int numThreads);

    ~EventLoopThreadPool();

    void start();

    EventLoop *getNextLoop();

private:
    EventLoop *baseLoop;
    bool started;
    int numThreads;
    int next;
    std::vector<std::shared_ptr<EventLoopThread>> threads;
    std::vector<EventLoop *> loops;
};

#endif //WEBNGINX_EVENTLOOPTHREADPOOL_H
