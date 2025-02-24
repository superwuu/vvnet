#pragma once

#include "Common.h"

#include <string>
#include <memory>
#include <vector>

class Buffer:noncopyable{
public:
    static const size_t cInitSize=1024; // 初始大小
    
    explicit Buffer(size_t initSize=cInitSize);
    ~Buffer();

    void reInit();

    size_t getReadableBytes() const;    // 获取可读字节数 writerindex-readerindex
    size_t getWritableBytes() const;    // 获取可写字节数 _buf.size()-writerindex
    size_t getHeaderBytes() const;     // 获取前部字节数 _readerIndex 

    char* getReadableBegin();   // 获取可读起始位置 _buf.data()+readerindex
    const char* getReadableBegin() const;   // 获取可读起始位置 _buf.data()+readerindex
    char* getWritableBegin();   // 获取可写起始位置 _buf.data()+writerindex

    void ensureWritableBytes(size_t len);   // 确保可写入数据长度大于等于len
    
    void moveReaderIndex(size_t len);
    void moveWriterIndex(size_t len);

    void writeBuf(const void* data,size_t len);
    void writeBufAndMove(const void* data,size_t len);
    void writeString(const std::string& data);
    void writeStringAndMove(const std::string& data);

    void readBuf(void* buf,size_t len);
    void readBufAndMove(void* buf,size_t len);
    std::string readString(size_t len);
    std::string readStringAndMove(size_t len);

    size_t readFd(int fd,int* saveErrno);   // 读取tcp fd数据
    size_t writeFd(int fd,int* saveErrno);  // 写入tcp fd数据

public:
// function
    std::string getLine();
    std::string getLineAndMove();
    std::string readAll();
    
private:
    std::vector<char> _buf; // 缓冲区
    size_t _readerIndex;    // 读索引
    size_t _writerIndex;    // 写索引,初始化时两者同一位置

    char* begin();  // 缓冲区起始位置
    const char* begin() const;

    void makespace(size_t len); // 增加空间
};