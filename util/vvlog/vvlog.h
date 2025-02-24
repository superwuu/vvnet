#pragma once

#include <iostream>

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>//getpid, gettid

enum LOG_LEVEL{
    FATAL=1,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE
};

extern pid_t gettid();

class Data_buffer{
public:
    enum BUFFER_STATUS{
        FREE,
        FULL
    };

    Data_buffer(uint32_t len):_status(FREE),next(nullptr),prev(nullptr),_allsize(len),_usedsize(0){
        _data=new char[len];
        if(!_data){
            exit(1);
        }
    }

    uint32_t getUsableSize() const{
        return _allsize-_usedsize;
    }

    bool empty() const{
        return _usedsize==0;
    }

    void append(const char* msg,uint32_t len){
        if(getUsableSize()<len){
            return;
        }
        memcpy(_data+_usedsize,msg,len);
        _usedsize+=len;
    }

    void clear(){
        _usedsize=0;
        _status=FREE;
    }

    void persist(FILE* fp){
        uint32_t writelen=fwrite(_data,sizeof(char),_usedsize,fp);
        if(writelen!=_usedsize){
            fprintf(stderr, "write log to disk error, wt_len %u\n", writelen);
        }
    }

    BUFFER_STATUS _status;
    Data_buffer* next;
    Data_buffer* prev;
private:
    Data_buffer(const Data_buffer&);
    Data_buffer& operator=(const Data_buffer&);

    uint32_t _allsize;
    uint32_t _usedsize;
    char* _data;
};


struct UtcTimer{
    UtcTimer(){
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        _sysAccSec=tv.tv_sec;
        _sysAccMin=_sysAccSec/60;

        struct tm curTm;
        localtime_r((time_t*)&_sysAccSec,&curTm);
        _year=curTm.tm_year+1900;
        _mon=curTm.tm_min+1;
        _day=curTm.tm_mday;
        _hour=curTm.tm_hour;
        _min=curTm.tm_min;
        _sec=curTm.tm_sec;
        resetUtcFmt();
    }

    uint64_t getCurrTime(int* sec){
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        if(sec){
            *sec=tv.tv_usec/1000;
        }
        if((uint32_t)tv.tv_sec!=_sysAccSec){
            _sec=tv.tv_sec%60;
            _sysAccSec=tv.tv_sec;
            if(_sysAccSec/60!=_sysAccMin){
                _sysAccMin=_sysAccSec/60;
                struct tm curTm;
                localtime_r((time_t*)&_sysAccSec,&curTm);
                _year=curTm.tm_year+1900;
                _mon=curTm.tm_min+1;
                _day=curTm.tm_mday;
                _hour=curTm.tm_hour;
                _min=curTm.tm_min;
                resetUtcFmt();
            }
            else{
                resetUtcFmtSec();
            }
        }
        return tv.tv_sec;
    }

    void resetUtcFmt(){
        snprintf(_utcFmt, 20, "%d-%02d-%02d %02d:%02d:%02d", _year, _mon, _day, _hour, _min, _sec);
    }
    void resetUtcFmtSec(){
        snprintf(_utcFmt+17,3,"%02d",_sec);
    }

    int _year,_mon,_day,_hour,_min,_sec;
    uint64_t _sysAccMin;
    uint64_t _sysAccSec;
    char _utcFmt[20];
};

class Vvlog{
public:
    static Vvlog* getInstance(){
        pthread_once(&_once,Vvlog::init);
        return _ins;
    }
    
    static void init(){
        while(!_ins){
            _ins=new Vvlog();
        }
    }

    int getLevel() const{
        return _level;
    }

    void initPath(const char* logDir,const char* name,int level);

    void persist();

    void append(const char* level,const char* fmt,...);

private:
    Vvlog();

    bool getFile(int year,int mon,int day);

private:
    static Vvlog* _ins;
    static pthread_once_t _once;

private:
    FILE* _fp;
    Data_buffer* _currBuf;
    Data_buffer* _persistBuf;

    pid_t _pid;

    UtcTimer _tm;
    uint64_t _lastLogTime;

    int _bufferCnt;

    int _year,_mon,_day,_logCnt;
    char _name[128];
    char _logDir[512];

    int _level;
    bool _envOk;

    static pthread_mutex_t _mutex;
    static pthread_cond_t _cond;

    static uint32_t _bufferSize;

};

void* be_thdo(void* args);

#define LOG_MEM_SET(mem_lmt) \
    do \
    { \
        if (mem_lmt < 90 * 1024 * 1024) \
        { \
            mem_lmt = 90 * 1024 * 1024; \
        } \
        else if (mem_lmt > 1024 * 1024 * 1024) \
        { \
            mem_lmt = 1024 * 1024 * 1024; \
        } \
        ring_log::_bufferSize = mem_lmt; \
    } while (0)

#define LOG_INIT(log_dir,name,level) \
    do \
    { \
        Vvlog::getInstance()->initPath(log_dir,name,level); \
        pthread_t tid; \
        pthread_create(&tid,nullptr,be_thdo,nullptr); \
        pthread_detach(tid); \
    }while(0) \

#define LOG_TRACE(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= TRACE) \
        { \
            Vvlog::getInstance()->append("[TRACE]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_DEBUG(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= DEBUG) \
        { \
            Vvlog::getInstance()->append("[DEBUG]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_INFO(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= INFO) \
        { \
            Vvlog::getInstance()->append("[INFO]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_NORMAL(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= INFO) \
        { \
            Vvlog::getInstance()->append("[INFO]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_WARN(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= WARN) \
        { \
            Vvlog::getInstance()->append("[WARN]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_ERROR(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= ERROR) \
        { \
            Vvlog::getInstance()->append("[ERROR]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define LOG_FATAL(fmt,args...) \
    do \
    { \
        Vvlog::getInstance()->append("[FATAL]", "[%u]%s:%d(%s): " fmt "\n", \
                gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0)

#define TRACE(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= TRACE) \
        { \
            Vvlog::getInstance()->append("[TRACE]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define DEBUG(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= DEBUG) \
        { \
            Vvlog::getInstance()->append("[DEBUG]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define INFO(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= INFO) \
        { \
            Vvlog::getInstance()->append("[INFO]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define NORMAL(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= INFO) \
        { \
            Vvlog::getInstance()->append("[INFO]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)

#define WARN(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= WARN) \
        { \
            Vvlog::getInstance()->append("[WARN]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)


#define ERROR(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= ERROR) \
        { \
            Vvlog::getInstance()->append("[ERROR]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)


#define FATAL(fmt,args...) \
    do \
    { \
        if (Vvlog::getInstance()->getLevel() >= FATAL) \
        { \
            Vvlog::getInstance()->append("[FATAL]", "[%u]%s:%d(%s): " fmt "\n", \
                    gettid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
        } \
    } while (0)
