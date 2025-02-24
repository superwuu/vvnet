#include <iostream>

#include "http/HttpCore.h"
#include "vvlog/vvlog.h"

// 中间件
void middle1(Context* ctx){
    std::cout<<"middle1 before"<<std::endl;
    ctx->next();
    std::cout<<"middle1 after"<<std::endl;
}

void middle2(Context* ctx){
    std::cout<<"middle2 before"<<std::endl;
    ctx->next();
    std::cout<<"middle2 after"<<std::endl;
}

int main(){
    HttpServer* server=new HttpServer(12345);   // 初始化服务器

    LOG_INIT("log", "vvnet", INFO); //日志init

    server->Get("/",[](Context* ctx){
        LOG_INFO("hello HUST");
        std::cout<<"hello HUST"<<std::endl;
        return "hello HUST!";
    });

    server->use(middle1);
    server->use(middle2);    // 使用中间件

    server->setThreadNum(4);    // 设置多线程

    server->useTimer(10);   // 启动定时器

    Signal::signal(SIGINT, [&] {
        std::cout << "\nServer exit!" << std::endl;
        delete server;
        exit(0);
    });

    server->spin();

    return 0;
}