//
// Created by Icharle on 2020/5/16.
//

#ifndef WEBNGINX_THREAD_H
#define WEBNGINX_THREAD_H

#include "CountDownLatch.h"
#include "noncopyable.h"
#include <functional>              // for function
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/syscall.h>           // for SYS_**
#include <unistd.h>

class Thread : noncopyable {
public:
    typedef std::function<void()> ThreadFunc;

    explicit Thread(const ThreadFunc &, const std::string &name = std::string());

    ~Thread();

    void start();

    int join();

    bool started() const { return started_; }

    pid_t tid() const { return tid_; }

    const std::string &name() const { return name_; }

private:
    void setDeafaultName();

    bool started_;
    bool joined_;
    pthread_t pthread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
};

#endif //WEBNGINX_THREAD_H
