//
// Created by Icharle on 2020/5/15.
//

#include "HttpData.h"
#include <cstring>
#include "Channel.h"
#include "EventLoop.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "time.h"
#include "Util.h"
#include <iostream>
#include <stdio.h>
#include "Config.h"
#include <sys/socket.h>

extern Config *config;
const __uint32_t DEFALT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000; // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms 5min

const std::unordered_map<std::string, std::string> HttpData::MimeType = {
        {".html",  "text/html"},
        {".css",   "text/css"},
        {".xml",   "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt",   "text/plain"},
        {".png",   "image/png"},
        {".gif",   "image/gif"},
        {".jpg",   "image/jpeg"},
        {".jpeg",  "image/jpeg"},
        {".txt",   "text/plain"},
        {".rtf",   "application/rtf"},
        {".pdf",   "application/pdf"},
        {".word",  "application/nsword"},
        {".au",    "audio/basic"},
        {".mpeg",  "video/mpeg"},
        {".mpg",   "video/mpeg"},
        {".avi",   "video/x-msvideo"},
        {".gz",    "application/x-gzip"},
        {".tar",   "application/x-tar"},
};

HttpData::HttpData(EventLoop *loop, int connfd) :
        loop(loop),
        fastCgi(new FastCgi()),
        fd(connfd),
        channel(new Channel(loop, connfd)),
        method(METHOD_GET),
        version(HTTP1_1),
        connectionState(H_CONNECTED),
        httpProcessState(PARSE_URI),
        parseState(H_START),
        nowReadPos(0),
        keepAlive(false),
        isPHP(false),
        error(false) {
    channel->setReadHandler(std::bind(&HttpData::handleRead, this));
    channel->setWriteHandler(std::bind(&HttpData::handleWrite, this));
    channel->setConnHandler(std::bind(&HttpData::handleConn, this));
}

HttpData::~HttpData() {
    close(fd);
}

void HttpData::reset() {
    fileName.clear();
    root.clear();
    nowReadPos = 0;
    httpProcessState = PARSE_URI;
    parseState = H_START;
    headers.clear();
    if (timer.lock()) {
        std::shared_ptr<TimerNode> m_timer(timer.lock());
        m_timer->clearReq();
        timer.reset();
    }
}

void HttpData::seperateTimer() {
    if (timer.lock()) {
        std::shared_ptr<TimerNode> m_timer(timer.lock());
        m_timer->clearReq();
        timer.reset();
    }
}

