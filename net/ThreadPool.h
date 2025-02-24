#pragma once

#include "Common.h"
#include "ThreadBase.h"

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class EventloopThread:noncopyable{
public:
    EventloopThread(const std::string& name=std::string());

    ~EventloopThread();

    EventLoop* startLoop();

    // 放到线程里面运行的程序
    void threadFunc();

private:
    EventLoop* _loop;    // 这个线程对应的Eventloop
    bool _quit;
    std::mutex _mutex;
    std::condition_variable _cond;
    Thread _thread;
};

class EventLoopThreadPool:noncopyable{
public:
    EventLoopThreadPool(EventLoop* baseLoop,const std::string& name);

    ~EventLoopThreadPool();

    void setThreadNum(int num);

    void start();

    EventLoop* getNextLoop(const std::string& key);

    void addEventLoopThread(const std::string& name);

    void deleteEventLoopThread(int index);


private:
    EventLoop* _baseLoop;
    std::string _name;
    bool _started;
    int _threadNum;
    int _next;  // 选择的下一个thread编号
    ConsistenHash* _hash;
    std::vector<std::unique_ptr<EventloopThread>> _threads;
    std::vector<EventLoop*> _loops;
};