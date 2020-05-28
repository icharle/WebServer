//
// Created by Icharle on 2020/5/24.
//

#include "FastCgi.h"
#include <string.h>
#include "Util.h"
#include <iostream>
#include <unistd.h>
#include <assert.h>

static const int PARAMS_BUFF_LEN = 2048;  //环境参数buffer的大小
static const int CONTENT_BUFF_LEN = 2048; //内容buffer的大小

FastCgi::FastCgi() :
        socketFd(0),
        requestId(0) {}

FastCgi::~FastCgi() {
    socketFd = 0;
    requestId = 0;
}

FASTCGI_Header FastCgi::makeHeader(int type, int requestId, int contentLength, int paddingLength) {
    FASTCGI_Header header{};
    header.version = FASTCGI_VERSION;
    header.type = (unsigned char) type;

    header.requestIdB1 = (unsigned char) ((requestId >> 8) & 0xff);
    header.requestIdB0 = (unsigned char) (requestId & 0xff);

    header.contentLengthB1 = (unsigned char) ((contentLength >> 8) & 0xff);
    header.contentLengthB0 = (unsigned char) ((contentLength & 0xff));

    header.paddingLength = (unsigned char) paddingLength;

    header.reserved = 0;

    return header;
}

FastCgiBeginRequestBody FastCgi::makeBeginRequestBody(int role, int keepConnection) {

    FastCgiBeginRequestBody body{};

    body.roleB1 = (unsigned char) ((role >> 8) & 0xff);
    body.roleB0 = (unsigned char) (role & 0xff);

    body.flags = (unsigned char) ((keepConnection > 0 ? FASTCGI_KEEP_CONN : 0));
    memset(body.reserved, 0, sizeof(body.reserved));
    return body;
}

void FastCgi::connectFpm() {
    socketFd = proxySocket(DEFAULT_ADDR, DEFAULT_PORT);
}

int FastCgi::sendStartRequestRecord() {
    int ret;
    FastCgiBeginRequestRecord beginRequestRecord{};
    beginRequestRecord.header = makeHeader(FASTCGI_BEGIN_REQUEST, requestId, sizeof(beginRequestRecord.body), 0);
    beginRequestRecord.body = makeBeginRequestBody(FASTCGI_RESPONDER, 0);

    ret = writen(socketFd, (char *) &beginRequestRecord, sizeof(beginRequestRecord));

    if (ret == sizeof(beginRequestRecord)) {
        return 0;
    } else {
        abort();
        return -1;
    }
}

int FastCgi::sendParams(char *name, char *value) {
    int nlen = strlen(name);
    int vlen = strlen(value);
    int ret, bodylen = nlen + vlen;
    (nlen < 128) ? (++bodylen) : (bodylen + 4);
    (vlen < 128) ? (++bodylen) : (bodylen + 4);

    auto *buf = static_cast<unsigned char *>(malloc(bodylen + FASTCGI_HEADER_LENGTH));

    // header
    FASTCGI_Header nameValueHeader = makeHeader(FASTCGI_PARAMS, requestId, bodylen, 0);
    memcpy(buf, (char *) &nameValueHeader, FASTCGI_HEADER_LENGTH);
    // header - end

    // body
    unsigned char bodyBuff[bodylen];
    int j = 0;
    /* 如果 nameLen 小于128字节 */
    if (nlen < 128) {
        bodyBuff[j++] = (unsigned char) nlen; //nameLen用1个字节保存
    } else {
        /* nameLen 用 4 个字节保存 */
        bodyBuff[j++] = (unsigned char) ((nlen >> 24) | 0x80);
        bodyBuff[j++] = (unsigned char) (nlen >> 16);
        bodyBuff[j++] = (unsigned char) (nlen >> 8);
        bodyBuff[j++] = (unsigned char) nlen;
    }

    /* valueLen 小于 128 就用一个字节保存 */
    if (vlen < 128) {
        bodyBuff[j++] = (unsigned char) vlen;
    } else {
        /* valueLen 用 4 个字节保存 */
        bodyBuff[j++] = (unsigned char) ((vlen >> 24) | 0x80);
        bodyBuff[j++] = (unsigned char) (vlen >> 16);
        bodyBuff[j++] = (unsigned char) (vlen >> 8);
        bodyBuff[j++] = (unsigned char) vlen;
    }

    for (size_t i = 0; i < strlen(name); i++) {
        bodyBuff[j++] = name[i];
    }

    for (size_t i = 0; i < strlen(value); i++) {
        bodyBuff[j++] = value[i];
    }
    memcpy(buf + FASTCGI_HEADER_LENGTH, bodyBuff, bodylen);
    // body - end
    std::cout << bodyBuff << std::endl;
    ret = write(socketFd, buf, bodylen + FASTCGI_HEADER_LENGTH);
    assert(ret == bodylen + FASTCGI_HEADER_LENGTH);
    return 1;
}

int FastCgi::sendPostStdinRecord(char *data, int len) {
    int bodylen, ret;

    while (len > 0) {

        if (len > FASTCGI_MAX_LENGTH) {
            bodylen = FASTCGI_MAX_LENGTH;
        }

        FASTCGI_Header header = makeHeader(FASTCGI_STDIN, requestId, bodylen, 0);

        ret = writen(socketFd, (char *) &header, FASTCGI_HEADER_LENGTH);
        if (ret != FASTCGI_HEADER_LENGTH) {
            return -1;
        }

        ret = writen(socketFd, data, bodylen);
        if (ret != bodylen) {
            return -1;
        }

        len -= bodylen;
        data += bodylen;
    }
    return 0;
}

int FastCgi::sendEndPostStdinRecord() {
    int ret;
    FASTCGI_Header header = makeHeader(FASTCGI_STDIN, requestId, 0, 0);
    ret = writen(socketFd, (char *) &header, FASTCGI_HEADER_LENGTH);

    if (ret == FASTCGI_HEADER_LENGTH) {
        return 0;
    } else {
        return -1;
    }
}

int FastCgi::sendEndRequestRecord() {
    int ret;
    FASTCGI_Header endHeader{};
    endHeader = makeHeader(FASTCGI_PARAMS, requestId, 0, 0);

    ret = writen(socketFd, (char *) &endHeader, sizeof(endHeader));
    if (ret == sizeof(endHeader)) {
        return 0;
    } else {
        abort();
        return -1;
    }
}

void FastCgi::recvRecord(char *data) {
    FASTCGI_Header respHeader{};

    int contentLen;
    char paddingBuf[8];

    while (read(socketFd, &respHeader, FASTCGI_HEADER_LENGTH) > 0) {
        if (respHeader.type == FASTCGI_STDOUT) {
            contentLen = (respHeader.contentLengthB1 << 8) + (respHeader.contentLengthB0);
            readn(socketFd, data, contentLen);

            if (respHeader.paddingLength > 0) {
                read(socketFd, paddingBuf, respHeader.paddingLength);
            }
        } else if (respHeader.type == FASTCGI_STDERR) {
            contentLen = (respHeader.contentLengthB1 << 8) + (respHeader.contentLengthB0);
            read(socketFd, data, contentLen);

            if (respHeader.paddingLength > 0) {
                read(socketFd, paddingBuf, respHeader.paddingLength);
            }
        } else if (respHeader.type == FASTCGI_END_REQUEST) {
            FastCgiEndRequestBody endRequestBody{};
            read(socketFd, &endRequestBody, sizeof(endRequestBody));
        }
    }
}