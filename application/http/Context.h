#pragma once
#include <iostream>
#include <memory>
#include <regex>

#include "http/Util.h"
#include "Utils.h"
#include "Buffer.h"
#include "Connection.h"
#include "engine.h"

#define MAX_LINE 8192

enum class ContextStatu{
    ERROR,  // 错误发生,需要发送错误信息报文
    WAITING,    // 接收不完整
    OK,     // 接收完整
};

enum class HttpRecvStatus{
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER,
    RECV_HTTP_ERROR,
};

class HttpContext:public Context{
public:
    HttpContext():_recvStatuStr(HttpRecvStatus::RECV_HTTP_LINE),_statuCode(200){}
    ~HttpContext(){}

    ContextStatu readAndParseRecvInfo(Buffer* buf){
        // 设置_statuCode _recvStatuStr
        parseRequest(*buf);
        if(_recvStatuStr==HttpRecvStatus::RECV_HTTP_ERROR){
            // 错误发生
            return ContextStatu::ERROR;
        }
        else if(_recvStatuStr!=HttpRecvStatus::RECV_HTTP_OVER){
            return ContextStatu::WAITING;
        }
        return ContextStatu::OK;
    }

    void writeAndSendInfo(std::string& msg,bool errorDeal=false){
        if(errorDeal){
            // 错误处理
            _body.clear();
            _body += "<html>";
            _body += "<head>";
            _body += "<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";
            _body += "</head>";
            _body += "<body>";
            _body += "<h1>";
            _body += std::to_string(_statuCode);
            _body += " ";
            _body += Util::getStatusInfo(_statuCode);
            _body += "</h1>";
            _body += "</body>";
            _body += "</html>";
            setSendHeader("Content-Type","text/html");
        }
        // 长短连接
        if(_shortConn){
            // 长连接
            setSendHeader("Connection","close");
        }
        else{
            setSendHeader("Connection","keep-alive");
        }
        // Content_length
        if(!_body.empty() && !checkSendHeader("Content-Length")){
            setSendHeader("Content-Length",std::to_string(_body.size()));
        }
        // application
        if(!_body.empty() && !checkSendHeader("Content-Type")){
            setSendHeader("Content-Type","application/octet-stream");
        }

        msg+=_version; msg+=" ";
        msg+=std::to_string(_statuCode); msg+=" ";
        msg+=Util::getStatusInfo(_statuCode); msg+="\r\n";

        for(auto& header:_sendHeader){
            msg+=header.first; msg+=": "; msg+=header.second; msg+="\r\n";
        }
        msg+="\r\n";
        msg += _body;
        return;
    }

    void reset(){
        _recvStatuStr=HttpRecvStatus::RECV_HTTP_LINE;
        _params.clear();
        _recvHeader.clear();
        _sendHeader.clear();
        _body.clear();
        _statuCode=200;
    }

    bool isShortConn() const{
        return _shortConn;
    }

    std::string getMethod() const{
        return _method;
    }

    std::string getPath() const{
        return _path;
    }

    void setStatuCode(int code){
        _statuCode=code;
    }

    void setBody(std::string& msg){
        _body=msg;
    }

private:
    void parseRequest(Buffer& buf){
        switch (_recvStatuStr)
        {
        case HttpRecvStatus::RECV_HTTP_LINE:
            if(!recvHttpLine(buf)){
                return;
            }
        case HttpRecvStatus::RECV_HTTP_HEAD:
            if(!recvHttpHead(buf)){
                return;
            }
        case HttpRecvStatus::RECV_HTTP_BODY:
            recvHttpBody(buf);
        }
        return;
    }

    bool recvHttpLine(Buffer& buf){
        std::string line=buf.getLineAndMove();
        if(line.size()==0){
            if(buf.getReadableBytes()>MAX_LINE){
                // 很长了都不足一行
                _statuCode=414;
                _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
                return false;
            }
            // 正常等待后续到来
            return true;
        }
        if(line.size()>MAX_LINE){
            _statuCode=414;
            _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
            return false;
        }
        if(!parseHttpLine(line)){
            return false;
        }
        _recvStatuStr=HttpRecvStatus::RECV_HTTP_HEAD;
        return true;
    }

