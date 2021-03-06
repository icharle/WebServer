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

int proxySocket(std::string addr, int port);

size_t sendn(int fd, std::string &inBuffer);

size_t recvn(int fd, std::string &outBuffer);

void regexUrl(std::string url, std::string &host, int &port, std::string &ip);

#endif //WEBNGINX_UTIL_H
