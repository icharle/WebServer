//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_HTTPDATA_H
#define WEBNGINX_HTTPDATA_H

#include <string>
#include "Timer.h"
#include <memory>
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <map>

class Channel;

class TimerNode;

class EventLoop;

enum HttpVersion {
    HTTP1_0,
    HTTP1_1
};

enum HttpMethod {
    METHOD_GET,
    METHOD_POST
};

enum HttpProcessState {
    PARSE_URI,
    PARSE_HEADERS,
    RECV_BODY,
    ANALYSIS,
    FINISH
};

enum URIState {
    PARSE_URI_AGAIN,
    PARSE_URI_ERROR,
    PARSE_URI_SUCC
};

enum HeaderState {
    PARSE_HEADER_SUCC,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum ParseState {
    H_START,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum ConnectionState {
    H_CONNECTED,
    H_DISCONNECTING,
    H_DISCONNECTED
};

enum AnalysisState {
    ANALYSIS_SUCC,
    ANALYSIS_ERR
};

class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    HttpData(EventLoop *loop, int connfd);

    ~HttpData();

    void reset();

    void seperateTimer();

    void linkTimer(std::shared_ptr<TimerNode> mtimer) {
        timer = mtimer;
    }

    std::shared_ptr<Channel> getChannel() {
        return channel;
    }

    EventLoop *getLoop() {
        return loop;
    }

    void handleClose();

    void newEvent();

private:
    EventLoop *loop;
    int fd;
    std::shared_ptr<Channel> channel;
    HttpMethod method;
    HttpVersion version;
    ConnectionState connectionState;
    HttpProcessState httpProcessState;
    ParseState parseState;
    int nowReadPos;
    bool keepAlive;
    std::string inBuffer;
    std::string outBuffer;
    std::string fileName;
    std::string filePath;
    bool error;
    std::string root;
    const static std::unordered_map<std::string, std::string> MimeType;
    std::map<std::string, std::string> headers;
    std::weak_ptr<TimerNode> timer;

    void handleRead();

    void handleWrite();

    void handleError(int fd, int code, std::string msg);

    void handleConn();

    URIState parseURI();

    HeaderState parseHeaders();

    AnalysisState analysisRequest();

};

#endif //WEBNGINX_HTTPDATA_H
