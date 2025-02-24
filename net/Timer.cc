#include "Timer.h"

#include "Channel.h"
#include "EventLoop.h"

TimerTask::TimerTask(uint64_t id,uint32_t delay,const functor_void_null& cb):_id(id),_timeout(delay),_task(cb){}
TimerTask::~TimerTask(){
    // std::cout<<"~TimerTask"<<std::endl;
    if(!_canceled){
        // 没有被取消,执行conn关闭任务
        _task();
    }
    _release(); // 销毁任务信息,从taskmap中删除
}

void TimerTask::cancel(){
    _canceled=true;
}

void TimerTask::setReleaseCallback(const functor_void_null& cb){
    _release=cb;
}

uint32_t TimerTask::getDelayTime() const{
    return _timeout;
}


TimerWheel::TimerWheel(EventLoop* loop,int capacity):_capacity(capacity),_wheel(capacity),_tick(0),_loop(loop),_timerFd(createTimerFd()),_timerChannel(new Channel(_loop,_timerFd)){
    _timerChannel->setReadCallback(std::bind(&TimerWheel::onTime,this));
    _timerChannel->enableRead();
}
TimerWheel::~TimerWheel(){}

void TimerWheel::addTimer(uint64_t id,uint32_t delay,const functor_void_null& cb){
    _loop->queueInLoop(std::bind(&TimerWheel::timerAddInLoop,this,id,delay,cb));
}

void TimerWheel::refreshTimer(uint64_t id){
    _loop->queueInLoop(std::bind(&TimerWheel::timerRefleshInLoop,this,id));
}

void TimerWheel::cancelTimer(uint64_t id){
    _loop->queueInLoop(std::bind(&TimerWheel::timerCancelInLoop,this,id));
}

bool TimerWheel::checkTimer(uint64_t id){
    return _taskMap.find(id)!=_taskMap.end();
}

void TimerWheel::removeTask(uint64_t id){
    if(_taskMap.find(id)!=_taskMap.end()){
        // std::cout<<"remove"<<std::endl;
        _taskMap.erase(id);
    }
    return;
}

int TimerWheel::readTimerFd(){
    uint64_t howmany;
    int ret=::read(_timerFd,&howmany,sizeof(howmany));
    assert(ret>=0);
    return howmany;
}

void TimerWheel::runTimerTask(){
    _tick=(_tick+1)%_capacity;
    _wheel[_tick].clear();  // 清空_tick位置的定时任务,会调用timerTask析构
}

void TimerWheel::onTime(){
    // 获取过期触发次数
    int times=readTimerFd();
    for(int i=0;i<times;++i){
        runTimerTask();
    }
}

void TimerWheel::timerAddInLoop(uint64_t id,uint32_t delay,const functor_void_null& cb){
    // std::cout<<"addTimerItem"<<std::endl;
    std::shared_ptr<TimerTask> task=std::make_shared<TimerTask>(id,delay,cb);
    task->setReleaseCallback(std::bind(&TimerWheel::removeTask,this,id));
    int pos=(_tick+delay)%_capacity;
    _wheel[pos].push_back(task);
    _taskMap[id]=task;
}

void TimerWheel::timerRefleshInLoop(uint64_t id){
    if(_taskMap.find(id)==_taskMap.end()){
        // 没找到,直接退出
        return;
    }
    std::shared_ptr<TimerTask> task=_taskMap[id].lock();
    int delay=task->getDelayTime();
    int pos=(_tick+delay)%_capacity;
    _wheel[pos].push_back(task);
}

void TimerWheel::timerCancelInLoop(uint64_t id){
    if(_taskMap.find(id)==_taskMap.end()){
        return;
    }
    std::shared_ptr<TimerTask> task=_taskMap[id].lock();
    task->cancel();
}

int TimerWheel::createTimerFd(){
        int fd=::timerfd_create(CLOCK_MONOTONIC,0);
        assert(fd>=0);
        struct itimerspec timerSpec;
        timerSpec.it_interval.tv_sec=1;
        timerSpec.it_interval.tv_nsec=0;
        timerSpec.it_value.tv_sec=1;
        timerSpec.it_value.tv_nsec=0;
        ::timerfd_settime(fd,0,&timerSpec,nullptr);
        return fd;
    }