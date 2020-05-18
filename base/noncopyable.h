//
// Created by Icharle on 2020/5/16.
//

#ifndef WEBNGINX_NONCOPYABLE_H
#define WEBNGINX_NONCOPYABLE_H

class noncopyable {
protected:
    noncopyable() {};

    ~noncopyable() {};

private:
    noncopyable(const noncopyable &);

    const noncopyable &operator=(const noncopyable &);
};

#endif //WEBNGINX_NONCOPYABLE_H
