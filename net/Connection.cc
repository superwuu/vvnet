#include "Connection.h"

#include "Common.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "Timer.h"

Connection::Connection(EventLoop* loop,int fd,uint64_t connId ,const std::string& name)
    :_loop(loop),
    _name(name),
    _useTimer(false),
    _connId(connId),
    _state(kConnecting),
    _sockConn(std::make_unique<Socket>(fd)),
    _channelConn(std::make_unique<Channel>(loop,fd)),
    _readBuffer(std::make_unique<Buffer>()),
    _sendBuffer(std::make_unique<Buffer>())
{
    if(loop!=nullptr){
        _channelConn->setReadCallback(std::bind(&Connection::handleRead,this));
        _channelConn->setWriteCallback(std::bind(&Connection::handleWrite,this));
        _channelConn->setCloseCallback(std::bind(&Connection::handleClose,this));

        // LOG_INFO("Connection::Connection() %s",_sockConn->getAddr().c_str());
        _sockConn->setKeepAlive(true);
    }
    else{
        // 客户端
        setState(kConnected);
    }
    
}

Connection::~Connection(){
    // LOG_INFO("Connection::~Connection() %s",_sockConn->getAddr().c_str());
}

void Connection::setConnectCallback(const functor_void_sp_conn &cb){
    _onConnectCallback = cb;
}

void Connection::setMessageCallback(const functor_void_sp_conn &cb){
    _onMessageCallback = cb;
}

void Connection::setCloseCallback(const functor_void_sp_conn &cb){
    _onCloseCallback = cb;
}

void Connection::handleRead(){
    // 收到消息之后执行的回调
    int savedErrno = -1;
    ssize_t n = _readBuffer->readFd(_sockConn->getFd(),&savedErrno);
    if(n>0){
        // 正常收到
        _loop->refreshTimer(_connId);
        _onMessageCallback(shared_from_this());
    }
    else if(n==0){
        // 对方已经关闭,直接关掉就行
        handleClose();
    }
    else{
        // LOG_ERROR("connection read error");
    }
}

void Connection::handleWrite(){
    // 可以读
    if(_channelConn->isWriteEvent()){
        int saveErrno=0;
        ssize_t n=_sendBuffer->writeFd(_sockConn->getFd(),&saveErrno);
        if(n>0){
            // 发送成功
            _sendBuffer->moveReaderIndex(n);
            if(_sendBuffer->getReadableBytes()==0){
                // 当前发送完毕
                _sendBuffer->reInit();
                _channelConn->disableWrite();
                if(_state==kDisconnecting){
                    shutdownInLoop();
                }
            }
        }
        else{
            // 发送失败
            // LOG_ERROR("connection write error");
        }
    }
    else{
        // LOG_ERROR("connection is down, no more writing");
    }
}

void Connection::handleClose(){
    // LOG_INFO("connection close");
    setState(kDisconnected);
    _channelConn->disableAll();
    disableTimer();

    std::shared_ptr<Connection> connPtr(shared_from_this());
    _onConnectCallback(connPtr);
    _onCloseCallback(connPtr);
}

// 连接建立,在创建的时候在loop自己的线程中调用
void Connection::connectEstablished(){
    setState(kConnected);
    _channelConn->enableRead();
    _onConnectCallback(shared_from_this());
}

// 连接销毁,在TcpServer析构时才调用
void Connection::connectDestroyed(){
    if(_state==kConnected){
        setState(kDisconnected);
        _channelConn->disableAll();
        _onCloseCallback(shared_from_this());
    }
    _channelConn->remove();
}

// 连接关闭,供应用调用,写端   EPOLLHUP =》 _closeCallback
void Connection::shutdown(){
    if(_state==kConnected){
        // handleClose();
        setState(kDisconnecting);
        _loop->runInLoop(std::bind(&Connection::shutdownInLoop,this));
    }
}

void Connection::shutdownInLoop(){
    if(!_channelConn->isWriteEvent()){
        // 已经发送完了
        _sockConn->shutdownWrite(); // 关闭写端
    }
}

Connection::State Connection::getState() const{
    return _state;
}

void Connection::setState(State state){
    _state=state;
}

Socket* Connection::getSocket() const{
    return _sockConn.get();
}

Buffer* Connection::getReadBuffer(){
    return _readBuffer.get();
}

Buffer* Connection::getSendBuffer(){
    return _sendBuffer.get();
}

EventLoop* Connection::getLoop(){
    return _loop;
}

std::string Connection::getName() const{
    return _name;
}

// read 和 send 是向用户提供的两个读写函数
std::string Connection::read(){
    std::cout<<"aaaaaaaaa"<<std::endl;
    if(_loop==nullptr){
        // 客户端
        int savedErrno = -1;
        ssize_t n = _readBuffer->readFd(_sockConn->getFd(),&savedErrno);
        if(n==0){
            // 关闭
            handleClose();
        }
    }
    return _readBuffer->readAll();
}

void Connection::send(const std::string& msg){
    if(_state==kConnected){
        if(_loop==nullptr){
            // 客户端
            sendInLoop(msg.c_str(),msg.size());
        }
        else{
            if(_loop->isInLoopThread()){
                sendInLoop(msg.c_str(),msg.size());
            }
            else{
                _loop->runInLoop(std::bind(&Connection::sendInLoop,this,msg.c_str(),msg.size()));
            }
        }
    }
}

void Connection::sendInLoop(const char* msg,size_t len){
    int remain=0;
    ssize_t nwrote=0;
    if(!_channelConn->isWriteEvent() && _sendBuffer->getReadableBytes()==0){
        // 第一次写数据,先直接写fd
        nwrote=::write(_channelConn->getFd(),msg,len);
        remain=len-nwrote;
        if(nwrote<0){
            // LOG_ERROR("TcpConnection::sendInLoop");
        }
    }

    if(remain>0){
        // 一次没有发送完,要进sendBuffer
        _sendBuffer->writeBufAndMove(msg+nwrote,remain);
        if(!_channelConn->isWriteEvent()){
            _channelConn->enableWrite();    // 注册写事件
        }
    }
}

void Connection::setContext(const Any& context){
    _context = context;
}

Any* Connection::getContext(){
    return &_context;
}

// Timer
void Connection::enableTimerInLoop(int sec){
    _useTimer=true;
    if(_loop->checkTimer(_connId)){
        return _loop->refreshTimer(_connId);
    }
    _loop->addTimer(_connId,sec,std::bind(&Connection::handleClose,this));
}
void Connection::enableTimer(int sec){
    _loop->runInLoop(std::bind(&Connection::enableTimerInLoop,this,sec));
}

void Connection::disableTimerInLoop(){
    _useTimer=false;
    if(_loop->checkTimer(_connId)){
        _loop->cancelTimer(_connId);
    }
}

void Connection::disableTimer(){
    _loop->runInLoop(std::bind(&Connection::disableTimerInLoop,this));
}