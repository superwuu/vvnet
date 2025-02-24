#pragma once
#include <iostream>
#include <regex>
#include <vector>

#include "trietree.h"

#include "Utils.h"


class Route{
public:
    Route(std::vector<std::string> name){
        for(const auto& it:name){
            _routeTree[it]=std::make_unique<TrieNode>();
        }
    }
    ~Route(){
        _routeTree.clear();
    }

    void Add(const std::string& method,const std::string& pattern,const Handler& handler){
        std::unique_ptr<TrieNode>& tree=_routeTree[method];
        tree->add(pattern,handler);
        return;
    }

    Handler route(const std::string method,const std::string path){
        Handler handler=nullptr;
        std::unique_ptr<TrieNode>& tree=_routeTree[method];
        bool ret=tree->matchRoute(path,handler);
        if(!ret){
            // 没找到
            return nullptr;
        }
        return handler;
    }

private:
    std::unordered_map<std::string,std::unique_ptr<TrieNode>> _routeTree;    
};