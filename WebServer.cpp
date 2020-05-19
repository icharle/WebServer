//
// Created by Icharle on 2020/5/15.
//

#include "WebServer.h"
#include "HttpData.h"
#include <functional>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

WebServer::WebServer(EventLoop *loop_, int threadNum_, int port_, int backlog_) :
        loop(loop_),
        port(port_),
        backlog(backlog_),
        threadNum(threadNum_),
        eventLoopThreadPool(new EventLoopThreadPool(loop_, threadNum_)),
        acceptChannel(new Channel(loop_)) {
    if (!initSocket()) {
        // todo 日志
        abort();
    }
    acceptChannel->setFd(socketFd);
}

void WebServer::run() {
    std::cout << "run" << std::endl;
    eventLoopThreadPool->start();
    acceptChannel->setEvents(EPOLLIN | EPOLLET);
    acceptChannel->setReadHandler(std::bind(&WebServer::acceptConnection, this));
    acceptChannel->setConnHandler(std::bind(&WebServer::handleThisConn, this));
    loop->addPoller(acceptChannel, 0);
}

void WebServer::acceptConnection() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addrlen = sizeof(client_addr);
    int connect_fd = 0;
    while ((connect_fd = accept(socketFd, (struct sockaddr *) &client_addr, &client_addrlen)) > 0) {
        EventLoop *loop_ = eventLoopThreadPool->getNextLoop();
        if (setSocketNonBlock(connect_fd) < 0) {
            // todo 日志
            return;
        }
        std::shared_ptr<HttpData> req_info(new HttpData(loop_, socketFd));
        req_info->getChannel()->setHolder(req_info);
        loop_->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    }
    acceptChannel->setEvents(EPOLLIN | EPOLLET);
    std::cout << "acceptConnection" << std::endl;
}

bool WebServer::initSocket() {
    struct sockaddr_in sockaddr;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        //todo 日志
        return false;
    }
    // 端口复用
    int opt = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt));
    // 地址端口
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htons(INADDR_ANY);
    sockaddr.sin_port = htons(port);
    // 设置非阻塞
    if (setSocketNonBlock(socketFd) == -1) {
        // todo 日志
        return false;
    }
    if (bind(socketFd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) == -1) {
        //todo 日志
        return false;
    }
    if (listen(socketFd, backlog) == -1) {
        // todo 日志
        return false;
    }
    return true;
}

int WebServer::setSocketNonBlock(int socketFd) {
    int flag = fcntl(socketFd, F_GETFL, 0);
    if (flag == -1) {
        return -1;
    }
    flag |= O_NONBLOCK;
    if (fcntl(socketFd, F_SETFL, flag) < 0) {
        return -1;
    }
    return 0;
}

void WebServer::handleThisConn() {
    loop->updatePoller(acceptChannel);
}

WebServer::~WebServer() = default;