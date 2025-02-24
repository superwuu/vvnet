#pragma once

#include <iostream>
#include <functional>

#include "route.h"

class HttpContext;

class Context{
public:
    Context(){}
    virtual ~Context(){}

    void next(){
        _middlewareIndex++;
        if(_middlewareIndex==_middlewareCount){
            _body=_handler(this);
        }
        else{
            _middleware[_middlewareIndex](this);
        }
        return;
    }

    std::string run(){
        while(_middlewareIndex<_middlewareCount){
            next();
        }
        return _body;
    }

public:
    Handler _handler;
    std::vector<MiddlewareHandler> _middleware;
    int _middlewareCount=0;
    int _middlewareIndex=-1;
    std::string _body;
};