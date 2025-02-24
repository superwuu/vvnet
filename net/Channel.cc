#include "Channel.h"

#include "EventLoop.h"

const short Channel::READ_EVENT=1;
const short Channel::WRITE_EVENT=2;
const short Channel::CLOSE_EVENT=4;
const short Channel::ET=8;

Channel::Channel(EventLoop* loop,int fd)
    :_loop(loop),_fd(fd),_listen_events(0),_ready_events(0),_statusCode(-1)
{}

Channel::~Channel(){}

// fd得到poller通知以后，处理事件的
void Channel::handleEvent() const{
    if(_ready_events & CLOSE_EVENT){
        if(_closeCallback){
            _closeCallback();
        }
    }
    if(_ready_events & READ_EVENT){
        if(_readCallback){
            _readCallback();
        }
    }
    if(_ready_events & WRITE_EVENT){
        if(_writeCallback){
            _writeCallback();
        }
    }
}

void Channel::enableRead(){
    _listen_events |= READ_EVENT;
    update();
}
void Channel::enableWrite(){
    _listen_events |= WRITE_EVENT;
    update();
}

void Channel::enableET(){
    _listen_events|=ET;
    _loop->updateChannel(this);
}

void Channel::disableRead(){
    _listen_events &= ~READ_EVENT;
    update();
}

void Channel::disableWrite(){
    _listen_events &= ~WRITE_EVENT;
    update();
}

void Channel::disableAll(){
    _listen_events = 0;
    update();
}

void Channel::update(){  // 更新epoll中的channel
    _loop->updateChannel(this);
}

void Channel::remove(){  // 删除epoll中的channel
    _loop->removeChannel(this);
}

int Channel::getFd() const{
    return _fd;
}

short Channel::getListenEvents() const{
    return _listen_events;
}

short Channel::getReadyEvents() const{
    return _ready_events;
}

bool Channel::isNoneEvent() const{
    return _listen_events==0;
}

bool Channel::isReadEvent() const{
    return _listen_events & READ_EVENT;
}

bool Channel::isWriteEvent() const{
    return _listen_events & WRITE_EVENT;
}

int Channel::getStatusCode() const{
    return _statusCode;
}

void Channel::setStatusCode(int code){
    _statusCode = code;
}

void Channel::setReadyEvents(short ev){
    if(ev&READ_EVENT){
        _ready_events|=READ_EVENT;
    }
    if(ev&WRITE_EVENT){
        _ready_events|=WRITE_EVENT;
    }
    if(ev&CLOSE_EVENT){
        _ready_events|=CLOSE_EVENT;
    }
    if(ev&ET){
        _ready_events|=ET;
    }
}

void Channel::setReadCallback(functor_void_null const &func){
    _readCallback=std::move(func);
}

void Channel::setWriteCallback(functor_void_null const &func){
    _writeCallback=std::move(func);
}

void Channel::setCloseCallback(functor_void_null const &func){
    _closeCallback=std::move(func);
}