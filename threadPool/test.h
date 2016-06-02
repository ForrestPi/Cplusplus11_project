#ifndef TEST_H
#define TEST_H

#include "ThreadPool.h"

void TestTheadPool(){
    ThreadPool pool;
    pool.Start(2);
    std::thread thd1([&pool]{
        for(int i=0;i<10;i++){
            auto threadId = this_thread::get_id();
            pool.AddTask([threadId]{
                std::cout<<"The first thread's ID:"<<threadId<<std::endl;
            });
        }
    });
    std::thread thd2([&pool]{
        for(int i=0;i<10;i++){
            auto threadId = this_thread::get_id();
            pool.AddTask([threadId]{
                std::cout<<"The Second thread's ID:"<<threadId<<std::endl;
            });
        }
    });
    this_thread::sleep_for(std::chrono::seconds(2));
    getchar();
    pool.Stop();
    thd1.join();
    thd2.join();
}

#endif // TEST_H
