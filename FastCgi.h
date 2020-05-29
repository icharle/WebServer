//
// Created by Icharle on 2020/5/24.
// https://blog.csdn.net/shreck66/article/details/50355729
//
#include <string>
#ifndef WEBNGINX_FASTCGI_H
#define WEBNGINX_FASTCGI_H

// 默认地址
#define DEFAULT_ADDR "127.0.0.1"
// 端口
#define DEFAULT_PORT 9000

struct FASTCGI_Header {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
};

// 允许传送的最大数据 65536
#define FASTCGI_MAX_LENGTH 0xffff
// fast_cgi header长度
#define FASTCGI_HEADER_LENGTH 8
// fast_cgi 版本
#define FASTCGI_VERSION 1

enum TypeState {
    FASTCGI_BEGIN_REQUEST = 1,
    FASTCGI_ABORT_REQUEST,
    FASTCGI_END_REQUEST,
    FASTCGI_PARAMS,
    FASTCGI_STDIN,
    FASTCGI_STDOUT,
    FASTCGI_STDERR,
    FASTCGI_DATA,
    FASTCGI_GET_VALUES,
    FASTCGI_GET_VALUES_RESULT,
    FASTCGI_UNKNOWN_TYPES
};

enum RoleState {
    FASTCGI_KEEP_CONN = 1,
    FASTCGI_RESPONDER = 1,
    FASTCGI_AUTHORIZER,
    FASTCGI_FILTER
};

enum EndState {
    FASTCGI_REQUEST_COMPLETE,
    FASTCGI_CANT_MPX_XONN,
    FASTCGI_OVERLOADED,
    FASTCGI_UNKNOWN_ROLE
};

struct FastCgiBeginRequestBody {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
};

struct FastCgiBeginRequestRecord {
    FASTCGI_Header header;
    FastCgiBeginRequestBody body;
};

struct FastCgiEndRequestBody {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
};

struct FastCgiEndRequestRecord {
    FASTCGI_Header header;
    FastCgiEndRequestBody body;
};

class FastCgi {
public:
    FastCgi();

    ~FastCgi();

    FASTCGI_Header makeHeader(int type, int requestId, int contentLength, int paddingLength);

    FastCgiBeginRequestBody makeBeginRequestBody(int role, int keepConnection);

    void setRequestId(int requestId_) {
        requestId = requestId_;
    }

    void connectFpm();

    int sendStartRequestRecord();

    int sendParams(char *name, char *value);

    int sendPostStdinRecord(char *data, int len);

    int sendEndPostStdinRecord();

    int sendEndRequestRecord();

    void recvRecord(char *data);

    void headerAndContent(char *p, std::string &header, std::string &body);

private:
    int socketFd;
    int requestId;
};

#endif //WEBNGINX_FASTCGI_H
