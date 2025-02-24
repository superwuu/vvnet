#pragma once

#include "Common.h"

#include <functional>
#include <memory>

class Acceptor: noncopyable{
public:
    Acceptor(uint16_t port, EventLoop* loop);

    ~Acceptor();

    void start();
    
    void handleAccept();

    void setNewConnectionCallback(std::function<void(int)> const &func);

    bool getListenStatus() const;
private:
    std::unique_ptr<Socket> _sockAccept;
    std::unique_ptr<Channel> _channelAccept;
    
    std::function<void(int)> _newConnectionCallback;

    bool _listenStatus;
};