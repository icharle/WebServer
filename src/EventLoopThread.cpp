//
// Created by Icharle on 2020/5/17.
//

#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread() :
        loop(nullptr),
        existing(false),
        thread(std::bind(&EventLoopThread::threadFunc, this)),
        mutex(),
        condition(mutex) {
}

EventLoopThread::~EventLoopThread() {
    existing = true;
    if (loop != nullptr) {
        loop->quit();
        thread.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    assert(!thread.started());
    thread.start();
    {
        MutexLockGuard lock(mutex);
        while (loop == nullptr) {
            condition.wait();
        }
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop_;
    {
        MutexLockGuard lock(mutex);
        loop = &loop_;
        condition.notify();
    }
    loop_.loop();
    loop = nullptr;
}