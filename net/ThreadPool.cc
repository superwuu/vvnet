#include "ThreadPool.h"
#include "EventLoop.h"
#include "ConsistenHash.h"

EventloopThread::EventloopThread(const std::string& name)
    :_loop(nullptr),
    _quit(false),
    _thread(std::bind(&EventloopThread::threadFunc,this),name),
    _mutex(),
    _cond()
{}

EventloopThread::~EventloopThread(){
    _quit = true;
    if(_loop!=nullptr){
        _loop->quit();
        // join后主线程等待loop结束
        _thread.join();
    }
}

EventLoop* EventloopThread::startLoop(){
    _thread.start();    // 启动线程，线程内运行threadFunc函数
    // 此时线程已经创建成功，已有线程号
    EventLoop* loop=nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);  // 如果还没启动创造Eventloop对象，就等待
        while(_loop==nullptr){
            _cond.wait(lock);
        }
        loop=_loop;
    }
    return loop;

}

// 放到线程里面运行的程序
void EventloopThread::threadFunc(){
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }

    loop.loop();    // 线程阻塞在loop
    std::unique_lock<std::mutex> lock(_mutex);
    _loop = nullptr;
}


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string& name)
    :_baseLoop(baseLoop),
    _name(name),
    _started(false),
    _threadNum(0),
    _next(0),
    _hash(new ConsistenHash(5))
{}

EventLoopThreadPool::~EventLoopThreadPool()
{}

void EventLoopThreadPool::addEventLoopThread(const std::string& name){
    int index=_threadNum++;
    EventloopThread *thread=new EventloopThread(name);
    _threads.push_back(std::unique_ptr<EventloopThread>(thread));
    // 这里才启动线程
    _loops.push_back(thread->startLoop());
    _hash->addNode(name,index);
}

void EventLoopThreadPool::deleteEventLoopThread(int index){
    std::unique_ptr<EventloopThread> thread = std::move(_threads[index]);
    _threads.erase(_threads.begin() + index);
    // _threadNum--;
}

void EventLoopThreadPool::setThreadNum(int num)
{
    _threadNum = num;
}

void EventLoopThreadPool::start(){
    _started=true;
    for(int i=0;i<_threadNum;++i){
        char buf[_name.size()+32];
        snprintf(buf,sizeof(buf),"%s%d",_name.c_str(),i);
        // 创建一个Thread类，代表一个线程，但是这个线程函数还没有运行
        EventloopThread *thread=new EventloopThread(buf);
        _threads.push_back(std::unique_ptr<EventloopThread>(thread));
        // 这里才启动线程
        _loops.push_back(thread->startLoop());
        _hash->addNode(buf,i);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(const std::string& key){
    EventLoop* loop=_baseLoop;
    // 轮询算法
    // if(!_loops.empty()){
    //     // 说明有多线程
    //     loop=_loops[_next];
    //     ++_next;
    //     if(_next>=_loops.size()){
    //         _next=0;
    //     }
    // }
    
    // 一致性哈希算法
    if(!_loops.empty()){
        _next=_hash->getNode(key);
        // std::cout<<_next<<std::endl;
        if(_next>=_loops.size()){
            _next=0;
        }
        loop=_loops[_next];
    }
    return loop;
}