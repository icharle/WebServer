//
// Created by Icharle on 2020/5/15.
//

#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <queue>
#include <cstdio>

Channel::Channel(EventLoop *loop_) :
        loop(loop_),
        events(0),
        lastEvents(0) {
}

Channel::Channel(EventLoop *loop_, int fd_) :
        loop(loop_),
        fd(fd_),
        events(0),
        lastEvents(0) {
}

int Channel::getFd() {
    return fd;
}

void Channel::setFd(int fd_) {
    fd = fd_;
}

void Channel::setHolder(std::shared_ptr<HttpData> holder_) {
    holder = holder_;
}

void Channel::setReadHandler(CallBack &&readHandler_) {
    readHandler = readHandler_;
}

void Channel::setWriteHandler(CallBack &&writeHandler_) {
    writeHandler = writeHandler_;
}

void Channel::setErrorHandler(CallBack &&errorHandler_) {
    errorHandler = errorHandler_;
}

void Channel::handleEvents() {
    events = 0;
    if ((revents & EPOLLHUP) && !(revents & EPOLLIN)) {
        events = 0;
        return;
    }
    if (revents & EPOLLERR) {
        if (errorHandler) {
            errorHandler();
        }
        events = 0;
        return;
    }
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        handleRead();
    }
    if (revents & EPOLLOUT) {
        handleWrite();
    }
    handleConn();
}

void Channel::setConnHandler(CallBack &&connHandler_) {
    connHandler = connHandler_;
}

void Channel::handleRead() {
    if (readHandler) {
        readHandler();
    }
}

void Channel::handleWrite() {
    if (writeHandler) {
        writeHandler();
    }
}

void Channel::handleConn() {
    if (connHandler) {
        connHandler();
    }
}

Channel::~Channel() = default;