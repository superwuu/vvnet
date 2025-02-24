#include "Buffer.h"

#include <iostream>

#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

Buffer::Buffer(size_t initSize)
    :_buf(initSize),_readerIndex(0),_writerIndex(0){}

Buffer::~Buffer(){}

void Buffer::reInit(){
    _readerIndex=0;
    _writerIndex=0;
}

size_t Buffer::getReadableBytes() const{
    return _writerIndex-_readerIndex;
}
size_t Buffer::getWritableBytes() const{
    return _buf.size()-_writerIndex;
}

size_t Buffer::getHeaderBytes() const{
    return _readerIndex;
}

char* Buffer::getReadableBegin(){
    return begin()+_readerIndex;
}

const char* Buffer::getReadableBegin() const{
    return begin()+_readerIndex;
}

char* Buffer::getWritableBegin(){
    return begin()+_writerIndex;
}

char* Buffer::begin(){
    return &*_buf.begin();
}

const char* Buffer::begin() const{
    return &*_buf.begin();
}

void Buffer::ensureWritableBytes(size_t len){
    if(getWritableBytes()<len){
        makespace(len);
    }
}

void Buffer::makespace(size_t len){
    if(len>getWritableBytes()+getHeaderBytes()){
        // 要写的空间比剩余空间还多，则扩容
        _buf.resize(_writerIndex+len);
    }
    else{
        // 可以放得下
        size_t readable=getReadableBytes();
        std::copy(getReadableBegin(),getWritableBegin(),begin());
        _readerIndex=0;
        _writerIndex=_readerIndex+readable;
    }
}

void Buffer::moveReaderIndex(size_t len){
    assert(len<=getReadableBytes());
    _readerIndex+=len;
}
void Buffer::moveWriterIndex(size_t len){
    assert(len<=getWritableBytes());
    _writerIndex+=len;
}

size_t Buffer::readFd(int fd,int* saveErrno){
    char extraBuf[65536]={0};
    struct iovec vec[2];

    const size_t writable=getWritableBytes();

    vec[0].iov_base=getWritableBegin();
    vec[0].iov_len=writable;

    vec[1].iov_base=extraBuf;
    vec[1].iov_len=sizeof(extraBuf);

    const int cnt=(writable<sizeof(extraBuf))?2:1;

    const ssize_t n=readv(fd,vec,cnt);
    if(n<0){
        *saveErrno=errno;
    }
    else if(n<=writable){
        // 第一块能读，已经进来了
        _writerIndex+=n;
    }
    else{
        _writerIndex=_buf.size();
        writeBufAndMove(extraBuf,n-writable);
    }

    return n;
}

size_t Buffer::writeFd(int fd,int* saveErrno){
    ssize_t n=write(fd,getReadableBegin(),getReadableBytes());
    // std::cout<<"send "<<n<<std::endl;
    if(n<0){
        *saveErrno=errno;
    }
    return n;
}


// write
void Buffer::writeBuf(const void* data,size_t len){
    ensureWritableBytes(len);
    const char* d=(const char*)data;
    std::copy(d,d+len,getWritableBegin());
}

void Buffer::writeBufAndMove(const void* data,size_t len){
    writeBuf(data,len);
    moveWriterIndex(len);
}

void Buffer::writeString(const std::string& data){
    return writeBuf(data.c_str(),data.size());
}

void Buffer::writeStringAndMove(const std::string& data){
    return writeBufAndMove(data.c_str(),data.size());
}

// read

void Buffer::readBuf(void* buf,size_t len){
    assert(len<=getReadableBytes());
    std::copy(getReadableBegin(),getReadableBegin()+len,(char*)buf);
}
void Buffer::readBufAndMove(void* buf,size_t len){
    readBuf(buf,len);
    moveReaderIndex(len);
}

std::string Buffer::readString(size_t len){
    assert(len<=getReadableBytes());
    std::string res;
    res.resize(len);
    readBuf(&res[0],len);
    return res;
}
std::string Buffer::readStringAndMove(size_t len){
    assert(len<=getReadableBytes());
    std::string res=readString(len);
    moveReaderIndex(len);
    return res;
}

// function
std::string Buffer::getLine(){
    char* pos=(char*)memchr(getReadableBegin(),'\n',getReadableBytes());
    if(pos==nullptr){
        // LOG_INFO("[net] Not Find \\n");
        return "";
    }
    // LOG_INFO("[net] Find \\n");
    return readString(pos-getReadableBegin()+1);
}

std::string Buffer::getLineAndMove(){
    std::string res=getLine();
    // LOG_INFO("[net] Read Line:%s",res.c_str());
    // LOG_INFO("[net] Before move readerIndex:%ld ,writerIndex:%ld",_readerIndex,_writerIndex);
    moveReaderIndex(res.size());
    // LOG_INFO("[net] After move readerIndex:%ld ,writerIndex:%ld",_readerIndex,_writerIndex);
    return res;
}

std::string Buffer::readAll(){
    return readStringAndMove(getReadableBytes());
}