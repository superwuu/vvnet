#include "Socket.h"

Socket::Socket():_fd(-1){}

Socket::Socket(int fd):_fd(fd){}

Socket::~Socket(){
    ::close(_fd);
}

void Socket::setFd(int fd){
    _fd = fd;
}

int Socket::getFd() const{
    return _fd;
}

std::string Socket::getAddr() const{
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    socklen_t len=sizeof(addr);
    if(getpeername(_fd,(struct sockaddr*)&addr,&len)==-1){
        return "";
    }
    std::string res(inet_ntoa(addr.sin_addr));
    res+=":";
    res+=std::to_string(ntohs(addr.sin_port));
    return res;
}

void Socket::createblocking(){
    _fd=::socket(AF_INET,SOCK_STREAM,0);
    if(_fd<0){
        // LOG_FATAL("create socket error");
        exit(0);
    }
}

void Socket::createNonblocking(){
    _fd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(_fd<0){
        // LOG_FATAL("create socket nonblocking error");
        exit(0);
    }
}

void Socket::bind(uint16_t port,const char* ip){
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    if(!strcmp(ip,"INADDR_ANY")){
        addr.sin_addr.s_addr=INADDR_ANY;
    }else{
        addr.sin_addr.s_addr=inet_addr(ip);
    }
    addr.sin_port=htons(port);

    if(0!=::bind(_fd,(sockaddr*)&addr,sizeof(addr))){
        // LOG_FATAL("bind socket:%d failed",_fd);
        exit(0);
    }
}

void Socket::listen(){
    if(0!=::listen(_fd,SOMAXCONN)){
        // LOG_FATAL("listen socket:%d failed",_fd);
        exit(0);
    }
}

void Socket::accept(int &clientFd){
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    bzero(&addr,addrLen);
    clientFd=::accept4(_fd,(sockaddr*)&addr,&addrLen,SOCK_NONBLOCK|SOCK_CLOEXEC);
}

// 客户端连接使用,传入需连接的服务器地址
void Socket::connect(const char* ip,uint16_t port){
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if(0!=::connect(_fd,(sockaddr*)&addr,sizeof(addr))){
        // LOG_FATAL("connect failed");
        exit(0);
    }
}

void Socket::shutdownWrite()
{
    if (::shutdown(_fd, SHUT_WR) < 0)
    {
        // LOG_ERROR("shutdownWrite error");
        fprintf(stderr, "shutdown failed: %s\n", strerror(errno));
    }
}

void Socket::setNonBlocking(){
    if(fcntl(_fd,F_SETFL,fcntl(_fd,F_GETFL)|O_NONBLOCK)==-1){
        // LOG_ERROR("setNonBlocking error");
    }
}

bool Socket::isNonBlocking() const{
    return (fcntl(_fd,F_GETFL)&O_NONBLOCK) != 0;
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}