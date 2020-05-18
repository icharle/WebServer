//
// Created by Icharle on 2020/5/15.
//

#include "Epoll.h"
#include <assert.h>

#define MAXEVENTS 1024
#define EPOLLWAIT_TIME 10000
typedef std::shared_ptr<Channel> shareChannel;

Epoll::Epoll() :
        epollFd(epoll_create(MAXEVENTS)),
        events(MAXEVENTS) {
    assert(epollFd > 0);
}

Epoll::~Epoll() {

}

void Epoll::add(shareChannel request, int timeout) {
    int fd = request->getFd();
    if (timeout > 0) {
        add_timer(request, timeout);
        fd2http[fd] = request->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    request->EqualAndUpdateLastEvents();

    fd2chan[fd] = request;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
        fd2chan[fd].reset();
    }
}

void Epoll::mod(shareChannel request, int timeout) {
    int fd = request->getFd();
    if (timeout > 0) {
        add_timer(request, timeout);
    }
    if (!request->EqualAndUpdateLastEvents()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) < 0) {
            fd2chan[fd].reset();
        }
    }
}

void Epoll::del(shareChannel request) {
    int fd = request->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event) < 0) {
        // todo 日志
    }
    fd2chan[fd].reset();
    fd2http[fd].reset();
}

std::vector<shareChannel> Epoll::poll() {
    while (true) {
        int event_count = epoll_wait(epollFd, &*events.begin(), events.size(), EPOLLWAIT_TIME);
        if (event_count < 0) {
            // todo 日志
        }
        std::vector<shareChannel> req_data = getEventsRequest(event_count);;
        if (!req_data.empty()) {
            return req_data;
        }
    }
}

void Epoll::handleExpired() {
    timerManager.handleExpiredEvent();
}

void Epoll::add_timer(shareChannel request_data, int timeout) {
    std::shared_ptr<HttpData> t = request_data->getHolder();
    if (t) {
        timerManager.addTimer(t, timeout);
    } else {
        //todo 日志
    }
}

std::vector<shareChannel> Epoll::getEventsRequest(int events_num) {
    std::vector<shareChannel> req_data;
    for (int i = 0; i < events_num; ++i) {
        int fd = events[i].data.fd;
        shareChannel cur_req = fd2chan[fd];
        if (cur_req) {
            cur_req->setRevents(events[i].events);
            cur_req->setEvents(0);

            req_data.push_back(cur_req);
        } else {
            // todo 日志
        }
    }
    return req_data;
}