#pragma once

#include "Common.h"

#include <string>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/tcp.h>

class Socket:noncopyable{
public:
    Socket();

    Socket(int fd);

    ~Socket();

    void setFd(int fd);

    int getFd() const;

    std::string getAddr() const;
    
    void createblocking();

    void createNonblocking();
    
    void bind(uint16_t port,const char* ip="INADDR_ANY");

    void listen();

    void accept(int &clientFd);

    // 客户端连接使用,传入需连接的服务器地址
    void connect(const char* ip,uint16_t port);

    void shutdownWrite();
    
    void setNonBlocking();

    bool isNonBlocking() const;

    void setTcpNoDelay(bool on);

    void setReuseAddr(bool on);

    void setReusePort(bool on);

    void setKeepAlive(bool on);

private:
    int _fd;
};