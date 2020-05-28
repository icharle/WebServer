//
// Created by Icharle on 2020/5/21.
//

#include "Config.h"
#include <stdio.h>
#include "base/cJSON.h"
#include "base/MutexLock.h"
#include <iostream>

Config *Config::instance = nullptr;
MutexLock Config::mutex;

Config::Config() {}

Config *Config::getInstance() {
    if (instance == nullptr) {
        MutexLockGuard lock(mutex);
        if (instance == nullptr) {
            instance = new Config();
            static Recovery recov;
        }
    }
    return instance;
}

void Config::load(const std::string &filePath) {
    FILE *fp = fopen(filePath.c_str(), "r");
    if (fp == NULL) {
        //todo 日志
        abort();
    }
    int i = 0;
    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        configBuf[i++] = ch;
    }
    fclose(fp);
}

std::string Config::getHostRoot(const char *host) {
    cJSON *root = NULL;
    cJSON *hosts = NULL;
    cJSON *item = NULL;
    root = cJSON_Parse(configBuf);

    hosts = cJSON_GetObjectItem(root, "host");
    item = cJSON_GetArrayItem(hosts, 0);
    if (cJSON_HasObjectItem(item, host) == 1) {
        item = cJSON_GetObjectItem(item, host);
    } else {
        // 返回默认站点
        item = cJSON_GetObjectItem(item, "default");
    }
    item = cJSON_GetObjectItem(item, "root");
    return item->valuestring;
}

std::string Config::getPassProxy(const char *host) {
    cJSON *root = NULL;
    cJSON *hosts = NULL;
    cJSON *item = NULL;
    root = cJSON_Parse(configBuf);

    hosts = cJSON_GetObjectItem(root, "host");
    item = cJSON_GetArrayItem(hosts, 0);
    if (cJSON_HasObjectItem(item, host) == 1) {
        item = cJSON_GetObjectItem(item, host);
        if (cJSON_HasObjectItem(item, "proxy_pass") == 1) {
            item = cJSON_GetObjectItem(item, "proxy_pass");
            return item->valuestring;
        }
    } else {
        // 返回默认站点
        item = cJSON_GetObjectItem(item, "default");
        if (cJSON_HasObjectItem(item, "proxy_pass") == 1) {
            item = cJSON_GetObjectItem(item, "proxy_pass");
            return item->valuestring;
        }
    }
    return "";
}

void Config::getServerConfig(int &port, int &threadNum, int &backlog) {
    cJSON *root = NULL;
    cJSON *server = NULL;
    root = cJSON_Parse(configBuf);
    server = cJSON_GetObjectItem(root, "server");
    port = cJSON_GetObjectItem(server, "PORT")->valueint;
    threadNum = cJSON_GetObjectItem(server, "ThreadNum")->valueint;
    backlog = cJSON_GetObjectItem(server, "BackLog")->valueint;
}

Config::~Config() {
}