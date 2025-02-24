#pragma once

#include "Common.h"
#include "CurrentThread.h"

#include <functional>
#include <iostream>
#include <thread>
#include <memory>
#include <atomic>

#include <semaphore.h>


class Thread:noncopyable{
public:
    using ThreadFunc=functor_void_null; //线程中运行的函数
    explicit Thread(ThreadFunc func,const std::string& name=std::string());

    ~Thread();

    void start();

    void join();


private:
    bool _started;
    bool _joined;
    std::shared_ptr<std::thread> _thread;   // 真正的线程
    pid_t _threadId;
    ThreadFunc _func;
    std::string _name;
    static std::atomic_int _threadIndex;

private:
    void setDefaultName();
};