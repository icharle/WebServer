//
// Created by Icharle on 2020/5/21.
//

#ifndef WEBNGINX_CONFIG_H
#define WEBNGINX_CONFIG_H

#include "base/MutexLock.h"
#include <string>

class Config {
private:
    static MutexLock mutex;
    static Config *instance;

    Config();

    class Recovery {
    public:
        ~Recovery() {
            if (instance != nullptr) {
                delete instance;
            }
        }
    };

    ~Config();

public:
    static Config *getInstance();

    void load(const std::string &filePath);

    std::string getHostRoot(const char *host);

    std::string getPassProxy(const char *host);

    void getServerConfig(int &port, int &threadNum, int &backlog);

public:
    char configBuf[4096]{};
};

#endif //WEBNGINX_CONFIG_H