HeaderState HttpData::parseHeaders() {
    std::string &str = inBuffer;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;

    for (; i < str.size() && notFinish; i++) {
        switch (parseState) {
            case H_START: {
                if (str[i] == '\n' || str[i] == '\r') {
                    break;
                }
                parseState = H_KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case H_KEY: {
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                    parseState = H_COLON;
                } else if (str[i] == '\n' || str[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_COLON: {
                if (str[i] == ' ') {
                    parseState = H_SPACES_AFTER_COLON;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_SPACES_AFTER_COLON: {
                parseState = H_VALUE;
                value_start = i;
                break;
            }
            case H_VALUE: {
                if (str[i] == '\r') {
                    parseState = H_CR;
                    value_end = i;
                    if (value_end - value_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                } else if (i - value_start > 255) {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_CR: {
                if (str[i] == '\n') {
                    parseState = H_LF;
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_LF: {
                if (str[i] == '\r') {
                    parseState = H_END_CR;
                } else {
                    key_start = i;
                    parseState = H_KEY;
                }
                break;
            }
            case H_END_CR: {
                if (str[i] == '\n') {
                    parseState = H_END_LF;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_END_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if (parseState == H_END_LF) {
        // 解决x-www-form-urlencoded方式中截取错误问题
        if (method == METHOD_POST) {
            str = str.substr(now_read_line_begin);
        } else {
            str = str.substr(i);
        }
        return PARSE_HEADER_SUCC;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

void HttpData::handleRead() {
    __uint32_t &events = channel->getEvents();
    do {
        bool zero = false;
        int read_num = readn(fd, inBuffer, zero);
        std::cout << "\nRequest: \n" << inBuffer << std::endl;
        if (connectionState == H_DISCONNECTING) {
            inBuffer.clear();
            break;
        }
        if (read_num < 0) {
            perror("handleRead: 1");
            error = true;
            handleError(fd, 400, "Bad Request");
            break;
        } else if (zero) {
            connectionState = H_DISCONNECTING;
            if (read_num == 0) {
                break;
            }
        }

        if (httpProcessState == PARSE_URI) {
            URIState flag = this->parseURI();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("handleRead: 2");
                inBuffer.clear();
                error = true;
                handleError(fd, 400, "Bad Request");
                break;
            } else {
                httpProcessState = PARSE_HEADERS;
            }
        }

        if (httpProcessState == PARSE_HEADERS) {
            HeaderState flag = this->parseHeaders();
            if (flag == PARSE_HEADER_AGAIN) {
                break;
            } else if (flag == PARSE_HEADER_ERROR) {
                perror("handleRead: 3");
                error = true;
                handleError(fd, 400, "Bad Request");
                break;
            }
            if (method == METHOD_POST) {
                httpProcessState = RECV_BODY;
            } else {
                httpProcessState = ANALYSIS;
            }
            // 处理root目录 反向代理URL
            if (headers.find("Host") != headers.end()) {
                root = config->getHostRoot(headers["Host"].c_str());
                passProxy = config->getPassProxy(headers["Host"].c_str());
            } else {
                root = config->getHostRoot("default");
                passProxy = config->getPassProxy("default");
            }
        }

        if (httpProcessState == RECV_BODY) {
            int content_length = -1;
            if (headers.find("Content-Length") != headers.end()) {
                content_length = stoi(headers["Content-Length"]);
            } else {
                error = true;
                handleError(fd, 400, "Bad Request: lost of Content-length");
                break;
            }
            if (static_cast<int >(inBuffer.size()) < content_length) {
                break;
            }
            httpProcessState = ANALYSIS;
        }

        if (httpProcessState == ANALYSIS) {
            AnalysisState flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCC) {
                httpProcessState = FINISH;
                break;
            } else {
                error = true;
                break;
            }
        }
    } while (false);

    if (!error) {
        if (outBuffer.size() > 0) {
            handleWrite();
        }
        if (!error && httpProcessState == FINISH) {
            this->reset();
            if (inBuffer.size() > 0) {
                if (connectionState != H_DISCONNECTING) {
                    handleRead();
                }
            }
        } else if (!error && connectionState != H_DISCONNECTED) {
            events |= EPOLLIN;
        }
    }
}

void HttpData::handleWrite() {
    if (!error && connectionState != H_DISCONNECTED) {
        __uint32_t &events = channel->getEvents();
        if (writen(fd, outBuffer) < 0) {
            perror("handleWrite: writen");
            // todo 日志
            events = 0;
            error = true;
        }
        if (!outBuffer.empty()) {
            events |= EPOLLOUT;
        }
    }
}


void HttpData::handleConn() {
    seperateTimer();
    __uint32_t &events = channel->getEvents();
    if (!error && connectionState == H_CONNECTED) {
        if (events != 0) {
            int timeout = DEFAULT_EXPIRED_TIME;
            if (keepAlive) {
                timeout = DEFAULT_KEEP_ALIVE_TIME;
            }
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = __uint32_t(0);
                events |= EPOLLOUT;
            }
            events |= EPOLLET;
            loop->updatePoller(channel, timeout);
        } else if (keepAlive) {
            events |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            loop->updatePoller(channel, timeout);
        } else {
            events |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            loop->updatePoller(channel, timeout);
        }
    } else if (!error && connectionState == H_DISCONNECTING && (events & EPOLLOUT)) {
        events = (EPOLLOUT | EPOLLET);
    } else {
        loop->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
    }
}

void HttpData::handleError(int fd, int code, std::string msg) {
    std::string header_buff, body_buff;
    char output[4 * 1024];
    // body
    body_buff += "<html><title>Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(code) + " : " + msg + "\n";
    body_buff += "<p>" + msg + "</p>";
    body_buff += "<hr><em>WebServer</em></body></html>";

    // header
    header_buff += "HTTP/1.1 " + std::to_string(code) + " " + msg + "\r\n";
    header_buff += "Server: WebServer\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + std::to_string(body_buff.size()) + "\r\n\r\n";

    sprintf(output, "%s", header_buff.c_str());
    writen(fd, output, strlen(output));
    sprintf(output, "%s", body_buff.c_str());
    writen(fd, output, strlen(output));
}

URIState HttpData::parseURI() {
    std::string &str = inBuffer;
    std::string cop = str;
    size_t pos = str.find('\r', nowReadPos);

    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    std::string request_line = str.substr(0, pos);
    if (str.size() > pos + 1) {
        str = str.substr(pos + 1);
    } else {
        str.clear();
    }
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");

    if (posGet >= 0) {
        pos = posGet;
        method = METHOD_GET;
    } else if (posPost >= 0) {
        pos = posPost;
        method = METHOD_POST;
    } else {
        return PARSE_URI_ERROR;
    }

    pos = request_line.find("/", pos);
    if (pos < 0) {
        fileName = "index.html";
        version = HTTP1_1;
        return PARSE_URI_SUCC;
    } else {
        size_t pos_ = request_line.find(' ', pos);
        if (pos_ < 0) {
            return PARSE_URI_ERROR;
        } else {
            if (pos_ - pos > 1) {
                fileName = request_line.substr(pos + 1, pos_ - pos - 1);
                size_t pos__ = fileName.find("?");
                if (pos__ != std::string::npos) {
                    query = fileName.substr(pos__ + 1, pos_);
                    fileName = fileName.substr(0, pos__);
                }
            } else {
                fileName = "index.html";
            }
        }
        pos = pos_;
    }
    // 是否需要php-fpm处理
    auto idx = fileName.find_last_of(".");
    if (idx != std::string::npos) {
        isPHP = fileName.substr(idx) == ".php";
    }

    pos = request_line.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if (request_line.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            std::string version_ = request_line.substr(pos + 1, 3);
            if (version_ == "1.0") {
                version = HTTP1_0;
            } else if (version_ == "1.1") {
                version = HTTP1_1;
            } else {
                return PARSE_URI_ERROR;
            }
        }
    }
    return PARSE_URI_SUCC;
}

AnalysisState HttpData::analysisRequest() {
    if (!passProxy.empty()) {
        // 处理反向代理 分离IP、Port、host
        int port;
        std::string host;
        std::string ip;
        regexUrl(passProxy, host, port, ip);
        // 反向代理
        int proxyFd = proxySocket(ip, port);
        std::string header_buff;
        header_buff +=
                (method == METHOD_GET ? "GET /" : "POST /") + fileName +
                (version == HTTP1_1 ? " HTTP/1.1" : " HTTP/1.0") +
                "\r\n";
        header_buff += "Host: " + host + "\r\n";
        header_buff += "User-Agent: " + headers["User-Agent"] + "\r\n";
        header_buff += "Content-Length: " + headers["Content-Length"] + "\r\n";
        header_buff += "Content-Type: " + headers["Content-Type"] + "\r\n";
        header_buff += "\r\n";
        sendn(proxyFd, header_buff);
        recvn(proxyFd, outBuffer);
        return ANALYSIS_SUCC;
    }
    // 判断文件是否存在
    fileName = root + fileName;
    struct stat sbuf;
    if (stat(fileName.c_str(), &sbuf) < 0) {
        handleError(fd, 404, "NOT FOUND!");
        return ANALYSIS_ERR;
    }
    std::string header, body, filetype;
    char *data = static_cast<char *>(malloc(2048));
    // 反向代理 or 负载均衡
    // php-fpm动态处理 or 静态资源文件
    if (method == METHOD_POST) {
        fastCgi->setRequestId(1);
        fastCgi->connectFpm();
        fastCgi->sendStartRequestRecord();
        fastCgi->sendParams(const_cast<char *>("SCRIPT_FILENAME"), const_cast<char *>(fileName.c_str()));
        fastCgi->sendParams(const_cast<char *>("REQUEST_METHOD"), const_cast<char *>("POST"));
        fastCgi->sendParams(const_cast<char *>("CONTENT_LENGTH"),
                            const_cast<char *>(headers["Content-Length"].c_str()));

        fastCgi->sendParams(const_cast<char *>("CONTENT_TYPE"),
                            const_cast<char *>(headers["Content-Type"].c_str()));
        fastCgi->sendEndRequestRecord();
        fastCgi->sendPostStdinRecord(const_cast<char *>(inBuffer.c_str()), stoi(headers["Content-Length"]));
        fastCgi->sendEndPostStdinRecord();
        fastCgi->recvRecord(data);
        body = data;
        delete [] data;
        header += "HTTP/1.1 200 OK\r\n";
        if (headers.find("Connection") != headers.end() &&
            (headers["Connection"] == "Keep-Alive" || headers["Connection"] == "keep-alive")) {
            keepAlive = true;
            header += std::string("Connection: Keep-Alive\r\n");
            header += "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        header += "Content-type: text/html\r\n";
        header += "Content-length: " + std::to_string(body.size()) + "\r\n";
        header += "Server: WebServer\r\n";
        header += "\r\n";
        outBuffer += header;
        outBuffer += body;
        return ANALYSIS_SUCC;
    } else if (method == METHOD_GET) {
        if (isPHP) {
            fastCgi->setRequestId(1);
            fastCgi->connectFpm();
            fastCgi->sendStartRequestRecord();
//         params
            fastCgi->sendParams(const_cast<char *>("SCRIPT_FILENAME"), const_cast<char *>(fileName.c_str()));
            fastCgi->sendParams(const_cast<char *>("REQUEST_METHOD"), const_cast<char *>("GET"));
            fastCgi->sendParams(const_cast<char *>("CONTENT_LENGTH"), const_cast<char *>("0"));
            fastCgi->sendParams(const_cast<char *>("CONTENT_TYPE"),
                                const_cast<char *>("text/html"));
            if (!query.empty()) {
                fastCgi->sendParams(const_cast<char *>("QUERY_STRING"), const_cast<char *>(query.c_str()));
            }
            fastCgi->sendEndRequestRecord();
            fastCgi->recvRecord(data);
            body = data;
            delete [] data;
            filetype = "text/html";
        } else {
            size_t dot_pos = fileName.find_last_of(".");
            if (dot_pos == std::string::npos) {
                filetype = "text/html";
            } else {
                std::string suffix = fileName.substr(dot_pos);
                auto it = MimeType.find(suffix);
                if (it == MimeType.cend()) {
                    filetype = "text/html";
                } else {
                    filetype = it->second;
                }
            }

            int src_fd = open(fileName.c_str(), O_RDONLY, 0);
            if (src_fd < 0) {
                outBuffer.clear();
                handleError(fd, 404, "NOT FOUND!");
                return ANALYSIS_ERR;
            }
            void *mmapRet = mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
            close(src_fd);
            if (mmapRet == (void *) (-1)) {
                munmap(mmapRet, sbuf.st_size);
                outBuffer.clear();
                handleError(fd, 404, "NOT FOUND!");
                return ANALYSIS_ERR;
            }
            char *src_addr = static_cast<char *>(mmapRet);
            body += std::string(src_addr, src_addr + sbuf.st_size);
            munmap(mmapRet, sbuf.st_size);
        }

        header += "HTTP/1.1 200 OK\r\n";
        if (headers.find("Connection") != headers.end() &&
            (headers["Connection"] == "Keep-Alive" || headers["Connection"] == "keep-alive")) {
            keepAlive = true;
            header += std::string("Connection: Keep-Alive\r\n");
            header += "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        header += "Content-type: " + filetype + "\r\n";
        header += "Content-length: " + std::to_string(body.size()) + "\r\n";
        header += "Server: WebServer\r\n";
        header += "\r\n";
        outBuffer += header;
        outBuffer += body;

        return ANALYSIS_SUCC;
    }
    return ANALYSIS_ERR;
}

void HttpData::handleClose() {
    connectionState = H_DISCONNECTED;
    std::shared_ptr<HttpData> guard(shared_from_this());
    loop->removeFromPoller(channel);
}

void HttpData::newEvent() {
    channel->setEvents(DEFALT_EVENT);
    loop->addPoller(channel, DEFAULT_EXPIRED_TIME);
}