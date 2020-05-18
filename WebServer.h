//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_WEBSERVER_H
#define WEBNGINX_WEBSERVER_H

#include "EventLoop.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"
#include <memory>

class WebServer {
public:
    WebServer(EventLoop *loop_, int threadNum_, int port_, int backlog_);

    void run();

    void acceptConnection();

    void handleThisConn();

    ~WebServer();

private:
    bool initSocket();

    int setSocketNonBlock(int socketFd);

private:
    EventLoop *loop;
    int port;
    int backlog;
    int threadNum;
    std::shared_ptr<EventLoopThreadPool> eventLoopThreadPool;
    std::shared_ptr<Channel> acceptChannel;
    int socketFd;
};

#endif //WEBNGINX_WEBSERVER_H
