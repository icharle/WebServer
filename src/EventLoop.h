//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_EVENTLOOP_H
#define WEBNGINX_EVENTLOOP_H

#include <functional>
#include <memory>
#include <assert.h>
#include <vector>
#include "Util.h"
#include "../base/MutexLock.h"
#include "../base/CurrentThread.h"
#include "../base/Thread.h"
#include "Epoll.h"
#include "Channel.h"

class EventLoop {
public:
    typedef std::function<void()> Functor;

    EventLoop();

    ~EventLoop();

    void loop();

    void quit();

    void runInLoop(Functor &&cb);

    void queueInLoop(Functor &&cb);

    bool isInLoopThread() const {
        return threadId_ == CurrentThread::tid();
    }

    void assertInLoopThread() const {
        assert(isInLoopThread());
    }

    void shutdown(std::shared_ptr<Channel> channel) {
        shutDownWR(channel->getFd());
    }

    void removeFromPoller(std::shared_ptr<Channel> channel) {
        poller_->del(channel);
    }

    void updatePoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->mod(channel, timeout);
    }

    void addPoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->add(channel, timeout);
    }

private:
    bool looping_;
    std::shared_ptr<Epoll> poller_;
    int wakeupFd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;

    std::vector<Functor> pendingFunctors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    std::shared_ptr<Channel> pwakeupChannel_;

    void wakeup();

    void handleRead();

    void doPendingFunctors();

    void handleConn();
};

#endif //WEBNGINX_EVENTLOOP_H
