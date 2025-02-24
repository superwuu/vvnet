#pragma once

#include "Common.h"

#include <functional>

class Channel:noncopyable{
public:
    Channel(EventLoop* loop,int fd);

    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent() const;

    void enableRead();
    
    void enableWrite();

    void enableET();

    void disableRead();

    void disableWrite();

    void disableAll();

    void update();

    void remove();

    int getFd() const;
    
    short getListenEvents() const;

    short getReadyEvents() const;

    bool isNoneEvent() const;

    bool isReadEvent() const;

    bool isWriteEvent() const;

    int getStatusCode() const;

    void setStatusCode(int code);

    void setReadyEvents(short ev);
    
    void setReadCallback(functor_void_null const &func);

    void setWriteCallback(functor_void_null const &func);
    
    void setCloseCallback(functor_void_null const &func);

    static const short READ_EVENT;
    static const short WRITE_EVENT;
    static const short CLOSE_EVENT;
    static const short ET;

private:
    EventLoop* _loop;
    int _fd;
    short _listen_events;
    short _ready_events;

    int _statusCode;    // 记录这个channel在epoll中状态
    
    functor_void_null _readCallback;
    functor_void_null _writeCallback;
    functor_void_null _closeCallback;
};