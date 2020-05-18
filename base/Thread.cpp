//
// Created by Icharle on 2020/5/16.
//

#include "Thread.h"
#include "CurrentThread.h"
#include <string>
#include <sys/prctl.h>
#include <sys/types.h>
#include <assert.h>

namespace CurrentThread {
    __thread int t_cacheTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6; // tid length = 5
    __thread const char *t_threadName = "default";
}

pid_t gettid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid() {
    if (t_cacheTid == 0) {
        t_cacheTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d", t_cacheTid);
    }
}

struct TheadData {
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t *tid_;
    CountDownLatch *latch_;

    TheadData(const ThreadFunc &func, const std::string &name, pid_t *tid, CountDownLatch *latch) :
            func_(func),
            name_(name),
            tid_(tid),
            latch_(latch) {

    }

    void runInThread() {
        *tid_ = CurrentThread::tid();
        tid_ = nullptr;
        latch_->countDown();
        latch_ = nullptr;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);

        func_();
        CurrentThread::t_threadName = "finished";
    }
};

void *startThread(void *obj) {
    TheadData *data = static_cast<TheadData *>(obj);
    data->runInThread();
    delete data;
    return nullptr;
}

Thread::Thread(const ThreadFunc &func, const std::string &name) :
        started_(false),
        joined_(false),
        pthread_(0),
        tid_(0),
        func_(func),
        name_(name),
        latch_(1) {
    setDeafaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(pthread_);
    }
}

void Thread::setDeafaultName() {
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

void Thread::start() {
    assert(!started_);
    started_ = true;
    TheadData *data = new TheadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthread_, nullptr, &startThread, data)) {
        started_ = false;
        delete data;
    } else {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthread_, nullptr);
}
