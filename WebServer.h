//
// Created by Icharle on 2020/5/15.
//

#ifndef WEBNGINX_WEBSERVER_H
#define WEBNGINX_WEBSERVER_H

class WebServer {
public:
    WebServer(int threadNum_, int port_, int backlog_);

    void run();

    void acceptConnection();

    ~WebServer();

private:
    bool initSocket();

    int setSocketNonBlock(int socketFd);

private:
    int port;

    int backlog;

    int socketFd;

    int threadNum;
};

#endif //WEBNGINX_WEBSERVER_H
