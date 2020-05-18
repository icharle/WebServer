//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_CHANNEL_H
#define WEBNGINX_CHANNEL_H

#include <functional>
#include <sys/epoll.h>
#include <memory>

class EventLoop;

class HttpData;

class Channel {
private:
    // 回调函数
    typedef std::function<void()> CallBack;
    EventLoop *loop;
    int fd;
    __uint32_t events;
    __uint32_t revents;
    __uint32_t lastEvents;
    // channel持有者
    std::weak_ptr<HttpData> holder;

public:
    Channel(EventLoop *loop_);

    Channel(EventLoop *loop_, int fd_);

    ~Channel();

    int getFd();

    void setFd(int fd_);

    void setHolder(std::shared_ptr<HttpData> holder_);

    std::shared_ptr<HttpData> getHolder() {
        std::shared_ptr<HttpData> res(holder.lock());
        return res;
    }

    void setReadHandler(CallBack &&readHandler_);

    void setWriteHandler(CallBack &&writeHandler_);

    void setErrorHandler(CallBack &&errorHandler_);

    void setConnHandler(CallBack &&connHandler_);

    // 读 写 异常 连接事件分发处理
    void handleEvents();

    void setRevents(__uint32_t ev) {
        revents = ev;
    }

    void setEvents(__uint32_t ev) {
        events = ev;
    }

    uint32_t &getEvents() {
        return events;
    }

    __uint32_t getLastEvents() {
        return lastEvents;
    }

    bool EqualAndUpdateLastEvents() {
        bool res = (lastEvents == events);
        lastEvents = events;
        return res;
    }

    void handleRead();

    void handleWrite();

    void handleError(int fd, int code, std::string msg);

    void handleConn();

private:
    // 处理请求行
    int parse_URI();

    // 处理请求头
    int parse_Headers();

    // 读取具体文件
    int analysisRequest();

    CallBack readHandler;
    CallBack writeHandler;
    CallBack errorHandler;
    CallBack connHandler;
};

typedef std::shared_ptr<Channel> shareChannel;

#endif //WEBNGINX_CHANNEL_H
