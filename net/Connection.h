#pragma once

#include "Common.h"

#include <functional>
#include <string>
#include <memory>
#include <iostream>
#include <cassert>

class Any{
private:
    class holder{
    public:
        virtual ~holder(){}
        virtual std::type_info const& type() = 0;
        virtual holder* clone() const = 0;
    };
    template<class T>
    class placeholder:public holder{
    public:
        placeholder(const T& val):_val(val){}
        // 获取子类对象保存的数据类型
        virtual std::type_info const& type(){
            return typeid(T);
        }

        virtual holder* clone() const{
            return new placeholder(_val);
        }
    public:
        T _val;
    };

    holder* _context;
public:
    Any():_context(nullptr){}
    template<class T>
    Any(const T& val):_context(new placeholder<T>(val)){
    }
    Any(const Any& other):_context(other._context?other._context->clone():nullptr){
    }
    ~Any(){
        delete _context;
    }

    Any& swap(Any &other){
        std::swap(_context,other._context);
        return *this;
    }
    // 返回子类对象保存的数据的指针
    template<class T>
    T* get(){
        // 想要获取的数据类型,必须和保存的数据类型一致
        assert(typeid(T)==_context->type());
        return &((placeholder<T>*)_context)->_val;
    }

    template<class T>
    Any& operator=(const T& val){
        Any(val).swap(*this);
        return *this;
    }
    Any& operator=(const Any& other){
        Any(other).swap(*this); // 创建一个临时对象进行swap
        return *this;
    }
};

class Connection:noncopyable,public std::enable_shared_from_this<Connection>{
public:
    enum State{
        kDisconnected=0,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    // DISALLOW_COPY_AND_MOVE(Connection);
    Connection(EventLoop* loop,int fd,uint64_t connId,const std::string& name);

    ~Connection();

    void setConnectCallback(const functor_void_sp_conn &cb);

    void setMessageCallback(const functor_void_sp_conn &cb);

    void setCloseCallback(const functor_void_sp_conn &cb);

    void handleRead();

    void handleWrite();

    void handleClose();

    // 连接建立,在创建的时候在loop自己的线程中调用
    void connectEstablished();

    // 连接销毁,在TcpServer析构时才调用
    void connectDestroyed();

    // 连接关闭,供应用调用,写端   EPOLLHUP =》 _closeCallback
    void shutdown();

    void shutdownInLoop();

    State getState() const;

    void setState(State state);

    Socket* getSocket() const;

    Buffer* getReadBuffer();

    Buffer* getSendBuffer();

    EventLoop* getLoop();

    std::string getName() const;

    // read 和 send 是向用户提供的两个读写函数
    std::string read();

    void send(const std::string& msg);

    void sendInLoop(const char* msg,size_t len);

    void setContext(const Any& context);

    Any* getContext();

    // Timer
    void enableTimerInLoop(int sec);
    void enableTimer(int sec);

    void disableTimerInLoop();

    void disableTimer();
    
private:
    EventLoop* _loop;   // 属于的loop
    std::unique_ptr<Socket> _sockConn;
    std::unique_ptr<Channel> _channelConn;

    Any _context;
    State _state;
    std::string _name;

    uint64_t _connId;

    std::unique_ptr<Buffer> _readBuffer;
    std::unique_ptr<Buffer> _sendBuffer;

    functor_void_sp_conn _onConnectCallback; // 连接建立和断开都调用它一下
    functor_void_sp_conn _onMessageCallback;
    functor_void_sp_conn _onCloseCallback;

    bool _useTimer;
};

