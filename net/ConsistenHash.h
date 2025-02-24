#pragma once

#include "Common.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <algorithm>
#include <unordered_map>

class ConsistenHash{
public:
    ConsistenHash(size_t dummySize,const functor_ul_string& hashFunc=std::hash<std::string>())
        :_dummySize(dummySize),_hashFunc(hashFunc)
    {}

    ~ConsistenHash()
    {}

    void addNode(const std::string& name,int index){
        std::lock_guard<std::mutex> _lock(_mutex);
        for(size_t i=0;i<_dummySize;++i){
            std::string dummyNode=name+"_make_hash_"+std::to_string(i);
            size_t hash=_hashFunc(dummyNode);
            while(_hashring.find(hash)!=_hashring.end()){
                dummyNode+="_conflict";
                hash=_hashFunc(dummyNode);
            }
            _hashring[hash]=index;
            _sortedHash.push_back(hash);
        }
        sort(_sortedHash.begin(),_sortedHash.end());
        return;
    }

    void deleteNode(int index){
        std::lock_guard<std::mutex> _lock(_mutex);
        for (const auto& pair : _hashring) {
            if (pair.second == index) {
                _hashring.erase(pair.first);
                auto it=find(_sortedHash.begin(),_sortedHash.end(),pair.first);
                if(it!=_sortedHash.end()){
                    _sortedHash.erase(it);
                }
            }
        }
        return;
    }

    int getNode(const std::string& key){
        std::lock_guard<std::mutex> _lock(_mutex);
        size_t hash=_hashFunc(key);
        auto it=std::upper_bound(_sortedHash.begin(),_sortedHash.end(),hash);
        if(it==_sortedHash.end()){
            it=_sortedHash.begin();
        }
        return _hashring[*it];
    }

private:
    size_t _dummySize;
    functor_ul_string _hashFunc;
    std::unordered_map<size_t,int> _hashring;   // {hash, thread_index}
    std::vector<size_t> _sortedHash;
    std::mutex _mutex;
};