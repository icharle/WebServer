//
// Created by Icharle on 2020/5/17.
//

#include "Util.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <regex>
#include <netdb.h>
#include <iostream>

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

int proxySocket(std::string addr, int port) {
    struct sockaddr_in proxyaddr;

    int proxyFd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(port);
    inet_aton(addr.c_str(), &proxyaddr.sin_addr);

    if (connect(proxyFd, (struct sockaddr *) &proxyaddr, sizeof(proxyaddr)) == -1) {
        perror("proxy socket");
        abort();
    }
    return proxyFd;
}

size_t sendn(int fd, std::string &inBuffer) {
    ssize_t nleft = inBuffer.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char *ptr = inBuffer.c_str();
    while (nleft > 0) {
        if ((nwritten = send(fd, ptr, MAX_BUFF, 0)) <= 0) {
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
    return writeSum;
}

size_t recvn(int fd, std::string &outBuffer) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true) {
        char buff[MAX_BUFF];
        if ((nread = recv(fd, buff, MAX_BUFF, 0)) < 0) {
            if (errno == EINTR) {
                continue;
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
        outBuffer += std::string(buff, buff + nread);
    }
    return readSum;
}

void regexUrl(std::string url, std::string &host, int &port, std::string &ip) {
    std::regex exUrl("(\\w+:\\/\\/)([^/:]+)(:\\d*)?([/s/S]*)");
    std::regex exIp("((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}");
    std::cmatch m;
    if (std::regex_match(url.c_str(), m, exUrl)) {
        host = std::string(m[2].first, m[2].second);
        port = std::string(m[3].first, m[3].second).empty() ? 80 : std::stoi(std::string(m[3].first+1, m[3].second));
        if (std::regex_match(host.c_str(), exIp)) {
            // 默认为点分十进制数IP地址格式
            ip = host;
        } else {
            // 转换成点分十进制数IP地址
            struct hostent *hostent = gethostbyname(host.c_str());
            ip = inet_ntoa(*(in_addr *) *hostent->h_addr_list);
        }
    }
}