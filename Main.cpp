#include "src/EventLoop.h"
#include "src/WebServer.h"
#include "src/Config.h"

Config *config(Config::getInstance());

int main(int argc, char *argv[]) {
    int port;
    int threadNum;
    int backlog;
    // 读取配置文件
    Config::getInstance()->load("./conf/conf.json");
    Config::getInstance()->getServerConfig(port, threadNum, backlog);

    EventLoop loop;
    WebServer webServer(&loop, threadNum, port, backlog);
    webServer.run();
    loop.loop();

    return 0;
}
