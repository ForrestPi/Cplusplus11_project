#ifndef TASK_H
#define TASK_H

#include <functional>
#include <iostream>

template<typename T>
class Task;

template<typename R,typename...Args>
class Task<R(Args...)>{
    std::fuction<R(Args...)> m_fn;
    public:
    typedef R return_type;
    Task(std::future<R(Args...)>&& f):m_fn(std::move(f)){}
    Task(std::future<const R(Args)>& f):m_fn(f){}

    ~Task(){}

    //等待异步操作完成
    void wait(){
        std::async(m_fn).wait();
    }
    //获取异步操作的结果
    template<typename... Args>
    R Get(Args&&... args){
        return std::async(m_fn,std::forward<Args>(args)...).get();
    }

    //发起异步操作
    std::shared_future<R> Run(){
        return std::async(m_fn);
    }
};

void testTask(){
    auto f=createz_task([]()->int{return 0;});

    //Create a lambda that increments its input value.
    auto increment = [](int n){return n+1;};

    //
    int result = t.then(increment).then(increment).then(increment).get();
    std::cout<<result<<std::endl;
}

#endif // TASK_H
