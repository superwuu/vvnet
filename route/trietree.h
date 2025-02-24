#pragma once
#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
#include <sstream>
#include <string>

class Context;

using Handler=std::function<std::string(Context*)>;

class TrieNode {
public:
    // 子节点映射
    std::unordered_map<std::string, std::unique_ptr<TrieNode>> children;
    // 如果该节点是终点，则关联一个处理器
    Handler handler;

    // 添加子节点
    void add(const std::string& _path, const Handler& h) {
        std::string path = removeLeadingSlash(_path);
        if (path.empty()) {
            handler = h;
            return;
        }
        auto pos = path.find('/');
        std::string segment = (pos == std::string::npos) ? path : path.substr(0, pos);
        std::string rest = (pos == std::string::npos) ? "" : path.substr(pos + 1);

        if (!children.count(segment)) {
            children[segment] = std::make_unique<TrieNode>();
        }

        if(segment[0]=='*'){
            children[segment]->handler = h;
        }
        else{
            children[segment]->add(rest, h);
        }
    }

    // 查找并调用处理器
    bool matchRoute(const std::string& _path,Handler& h) {
        std::string path = removeLeadingSlash(_path);
        std::istringstream iss(path);
        std::string segment;
        TrieNode* current = this;

        if(path.empty() && current->handler){
            h=current->handler;
            return true;
        }

        while (std::getline(iss, segment, '/')) {
            bool found = false;
            if (current->children.count(segment)) {
                found=true;
                current = current->children[segment].get();
            } else {
                // 尝试匹配参数
                for (auto& [key, value] : current->children) {
                    if (key[0] == ':') {
                        std::string k=key.substr(1);
                        // std::cout<<k<<": "<<segment<<std::endl;
                        current = value.get();
                        found=true;
                        break;
                    }
                }
                if(!found){
                    // 找通配路由
                    for (auto& [key, value] : current->children) {
                        if (key[0] == '*') {
                            current = value.get();

                            std::ostringstream remainingPath;
                            remainingPath << segment;
                            std::string temp;
                            while (std::getline(iss, temp, '/')) {
                                remainingPath << '/' << temp;
                            }
                            std::string k=key.substr(1);
                            // std::cout<<k<<": "<<remainingPath.str()<<std::endl;
                            found=true;
                            break;
                        }
                    }
                }
            }
            if (found && current->handler) {
                h=current->handler;
                return true;
            }
        }
        return false;
    }
private:
    static std::string removeLeadingSlash(const std::string& path) {
        if (!path.empty() && path[0] == '/') {
            return path.substr(1);
        }
        return path;
    }
};