#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include "SyncQueue.h"

const int MaxTaskCount=100;
class ThreadPool{
public:
    using Task = std::function<void()>;
private:
    std::list<std::shared_ptr<std::thread>>m_threadgroup;
    SyncQueue<Task> m_queue;
    atomic_bool m_running;
    std::once_flag m_flag;
public:
    ThreadPool(int numThreads=std::thread::hardware_concurrency()):m_queue(MaxTaskCount){
        Start(numThreads);
    }
    ~ThreadPool(){
        Stop();
    }
    void Stop(){
        std::call_once(m_flag,[this]{StopThreadGroup();});
    }
    void AddTask(const Task& task){
        m_queue.Put(task);
    }

    void Start(int numThreads){
        m_running=true;
        for(int i=0;i<numThreads;i++){
            m_threadgroup.push_back(std::make_shared<std::thread>(&ThreadPool::RunInThread,this));
        }
    }
private:
    void RunInThread(){
        while(m_running){
            std::list<Task>list;
            m_queue.Take(list);
            for(auto& task:list){
                if(!m_running)
                    return;
                task();
            }
        }
    }

    void StopThreadGroup(){
        m_queue.Stop();
        m_running=false;
        for(auto thread:m_threadgroup){
            if(thread)
                thread->join();
        }
        m_threadgroup.clear();
    }
};

#endif // THREADPOOL_H
