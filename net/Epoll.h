#pragma once

#include "Common.h"

#include <vector>
#include <unordered_map>

#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

class Epoll:noncopyable{
public:
    Epoll();

    ~Epoll();

    using EventList = std::vector<epoll_event>;
    using ChannelMap = std::unordered_map<int, Channel*>;

    void updateChannel(Channel* ch);

    void removeChannel(Channel* ch);

    void modify(int op,Channel* ch);
    
    std::vector<Channel*> poll(int timeout=-1);


private:
    int _epfd;
    EventList _events;  // 接收内核信息
    ChannelMap _channels;   // 记录是否在epoll中注册过

    const int kNew=-1;
    const int kAdded=1;
    const int kDeleted=2;

    static const int kInitEventListSize = 16;
};