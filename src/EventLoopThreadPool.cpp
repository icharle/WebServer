//
// Created by Icharle on 2020/5/17.
//

#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, int numThreads) :
        baseLoop(baseloop),
        started(false),
        numThreads(numThreads),
        next(0) {
    if (numThreads <= 0) {
        abort();
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
    //todo 日志
}

void EventLoopThreadPool::start() {
    baseLoop->assertInLoopThread();
    started = true;
    for (int i = 0; i < numThreads; ++i) {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        threads.push_back(t);
        loops.push_back(t->startLoop());
    }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    baseLoop->assertInLoopThread();
    assert(started);
    EventLoop *loop = baseLoop;
    if (!loops.empty()) {
        loop = loops[next];
        next = (next + 1) % numThreads;
    }
    return loop;
}