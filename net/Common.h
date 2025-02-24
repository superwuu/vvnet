#pragma once

#include <functional>
#include <memory>

class Acceptor;
class Buffer;
class Channel;
class Connection;
class Epoll;
class EventLoop;
class Socket;
class ConsistenHash;
class ThreadBase;
class EventloopThread;
class EventLoopThreadPool;

class TimerTask;
class TimerWheel;

using functor_void_null=std::function<void()>;
using functor_void_sp_conn=std::function<void(std::shared_ptr<Connection>)>;
using functor_ul_string=std::function<size_t(std::string)>;


class noncopyable{  // 继承这个类之后就失去了拷贝构造和拷贝赋值
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable& operator=(const noncopyable&)=delete;

protected:
    noncopyable()=default;
    ~noncopyable()=default;
};