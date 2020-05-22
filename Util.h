//
// Created by Icharle on 2020/5/17.
//

#ifndef WEBNGINX_UTIL_H
#define WEBNGINX_UTIL_H

#include <cstdio>
#include <string>


ssize_t readn(int fd, void *buff, size_t n);

ssize_t readn(int fd, std::string &inBuffer, bool &zero);

ssize_t readn(int fd, std::string &inBuffer);

ssize_t writen(int fd, void *buff, size_t n);

size_t writen(int fd, std::string &sbuff);

void shutDownWR(int fd);

int proxySocket(int port);

#endif //WEBNGINX_UTIL_H
