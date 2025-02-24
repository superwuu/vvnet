#pragma once

#include "Common.h"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

#include <sys/eventfd.h>

class EventLoop:noncopyable {
public:
    EventLoop();

    ~EventLoop();

    using ChannelList = std::vector<Channel*>;

    void loop();
    
    void quit();

    // 在当前loop中执行cb
    void runInLoop(functor_void_null cb);

    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(functor_void_null cb);

    // 用来唤醒loop所在的线程的
    void wakeup();

    void updateChannel(Channel* ch) const;

    void removeChannel(Channel* ch) const;

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const;

    // Timer
    void addTimer(uint64_t id,uint32_t delay,const functor_void_null& cb);

    void refreshTimer(uint64_t id);

    bool checkTimer(uint64_t id);

    void cancelTimer(uint64_t id);


private:

    void handleWakeUp();

    void doPendingFunctors();

    int createEventFd();

    std::atomic_bool _looping;  // 原子操作，通过CAS实现的
    std::atomic_bool _quit; // 标识退出loop循环

    std::unique_ptr<Epoll> _ep;

    int _wakeupFd;
    std::unique_ptr<Channel> _wakeupChannel;

    ChannelList _activeChannels;

    std::atomic_bool _callingPendingFunctors; // 标识当前loop是否有需要执行的回调操作
    std::vector<functor_void_null> _pendingFunctors; // 存储loop需要执行的所有的回调操作
    std::mutex _mutex; // 互斥锁，用来保护上面vector容器的线程安全操作

    const pid_t _threadId; // 记录当前loop所在线程的id

    std::unique_ptr<TimerWheel> _timerWheel;
};