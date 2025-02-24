#include "vvlog.h"

#include <stdarg.h>//va_list
#include <unistd.h>//access, getpid
#include <sys/stat.h>//mkdir
#include <sys/syscall.h>//system call

#define LOG_LEN_LIMIT (4 * 1024)//4K
#define RELOG_THRESOLD 5
#define BUFFER_WAIT_TIME 1

pid_t gettid(){
    return syscall(__NR_gettid);
}

pthread_mutex_t Vvlog::_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Vvlog::_cond=PTHREAD_COND_INITIALIZER;

Vvlog* Vvlog::_ins=nullptr;
pthread_once_t Vvlog::_once=PTHREAD_ONCE_INIT;

uint32_t Vvlog::_bufferSize=30*1024*1024;

Vvlog::Vvlog():_bufferCnt(3),_currBuf(nullptr),_persistBuf(nullptr),_fp(nullptr),_logCnt(0),_level(INFO),_envOk(false),_tm(),_lastLogTime(0){
    Data_buffer* head=new Data_buffer(_bufferSize);
    if(!head){
        exit(1);
    }
    Data_buffer* curr;
    Data_buffer* prev=head;
    for(int i=1;i<_bufferCnt;++i){
        curr=new Data_buffer(_bufferSize);
        if(!curr){
            exit(1);
        }
        curr->prev=prev;
        prev->next=curr;
        prev=curr;
    }
    prev->next=head;
    head->prev=prev;

    _currBuf=head;
    _persistBuf=head;

    _pid=getpid();
}

void Vvlog::initPath(const char* logDir,const char* name,int level){
    pthread_mutex_lock(&_mutex);
    strncpy(_logDir,logDir,512);
    strncpy(_name,name,128);

    mkdir(_logDir,0777);
    if(access(_logDir,F_OK|W_OK)==-1){
        fprintf(stderr, "logdir: %s error: %s\n", _logDir, strerror(errno));
    }
    else{
        _envOk=true;
    }
    if(level>TRACE){
        level=TRACE;
    }
    if(level<FATAL){
        level=FATAL;
    }
    _level=level;
    pthread_mutex_unlock(&_mutex);
}

void Vvlog::persist(){
    while(true){
        pthread_mutex_lock(&_mutex);
        if(_persistBuf->_status==Data_buffer::FREE){
            struct timespec tsp;
            struct timeval now;
            gettimeofday(&now,nullptr);
            tsp.tv_sec=now.tv_sec;
            tsp.tv_nsec=now.tv_sec*1000;
            tsp.tv_sec+=BUFFER_WAIT_TIME;
            pthread_cond_timedwait(&_cond,&_mutex,&tsp);
        }
        if(_persistBuf->empty()){
            pthread_mutex_unlock(&_mutex);
            continue;
        }
        if(_persistBuf->_status==Data_buffer::FREE){
            _currBuf->_status=Data_buffer::FULL;
            _currBuf=_currBuf->next;
        }
        int year=_tm._year,mon=_tm._mon,day=_tm._day;
        pthread_mutex_unlock(&_mutex);
        if(!getFile(year,mon,day)){
            continue;
        }
        _persistBuf->persist(_fp);
        fflush(_fp);

        pthread_mutex_lock(&_mutex);
        _persistBuf->clear();
        _persistBuf=_persistBuf->next;
        pthread_mutex_unlock(&_mutex);
    }
}

void Vvlog::append(const char* level,const char* format,...){
    int ms;
    uint64_t currSec=_tm.getCurrTime(&ms);
    if(_lastLogTime && currSec-_lastLogTime<RELOG_THRESOLD){
        return;
    }
    char logLine[LOG_LEN_LIMIT];
    int len=snprintf(logLine,LOG_LEN_LIMIT,"%s[%s.%03d]",level,_tm._utcFmt,ms);
    
    va_list arg_ptr;
    va_start(arg_ptr, format);

    int mainLen=vsnprintf(logLine+len,LOG_LEN_LIMIT-len,format,arg_ptr);

    va_end(arg_ptr);

    len=mainLen+len;
    _lastLogTime=0;

    bool tellBackend=false;

    pthread_mutex_lock(&_mutex);
    if(_currBuf->_status==Data_buffer::FREE && _currBuf->getUsableSize()>len){
        _currBuf->append(logLine,len);
    }
    else{
        if(_currBuf->_status==Data_buffer::FREE){
            _currBuf->_status=Data_buffer::FULL;
            Data_buffer* nextBuf=_currBuf->next;
            tellBackend=true;
            if(nextBuf->_status==Data_buffer::FULL){
                Data_buffer* newBuf=new Data_buffer(_bufferSize);
                _bufferCnt++;
                newBuf->prev=_currBuf;
                _currBuf->next=newBuf;
                newBuf->next=nextBuf;
                nextBuf->prev=newBuf;
                _currBuf=newBuf;
            }
            else{
                _currBuf=nextBuf;
            }
            if(!_lastLogTime){
                _currBuf->append(logLine,len);
            }
        }
        else{
            // 当前满了
            _lastLogTime=currSec;
        }
    }
    pthread_mutex_unlock(&_mutex);
    if(tellBackend){
        pthread_cond_signal(&_cond);
    }
}

bool Vvlog::getFile(int year,int mon,int day){
    if(!_envOk){
        if(_fp){
            fclose(_fp);
        }
        _fp=fopen("/dev/null","w");
        return _fp!=nullptr;
    }
    if(!_fp){
        // 还没有打开文件
        _year=year,_mon=mon,_day=day;
        char logPath[1024]={};
        sprintf(logPath,"%s/%s.%d%02d%02d.%u.log",_logDir,_name,_year,_mon,_day,_pid);
        _fp=fopen(logPath,"w");
        if(_fp){
            _logCnt++;
        }
    }
    else if(_day!=day){
        // 跨天
        fclose(_fp);
        _year=year,_mon=mon,_day=day;
        char logPath[1024]={};
        sprintf(logPath,"%s/%s.%d%02d%02d.%u.log",_logDir,_name,_year,_mon,_day,_pid);
        _fp=fopen(logPath,"w");
        if(_fp){
            _logCnt++;
        }
    }
    return _fp!=nullptr;
}

void* be_thdo(void* args){
    Vvlog::getInstance()->persist();
    return nullptr;
}