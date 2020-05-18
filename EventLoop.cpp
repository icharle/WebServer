//
// Created by Icharle on 2020/5/15.
//

#include "EventLoop.h"
#include <unistd.h>
#include "sys/eventfd.h"

__thread EventLoop *t_loopInThisThread = 0;

int createEventfd() {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop() :
        looping_(false),
        poller_(new Epoll()),
        wakeupFd_(createEventfd()),
        quit_(false),
        eventHandling_(false),
        callingPendingFunctors_(false),
        threadId_(CurrentThread::tid()),
        pwakeupChannel_(new Channel(this, wakeupFd_)) {
    if (t_loopInThisThread) {
        // todo 日志
    } else {
        t_loopInThisThread = this;
    }
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(std::bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(std::bind(&EventLoop::handleConn, this));
    poller_->add(pwakeupChannel_, 0);
}

EventLoop::~EventLoop() {
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleConn() {
    updatePoller(pwakeupChannel_, 0);
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char *) (&one), sizeof(one));
    if (n != sizeof(one)) {
        //todo 日志
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, (char *) &one, sizeof(one));
    if (n != sizeof(one)) {
        //todo 日志
    }
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::runInLoop(Functor &&cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor &&cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::loop() {
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;
    std::vector<shareChannel> ret;
    while (!quit_) {
        ret.clear();
        ret = poller_->poll();
        eventHandling_ = true;
        for (auto &it : ret) {
            it->handleEvents();
        }
        eventHandling_ = false;
        doPendingFunctors();
        poller_->handleExpired();
    }
    looping_ = false;
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}