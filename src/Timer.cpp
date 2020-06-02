//
// Created by Icharle on 2020/5/18.
//

#include "Timer.h"
#include <sys/time.h>
#include <queue>
#include "HttpData.h"

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout) :
        deleted(false),
        shareHttpData(requestData) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    expiredTime = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::TimerNode(TimerNode &timerNode) :
        shareHttpData(timerNode.shareHttpData) {

}

TimerNode::~TimerNode() {
    if (shareHttpData) {
        shareHttpData->handleClose();
    }
}

void TimerNode::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    expiredTime = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    size_t diff = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));;
    if (diff < expiredTime) {
        return true;
    } else {
        this->setDeleted();
        return false;
    }
}

void TimerNode::clearReq() {
    shareHttpData.reset();
    this->setDeleted();
}

TimerManager::TimerManager() = default;

TimerManager::~TimerManager() = default;

void TimerManager::addTimer(std::shared_ptr<HttpData> shareHttpData, int timeout) {
    shareTimerNode new_node(new TimerNode(shareHttpData, timeout));
    timerNodeQueue.push(new_node);
    shareHttpData->linkTimer(new_node);
}

void TimerManager::handleExpiredEvent() {
    while (!timerNodeQueue.empty()) {
        shareTimerNode ptimer_now = timerNodeQueue.top();
        if (ptimer_now->isDeleted()) {
            timerNodeQueue.pop();
        } else if (!ptimer_now->isValid()) {
            timerNodeQueue.pop();
        } else {
            break;
        }
    }
}