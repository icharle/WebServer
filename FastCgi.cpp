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

//int FastCgi::sendParams(char *name, char *value) {
//    int nlen = strlen(name);
//    int vlen = strlen(value);
//    unsigned char *buf;
//    int ret, bodylen = nlen + vlen;
//    (nlen < 128) ? (++bodylen) : (bodylen + 4);
//    (vlen < 128) ? (++bodylen) : (bodylen + 4);
//
//    buf = static_cast<unsigned char *>(malloc(bodylen + FASTCGI_HEADER_LENGTH));
//
//    std::cout << bodylen << std::endl;
//
//    // header
//    FASTCGI_Header nameValueHeader = makeHeader(FASTCGI_PARAMS, requestId, bodylen, 0);
//    memcpy(buf, (char *) &nameValueHeader, FASTCGI_HEADER_LENGTH);
//    buf = buf + FASTCGI_HEADER_LENGTH;
//    // header - end
//
//    // body
//    /* 如果 nameLen 小于128字节 */
//    if (nlen < 128) {
//        *buf++ = (unsigned char) nlen; //nameLen用1个字节保存
//    } else {
//        /* nameLen 用 4 个字节保存 */
//        *buf++ = (unsigned char) ((nlen >> 24) | 0x80);
//        *buf++ = (unsigned char) (nlen >> 16);
//        *buf++ = (unsigned char) (nlen >> 8);
//        *buf++ = (unsigned char) nlen;
//    }
//
//    /* valueLen 小于 128 就用一个字节保存 */
//    if (vlen < 128) {
//        *buf++ = (unsigned char) vlen;
//    } else {
//        /* valueLen 用 4 个字节保存 */
//        *buf++ = (unsigned char) ((vlen >> 24) | 0x80);
//        *buf++ = (unsigned char) (vlen >> 16);
//        *buf++ = (unsigned char) (vlen >> 8);
//        *buf++ = (unsigned char) vlen;
//    }
//
//    memcpy(buf, name, nlen);
//    buf = buf + nlen;
//    memcpy(buf, value, vlen);
//    // body - end
//
//    ret = writen(socketFd, buf, FASTCGI_HEADER_LENGTH + bodylen);
//    if (ret == FASTCGI_HEADER_LENGTH + bodylen) {
//        return 0;
//    } else {
//        std::cout << "sendParams" << std::endl;
//        abort();
//        return -1;
//    }
//}

int FastCgi::makeNameValueBody(char *name, int nameLen, char *value, int valueLen, unsigned char *bodyBuffPtr,
                               int *bodyLenPtr) {
    /* 记录 body 的开始位置 */
    unsigned char *startBodyBuffPtr = bodyBuffPtr;

    /* 如果 nameLen 小于128字节 */
    if (nameLen < 128) {
        *bodyBuffPtr++ = (unsigned char) nameLen; //nameLen用1个字节保存
    } else {
        /* nameLen 用 4 个字节保存 */
        *bodyBuffPtr++ = (unsigned char) ((nameLen >> 24) | 0x80);
        *bodyBuffPtr++ = (unsigned char) (nameLen >> 16);
        *bodyBuffPtr++ = (unsigned char) (nameLen >> 8);
        *bodyBuffPtr++ = (unsigned char) nameLen;
    }

    /* valueLen 小于 128 就用一个字节保存 */
    if (valueLen < 128) {
        *bodyBuffPtr++ = (unsigned char) valueLen;
    } else {
        /* valueLen 用 4 个字节保存 */
        *bodyBuffPtr++ = (unsigned char) ((valueLen >> 24) | 0x80);
        *bodyBuffPtr++ = (unsigned char) (valueLen >> 16);
        *bodyBuffPtr++ = (unsigned char) (valueLen >> 8);
        *bodyBuffPtr++ = (unsigned char) valueLen;
    }

    /* 将 name 中的字节逐一加入body中的buffer中 */
    for (int i = 0; i < strlen(name); i++) {
        *bodyBuffPtr++ = name[i];
    }

    /* 将 value 中的值逐一加入body中的buffer中 */
    for (int i = 0; i < strlen(value); i++) {
        *bodyBuffPtr++ = value[i];
    }

    /* 计算出 body 的长度 */
    *bodyLenPtr = bodyBuffPtr - startBodyBuffPtr;
    return 1;
}

int FastCgi::sendParams(char *name, char *value) {
    int rc;

    unsigned char bodyBuff[PARAMS_BUFF_LEN];

    bzero(bodyBuff, sizeof(bodyBuff));

    /* 保存 body 的长度 */
    int bodyLen;

    /* 生成 PARAMS 参数内容的 body */
    makeNameValueBody(name, strlen(name), value, strlen(value), bodyBuff, &bodyLen);

    FASTCGI_Header nameValueHeader;
    nameValueHeader = makeHeader(FASTCGI_PARAMS, requestId, bodyLen, 0);
    /*8 字节的消息头*/

    int nameValueRecordLen = bodyLen + FASTCGI_HEADER_LENGTH;
    char nameValueRecord[nameValueRecordLen];

    /* 将头和body拷贝到一块buffer 中只需调用一次write */
    memcpy(nameValueRecord, (char *) &nameValueHeader, FASTCGI_HEADER_LENGTH);
    memcpy(nameValueRecord + FASTCGI_HEADER_LENGTH, bodyBuff, bodyLen);

    rc = write(socketFd, nameValueRecord, nameValueRecordLen);
    assert(rc == nameValueRecordLen);

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

char *FastCgi::recvRecord() {
    FASTCGI_Header respHeader{};

    int contentLen;
    char content[2048];
    char paddingBuf[8];

    while (read(socketFd, &respHeader, FASTCGI_HEADER_LENGTH) > 0) {
        if (respHeader.type == FASTCGI_STDOUT) {
            contentLen = (respHeader.contentLengthB1 << 8) + (respHeader.contentLengthB0);
            memset(content, 0, 2048);

            read(socketFd, content, contentLen);

            if (respHeader.paddingLength > 0) {
                read(socketFd, paddingBuf, respHeader.paddingLength);
            }
        } else if (respHeader.type == FASTCGI_STDERR) {
            contentLen = (respHeader.contentLengthB1 << 8) + (respHeader.contentLengthB0);
            memset(content, 0, contentLen);

            read(socketFd, content, contentLen);

            if (respHeader.paddingLength > 0) {
                read(socketFd, paddingBuf, respHeader.paddingLength);
            }
        } else if (respHeader.type == FASTCGI_END_REQUEST) {
            FastCgiEndRequestBody endRequestBody{};
            read(socketFd, &endRequestBody, sizeof(endRequestBody));
        }
    }
    return content;
}