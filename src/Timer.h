//
// Created by Icharle on 2020/5/18.
//

#ifndef WEBNGINX_TIMER_H
#define WEBNGINX_TIMER_H

#include "HttpData.h"
#include <functional>
#include <memory>
#include <queue>
#include <deque>

class HttpData;

class TimerNode {
public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);

    TimerNode(TimerNode &timerNode);

    ~TimerNode();

    void update(int timeout);

    bool isValid();

    void clearReq();

    void setDeleted() {
        deleted = true;
    }

    bool isDeleted() const {
        return deleted;
    }

    size_t getExpTime() const {
        return expiredTime;
    }

private:
    bool deleted;
    size_t expiredTime;
    std::shared_ptr<HttpData> shareHttpData;
};

struct TimerCmp {
    bool operator()(std::shared_ptr<TimerNode> &first, std::shared_ptr<TimerNode> &second) const {
        return first->getExpTime() > second->getExpTime();
    }
};

class TimerManager {
public:
    TimerManager();

    ~TimerManager();

    void addTimer(std::shared_ptr<HttpData> shareHttpData, int timeout);

    void handleExpiredEvent();

private:
    typedef std::shared_ptr<TimerNode> shareTimerNode;
    std::priority_queue<shareTimerNode, std::deque<shareTimerNode>, TimerCmp> timerNodeQueue;
};

#endif //WEBNGINX_TIMER_H
