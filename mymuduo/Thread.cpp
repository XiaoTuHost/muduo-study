#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

Thread::Thread(threadFunc func,const std::string&name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{

}

Thread::~Thread(){
    if(!started_ && !joined_){
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

void Thread::setDefaultName(){
    int num = ++ numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_=buf;
    }
}

void Thread::start(){
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid(); // 获取线程的tid值
        sem_post(&sem);
        func_(); // 开启一个新线程，准们执行该线程的函数
    }));

    // 这里必须等待获取上面新创建的tid值
    sem_wait(&sem);
}
