#include "ThreadBase.h"

std::atomic_int Thread::_threadIndex(0);

Thread::Thread(ThreadFunc func,const std::string& name)
    :_started(false),
    _joined(false),
    _threadId(0),
    _func(std::move(func)),
    _name(name)
{
    setDefaultName();
}

Thread::~Thread(){
    if(_started && !_joined){
        _thread->detach();  // 设置分离线程
    }
}

void Thread::start(){
    _started=true;
    sem_t sem;
    // 主进程中创建信号量，等待子线程号生成完成
    sem_init(&sem,false,0);

    // 开启线程
    _thread=std::shared_ptr<std::thread>(new std::thread([&](){
        _threadId=CurrentThread::tid();
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        _func();
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}

void Thread::join(){
    _joined=true;
    _thread->join();
}

void Thread::setDefaultName(){
    int num=++_threadIndex;
    if(_name.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        _name=buf;
    }
}