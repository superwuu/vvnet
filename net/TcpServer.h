#pragma once

#include "Common.h"

#include <map>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "Timer.h"


class TcpServer:noncopyable{
public:

    TcpServer(uint16_t port=8080,const std::string& name=std::string());
    ~TcpServer();

    using ConnectionMap=std::unordered_map<std::string,std::shared_ptr<Connection>>;

    // 开启服务器监听
    void start();

    void setThreadNum(int num);

    void setOnConnectCallback(const functor_void_sp_conn func);

    void setOnMessageCallback(const functor_void_sp_conn func);

    void useTimer(int sec){
        _interval=sec;
        _useTimer=true;
    }

private:
    std::unique_ptr<EventLoop> _mainloop;
    std::unique_ptr<Acceptor> _acceptor;    // 监听链接事件
    ConnectionMap _connections; // 保存所有的连接
    
    std::unique_ptr<EventLoopThreadPool> _threadpool;
    functor_void_sp_conn _onConnectCallback;
    functor_void_sp_conn _onMessageCallback;

    std::atomic_int _started;
    const std::string _name;
    int _connId;

    bool _useTimer;
    int _interval;

    int _min_thread;
    int _max_thread;
    int _connCnt;

private:
    void newConnection(int sockFd);

    void removeConnection(const std::shared_ptr<Connection> &conn);

    void removeConnectionInLoop(const std::shared_ptr<Connection> &conn);
};