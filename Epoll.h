//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_EPOLL_H
#define WEBNGINX_EPOLL_H

#define MAXEVENTS 1024

#include <sys/epoll.h>
#include "Channel.h"
#include "Timer.h"
#include <vector>

class Epoll {
public:
    Epoll();

    void add(shareChannel request, int timeout);

    void mod(shareChannel request, int timeout);

    void del(shareChannel request);

    std::vector<shareChannel> poll();

    std::vector<shareChannel> getEventsRequest(int events_num);

    void add_timer(shareChannel request_data, int timeout);

    int getEpollFd() {
        return epollFd;
    }

    void handleExpired();

    ~Epoll();

private:
    int epollFd;
    std::vector<epoll_event> events;
    std::shared_ptr<Channel> fd2chan[MAXEVENTS];
    std::shared_ptr<HttpData> fd2http[MAXEVENTS];
    TimerManager timerManager;
};

#endif //WEBNGINX_EPOLL_H
