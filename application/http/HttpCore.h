#pragma once

#include <iostream>

#include "Context.h"
#include "route.h"

#include "TcpServer.h"
#include "Connection.h"
#include "Socket.h"
#include "Buffer.h"
#include "Acceptor.h"

class HttpServer{
public:
    HttpServer(uint16_t port=8080){
        _server=std::make_unique<TcpServer>(port);
        std::vector<std::string> name{"GET","POST"};
        _route=std::make_unique<Route>(name);
        makeCallback();
    }

    void useTimer(int interval){
        _server->useTimer(interval);
    }

    // 中间件层
    void use(const MiddlewareHandler& hander){
        _middleware.push_back(hander);
    }

    // 路由层
    void Get(const std::string& pattern,const Handler& handler){
        _route->Add("GET",pattern,handler);
    }
    void Post(const std::string& pattern,const Handler& handler){
        _route->Add("POST",pattern,handler);
    }

    // 网络层
    void setThreadNum(int num){
        _server->setThreadNum(num);
    }
    
    void makeCallback(){
        _server->setOnConnectCallback([&](std::shared_ptr<Connection> conn){
            if(conn->getState()==Connection::kConnected){
                conn->setContext(HttpContext());
            }
        });
        
        _server->setOnMessageCallback([&](std::shared_ptr<Connection> conn){
            Buffer* buffer=conn->getReadBuffer();
            HttpContext* context=conn->getContext()->get<HttpContext>();
            std::string msg;

            ContextStatu ret=context->readAndParseRecvInfo(buffer);
            if(ret==ContextStatu::ERROR){
                if(_sendErrorMsg){
                    context->writeAndSendInfo(msg,true);
                    conn->send(msg);
                }
                conn->shutdown();
                return;
            }
            else if(ret==ContextStatu::WAITING){
                return;
            }

            // 接收完毕
            // route
            context->_handler=_route->route(context->getMethod(),context->getPath());

            if(context->_handler==nullptr){
                std::cout<<"here"<<std::endl;
                // 没找到
                if(_sendErrorMsg){
                    context->setStatuCode(404);
                    context->writeAndSendInfo(msg,true);
                    conn->send(msg);
                }
                conn->shutdown();
                return;
            }

            // 中间件层
            if(context->_middleware.empty()){
                for(const auto&func:_middleware){
                    context->_middleware.push_back(func);
                    context->_middlewareCount++;
                }
            }

            std::string body=context->run();
            context->setBody(body);

            context->writeAndSendInfo(msg);
            conn->send(msg);
            if(context->isShortConn()){
                // 短连接
                conn->shutdown();
                return;
            }
            else{
                // std::cout<<"long connect"<<std::endl;
            }
            context->reset();
            
        });
    }

    void run(HttpContext& context){

    }

    void spin(){
        _server->start();
    }

private:
    std::unique_ptr<TcpServer> _server;
    std::unique_ptr<Route> _route;
    std::vector<MiddlewareHandler> _middleware;

    int i=0;

    bool _sendErrorMsg=true;

};