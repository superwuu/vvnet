#include "EventLoop.h"
#include "CurrentThread.h"
#include "Channel.h"
#include "Epoll.h"
#include "Timer.h"

#include <iostream>

EventLoop::EventLoop()
    :_looping(false),
    _quit(false),
    _callingPendingFunctors(false),
    _threadId(CurrentThread::tid()),
    _ep(std::make_unique<Epoll>()),
    _wakeupFd(createEventFd()),
    _wakeupChannel(std::make_unique<Channel>(this,_wakeupFd)),
    _timerWheel(std::make_unique<TimerWheel>(this,60))
{
    // LOG_DEBUG("EventLoop created %p in thread %d, wakeupfd:%d",this,_threadId,_wakeupFd);
    _wakeupChannel->setReadCallback(std::bind(&EventLoop::handleWakeUp,this));
    _wakeupChannel->enableRead();
}

EventLoop::~EventLoop(){
    _wakeupChannel->disableAll();
    _wakeupChannel->remove();
    ::close(_wakeupFd);
}
void EventLoop::loop(){
    _looping=true;
    _quit=false;

    // LOG_INFO("Eventloop %p start looping",this);

    while(!_quit){
        // std::cout<<"start loop: "<<this<<std::endl;
        _activeChannels.clear();
        _activeChannels=_ep->poll();
        // std::cout<<this<<" _activeChannels size: "<<_activeChannels.size()<<' '<<_activeChannels[0]->getFd()<<std::endl;
        for(Channel* ch:_activeChannels){
            ch->handleEvent();
        }
        doPendingFunctors();
    }
    // LOG_INFO("Eventloop %p stop looping",this);
    _looping=false;
}

void EventLoop::quit(){
    _quit=true;
    if(!isInLoopThread()){
        // std::cout<<"wakeup"<<std::endl;
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(functor_void_null cb){
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(cb);
    }
}

// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(functor_void_null cb){
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _pendingFunctors.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    // || _callingPendingFunctors的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if (!isInLoopThread() || _callingPendingFunctors) 
    {
        wakeup(); // 唤醒loop所在线程
    }
}

// 用来唤醒loop所在的线程的
void EventLoop::wakeup(){
    // std::cout<<"wakeup function"<<std::endl;
    uint64_t one=1;
    ssize_t n=::write(_wakeupFd,&one,sizeof(one));
    if(n!=sizeof(one)){
        // LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8",n);
    }
}

void EventLoop::updateChannel(Channel* ch) const{
    _ep->updateChannel(ch);
}

void EventLoop::removeChannel(Channel* ch) const{
    _ep->removeChannel(ch);
}

// 判断EventLoop对象是否在自己的线程里面
bool EventLoop::isInLoopThread() const{
    return _threadId ==  CurrentThread::tid();
}

void EventLoop::handleWakeUp(){    // wake up
    uint64_t one=1;
    ssize_t n=::read(_wakeupFd,&one,sizeof one);
    if(n!=sizeof one){
        // LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8",n);
    }
}

void EventLoop::doPendingFunctors(){    // 执行回调
    std::vector<functor_void_null> functors;
    _callingPendingFunctors=true;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        functors.swap(_pendingFunctors);
    }
    for(const functor_void_null& fn:functors){
        fn();
    }
    _callingPendingFunctors=false;
}

int EventLoop::createEventFd(){
    int eventFd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(eventFd<0){
        // LOG_FATAL("eventfd error");
        exit(0);
    }
    return eventFd;
}

// Timer
void EventLoop::addTimer(uint64_t id,uint32_t delay,const functor_void_null& cb){
    _timerWheel->addTimer(id,delay,cb);
}

void EventLoop::refreshTimer(uint64_t id){
    _timerWheel->refreshTimer(id);
}

bool EventLoop::checkTimer(uint64_t id){
    return _timerWheel->checkTimer(id);
}

void EventLoop::cancelTimer(uint64_t id){
    _timerWheel->cancelTimer(id);
}