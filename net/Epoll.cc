#include "Epoll.h"
#include "Channel.h"

#include <iostream>

Epoll::Epoll()
    :_epfd(::epoll_create1(EPOLL_CLOEXEC)),
    _events(kInitEventListSize)
{
    if(_epfd == -1){
        // LOG_FATAL("epoll_create1 error");
        exit(0);
    }
}

Epoll::~Epoll(){
    ::close(_epfd);
}

void Epoll::updateChannel(Channel* ch){
    const int statusCode=ch->getStatusCode();
    // LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, ch->getFd(), ch->getListenEvents(), statusCode);

    if(statusCode==kNew || statusCode==kDeleted){
        // 重没注册过或已经被删除
        if(statusCode==kNew){
            int sockFd=ch->getFd();
            _channels[sockFd]=ch;
        }
        ch->setStatusCode(kAdded);
        modify(EPOLL_CTL_ADD,ch);
    }
    else{
        // 已经注册过
        int sockFd=ch->getFd();
        // std::cout<<ch->isNoneEvent()<<std::endl;
        if(ch->isNoneEvent()){
            modify(EPOLL_CTL_DEL,ch);
            ch->setStatusCode(kDeleted);
        }
        else{
            modify(EPOLL_CTL_MOD,ch);
        }
    }
}

void Epoll::removeChannel(Channel* ch){
    int sockFd=ch->getFd();
    _channels.erase(sockFd);

    // LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, sockFd);

    int statusCode=ch->getStatusCode();
    if(statusCode==kAdded){
        modify(EPOLL_CTL_DEL,ch);
    }
    ch->setStatusCode(kNew);
}

void Epoll::modify(int op,Channel* ch){
    struct epoll_event ev;  // 向epoll注册的结构
    bzero(&ev,sizeof(ev));

    int sockFd=ch->getFd();

    ev.data.fd=sockFd;
    ev.data.ptr=ch;

    if(ch->getListenEvents()&Channel::READ_EVENT){
        ev.events|=(EPOLLIN|EPOLLPRI);
    }
    if(ch->getListenEvents()&Channel::WRITE_EVENT){
        ev.events|=EPOLLOUT;
    }
    if(ch->getListenEvents()&Channel::ET){
        ev.events|=EPOLLET;
    }

    if(::epoll_ctl(_epfd,op,sockFd,&ev)<0){
        if(op==EPOLL_CTL_DEL){
            // LOG_ERROR("epoll_ctl op=%d fd=%d error:%s",op,sockFd,strerror(errno));
        }
        else{
            // LOG_FATAL("epoll_ctl op=%d fd=%d error:%s",op,sockFd,strerror(errno));
        }
    }

}

std::vector<Channel*> Epoll::poll(int timeout){
    // 监听的主程序
    // LOG_DEBUG("func=%s epfd=%d events's size=%ld",__FUNCTION__,_epfd,_events.size());

    std::vector<Channel*> activeChannels;
    int saveErrno = errno;

    int eventNum=::epoll_wait(_epfd,&*_events.begin(),_events.size(),timeout);

    if(eventNum>0){
        // 有通知
        // LOG_INFO("func=%s eventNum=%d",__FUNCTION__,eventNum);
        for(int i=0;i<eventNum;i++){
            Channel* ch=static_cast<Channel*>(_events[i].data.ptr);
            int events=_events[i].events;
            if(events&(EPOLLIN | EPOLLPRI)){
                ch->setReadyEvents(Channel::READ_EVENT);
            }
            if(events&EPOLLOUT){
                ch->setReadyEvents(Channel::WRITE_EVENT);
            }
            if((events&EPOLLHUP)&&!(events&EPOLLIN)){
                ch->setReadyEvents(Channel::CLOSE_EVENT);
            }
            activeChannels.push_back(ch);
        }

        if(eventNum==_events.size()){
            _events.resize(25*_events.size());
        }
    }
    else if(eventNum==0){
        // LOG_DEBUG("epoll wait timeout");
    }
    else{
        if(saveErrno!=EINTR){
            // LOG_ERROR("epoll wait error");
        }
    }
    return activeChannels;
}