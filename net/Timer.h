#pragma once

#include "Common.h"

#include <cassert>
#include <vector>
#include <memory>
#include <unordered_map>

#include <sys/timerfd.h>
#include <unistd.h>

#include <iostream>

class TimerTask:noncopyable{
public:
    TimerTask(uint64_t id,uint32_t delay,const functor_void_null& cb);
    ~TimerTask();

    void cancel();

    void setReleaseCallback(const functor_void_null& cb);

    uint32_t getDelayTime() const;
private:
    uint64_t _id;   // Timer任务对象id
    uint32_t _timeout;
    bool _canceled;
    functor_void_null _task;
    functor_void_null _release;

};

class TimerWheel:noncopyable{
public:
    TimerWheel(EventLoop* loop,int capacity);
    ~TimerWheel();

    void addTimer(uint64_t id,uint32_t delay,const functor_void_null& cb);

    void refreshTimer(uint64_t id);

    void cancelTimer(uint64_t id);

    bool checkTimer(uint64_t id);

private:
    void removeTask(uint64_t id);

    static int createTimerFd();

    int readTimerFd();

    void runTimerTask();

    void onTime();

    void timerAddInLoop(uint64_t id,uint32_t delay,const functor_void_null& cb);

    void timerRefleshInLoop(uint64_t id);

    void timerCancelInLoop(uint64_t id);

private:
    int _tick;  // 当前时间
    int _capacity;  // 表盘最大数量, 最大延迟时间

    using WeakTask=std::weak_ptr<TimerTask>;
    using PtrTask=std::shared_ptr<TimerTask>;

    std::vector<std::vector<PtrTask>> _wheel;   // 定时器轮结构
    std::unordered_map<uint64_t,WeakTask> _taskMap; // 任务id->任务指针

    EventLoop* _loop;   // 所属的EventLoop
    int _timerFd;
    std::unique_ptr<Channel> _timerChannel;
};