    bool recvHttpHead(Buffer& buf){
        if(_recvStatuStr!=HttpRecvStatus::RECV_HTTP_HEAD){
            return false;
        }
        while(true){
            std::string line=buf.getLineAndMove();
            if(line.size()==0){
                if(buf.getReadableBytes()>MAX_LINE){
                    // 很长了都不足一行
                    _statuCode=414;
                    _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
                    return false;
                }
                // 正常等待后续到来
                return true;
            }
            if(line.size()>MAX_LINE){
                _statuCode=414;
                _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
                return false;
            }
            if(line=="\n"||line=="\r\n"){
                break;
            }
            if(!parseHttpHead(line)){
                return false;
            }
        }
        if((!checkRecvHeader("Connection")&&_version=="HTTP/1.1") || getRecvHeader("Connection")=="keep-alive"){
            // 长连接
            _shortConn=false;
        }
        else{
            _shortConn=true;
        }
        _recvStatuStr=HttpRecvStatus::RECV_HTTP_BODY;
        return true;
    }

    bool recvHttpBody(Buffer& buf){
        if(_recvStatuStr!=HttpRecvStatus::RECV_HTTP_BODY){
            return false;
        }

        size_t contentSize=0;
        std::string contentSizeStr=getRecvHeader("Content-Length");
        if(contentSizeStr.size()==0){
            contentSize=0;
            _recvStatuStr=HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        contentSize=stol(contentSizeStr);

        size_t need2recv=contentSize-_body.size();
        if(buf.getReadableBytes()>=need2recv){
            // 需要的内容全部接收到了
            _body+=buf.readStringAndMove(need2recv);
            _recvStatuStr=HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        _body+=buf.readStringAndMove(buf.getReadableBytes());
        return true;
    }

    bool parseHttpLine(const std::string& line){
        std::smatch matches;
        std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?",std::regex::icase);
        if(!std::regex_match(line,matches,e)){
            _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
            _statuCode=400;    // BAD REQUEST
            return false;
        }
        _method=matches[1];
        std::transform(_method.begin(), _method.end(), _method.begin(), ::toupper);
        _path=Util::urlDecode(matches[2], false);
        _version=matches[4];

        std::string queryStr=matches[3];
        std::vector<std::string> query;
        Util::usplit(queryStr, "&", &query);
        for(auto& item:query){
            size_t pos=item.find("=");
            if(pos==std::string::npos){
                _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
                _statuCode=400;    // BAD REQUEST
                return false;
            }
            std::string key = Util::urlDecode(item.substr(0, pos), true);
            std::string val = Util::urlDecode(item.substr(pos + 1), true);
            setParam(key, val);
        }
        return true;
    }

    bool parseHttpHead(std::string& line){
        if(line.back()=='\n'){
            line.pop_back();
        }
        if(line.back()=='\r'){
            line.pop_back();
        }
        size_t pos=line.find(": ");
        if(pos==std::string::npos){
            _recvStatuStr=HttpRecvStatus::RECV_HTTP_ERROR;
            _statuCode=400;
            return false;
        }
        std::string key=line.substr(0,pos);
        std::string val=line.substr(pos+2);
        setRecvHeader(key,val);
        return true;
    }

    void setRecvHeader(const std::string& key,const std::string& val){
        _recvHeader[key]=val;
    }

    bool checkRecvHeader(const std::string& key){
        return _recvHeader.find(key)!=_recvHeader.end();
    }

    std::string getRecvHeader(const std::string& key) const{
        auto it=_recvHeader.find(key);
        return (it==_recvHeader.end())?"":it->second;
    }

    void setSendHeader(const std::string& key,const std::string& val){
        _sendHeader[key]=val;
    }

    bool checkSendHeader(const std::string& key){
        return _sendHeader.find(key)!=_sendHeader.end();
    }

    std::string getSendHeader(const std::string& key) const{
        auto it=_sendHeader.find(key);
        return (it==_sendHeader.end())?"":it->second;
    }

    void setParam(const std::string& key,const std::string& val){
        _params[key]=val;
    }

    bool checkParam(const std::string& key){
        return _params.find(key)!=_params.end();
    }

    std::string getParam(const std::string& key) const{
        auto it=_params.find(key);
        return (it==_params.end())?"":it->second;
    }


private:
    HttpRecvStatus _recvStatuStr;
    int _statuCode;
    bool _shortConn;

    std::string _method;
    std::string _path;
    std::string _version;
    std::unordered_map<std::string,std::string> _params;
    std::unordered_map<std::string,std::string> _recvHeader;
    std::unordered_map<std::string,std::string> _sendHeader;
    std::string _body;
};