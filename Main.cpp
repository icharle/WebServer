#include "EventLoop.h"
#include "WebServer.h"
#include "Util.h"

int main(int argc, char *argv[]) {
    int port;
    int threadNum;
    int backlog;
    char configBuf[4096] = {0};
    // 读取配置文件
    readConfig(configBuf);
    getServerConfig(configBuf, port, threadNum, backlog);

    EventLoop loop;
    WebServer webServer(&loop, threadNum, port, backlog);
    webServer.run();
    loop.loop();

    return 0;
}
