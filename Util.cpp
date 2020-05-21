//
// Created by Icharle on 2020/5/17.
//

#include "Util.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include "base/cJSON.h"

#define MAX_BUFF 4096

ssize_t readn(int fd, void *buff, size_t n) {
    ssize_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;

    char *ptr = (char *) buff;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else if (errno == EAGAIN) {
                return readSum;
            } else {
                perror("read error");
                return -1;
            }
        } else if (nread == 0) {
            break;
        }
        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}

ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true) {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                return readSum;
            } else {
                perror("read error");
                return -1;
            }
        } else if (nread == 0) {
            zero = true;
            break;
        }
        readSum += nread;
        inBuffer += std::string(buff, buff + nread);
    }
    return readSum;
}

ssize_t readn(int fd, std::string &inBuffer) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true) {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else if (errno == EAGAIN) {
                return readSum;
            } else {
                perror("read error");
                return -1;
            }
        } else if (nread == 0) {
            break;
        }
        readSum += nread;
        inBuffer += std::string(buff, buff + nread);
    }
    return readSum;
}

ssize_t writen(int fd, void *buff, size_t n) {
    ssize_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char *) buff;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR) {
                    nwritten = 0;
                } else if (errno == EAGAIN) {
                    return writeSum;
                } else {
                    return -1;
                }
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writeSum;
}

size_t writen(int fd, std::string &sbuff) {
    ssize_t nleft = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;

    const char *ptr = sbuff.c_str();
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR) {
                    nwritten = 0;
                } else if (errno == EAGAIN) {
                    break;
                } else {
                    return -1;
                }
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    if (writeSum == static_cast<int>(sbuff.size())) {
        sbuff.clear();
    } else {
        sbuff = sbuff.substr(writeSum);
    }
    return writeSum;
}

void shutDownWR(int fd) {
    shutdown(fd, SHUT_WR);
}

void readConfig(char *buf) {
    FILE *fp = fopen("./conf/conf.json", "r");
    if (fp == NULL) {
        //todo 日志
        abort();
    }
    int i = 0;
    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        buf[i++] = ch;
    }
    fclose(fp);
}

void getServerConfig(char *buf, int &port, int &threadNum, int &backlog) {
    cJSON *root = NULL;
    cJSON *server = NULL;
    root = cJSON_Parse(buf);
    server = cJSON_GetObjectItem(root, "server");
    port = cJSON_GetObjectItem(server, "PORT")->valueint;
    threadNum = cJSON_GetObjectItem(server, "ThreadNum")->valueint;
    backlog = cJSON_GetObjectItem(server, "BackLog")->valueint;
}

std::string getHostRoot(char *buf, const char *host) {
    cJSON *root = NULL;
    cJSON *hosts = NULL;
    cJSON *item = NULL;
    root = cJSON_Parse(buf);

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