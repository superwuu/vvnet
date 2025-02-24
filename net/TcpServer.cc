#include "TcpServer.h"
#include "Acceptor.h"
#include "Connection.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include "ThreadBase.h"

TcpServer::TcpServer(uint16_t port,const std::string& name)
    :_mainloop(std::make_unique<EventLoop>()),
    _name(name),
    _useTimer(false),
    _acceptor(std::make_unique<Acceptor>(port,_mainloop.get())),
    _threadpool(std::make_unique<EventLoopThreadPool>(_mainloop.get(),name)),
    _started(0),
    _connId(0),
    _onConnectCallback(),
    _onMessageCallback(),
    _connCnt(0),
    _min_thread(0),
    _max_thread(0)
{
    std::function<void(int)> cb=std::bind(&TcpServer::newConnection,this,std::placeholders::_1);
    _acceptor->setNewConnectionCallback(cb);
}
TcpServer::~TcpServer(){
    // LOG_INFO("TcpServer destructor");
    for(auto& item:_connections){
        // 这个局部的shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源了
        std::shared_ptr<Connection> conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&Connection::connectDestroyed,conn));
    }
}

// 开启服务器监听
void TcpServer::start(){
    if(_started++==0){
        _threadpool->start();
        _mainloop->runInLoop(std::bind(&Acceptor::start,_acceptor.get()));
        _mainloop->loop();
    }
}

void TcpServer::setThreadNum(int num){
    _threadpool->setThreadNum(num);
}

void TcpServer::setOnConnectCallback(const functor_void_sp_conn func){
    _onConnectCallback=std::move(func);
}

void TcpServer::setOnMessageCallback(const functor_void_sp_conn func){
    _onMessageCallback=std::move(func);
}

void TcpServer::newConnection(int sockFd)
{
    _connCnt++;
    if(_connCnt>_max_thread){
        // Expansion    _threadpool->addEventLoopThread()
    }
    
    char buf[64]={0};
    snprintf(buf,sizeof(buf),"-%d",_connId);
    std::string connName=_name+buf;

    EventLoop* subLoop=_threadpool->getNextLoop(connName);

    // LOG_INFO("new connection %s",connName.c_str());

    std::shared_ptr<Connection> conn=std::make_unique<Connection>(subLoop,sockFd,_connId,connName);

    _connId++;
    _connections[connName]=conn;

    conn->setConnectCallback(_onConnectCallback);
    conn->setMessageCallback(_onMessageCallback);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));

    if(_useTimer){
        conn->enableTimer(_interval);
    }
    
    subLoop->runInLoop(std::bind(&Connection::connectEstablished,conn));
}

void TcpServer::removeConnection(const std::shared_ptr<Connection> &conn)
{
    _connCnt--;
    if(_connCnt<_min_thread){
        // Reduction    _threadpool->deleteEventLoopThread()
    }
    // conn的closeCallback
    _mainloop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const std::shared_ptr<Connection> &conn)
{
    // LOG_INFO("removeConnectionInLoop");
    _connections.erase(conn->getName());
    EventLoop* loop = conn->getLoop();
    loop->queueInLoop(std::bind(&Connection::connectDestroyed,conn));
}