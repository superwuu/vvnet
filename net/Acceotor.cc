#include "Acceptor.h"

#include "Common.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>
#include <memory>
#include <iostream>

Acceptor::Acceptor(uint16_t port, EventLoop* loop)
    :_sockAccept(std::make_unique<Socket>()),
    _listenStatus(false)
{
    _sockAccept->createNonblocking();
    _channelAccept=std::make_unique<Channel>(loop,_sockAccept->getFd());
    
    _sockAccept->setReuseAddr(true);
    _sockAccept->setReusePort(true);

    _sockAccept->bind(port);

    std::function<void()> cb=std::bind(&Acceptor::handleAccept,this);
    _channelAccept->setReadCallback(cb);
}

Acceptor::~Acceptor(){
    _channelAccept->disableAll();
    _channelAccept->remove();
}

void Acceptor::start(){
    _listenStatus=true;
    _sockAccept->listen();
    _channelAccept->enableRead();
}

void Acceptor::handleAccept(){
    // std::cout<<"handleAccept"<<std::endl;
    int clientFd;
    _sockAccept->accept(clientFd);
    if(clientFd>=0)
    {
        if(_newConnectionCallback){
            _newConnectionCallback(clientFd);
        }
        else{
            ::close(clientFd);
        }
    }
    else{
        // LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            // LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

void Acceptor::setNewConnectionCallback(std::function<void(int)> const &func){
    _newConnectionCallback = func;
}

bool Acceptor::getListenStatus() const{
    return _listenStatus;
}