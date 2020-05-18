#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include "EventLoop.h"
#include "WebServer.h"

int main(int argc, char *argv[]) {
    int port = 80;
    int threadNum = 4;
    int backlog = 1024;

    int opt;
    const char *str = "p:t:b:";

    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                threadNum = atoi(optarg);
                break;
            case 'b':
                backlog = atoi(optarg);
                break;
            default:
                break;
        }
    }

    EventLoop loop;
    WebServer webServer(&loop, threadNum, port, backlog);
    webServer.run();
    loop.loop();

    return 0;
}
