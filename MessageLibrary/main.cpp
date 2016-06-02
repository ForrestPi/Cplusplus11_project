#include <iostream>
#include <functional>
using namespace std;
#include "function_traits.h"
#include "MessageBus.h"
#include "TestBus.h"

void test_function_traits(){
    auto f=to_function([](int i){return i;});
    std::function<int(int)> f1=[](int i){return i;};
    if(std::is_same<decltype(f),decltype(f1)>::value){
        cout<<"same"<<endl;
    }
    else{
        cout<<"Not same"<<endl;
    }
}

void TestMsgBus(){
    MessageBus bus;
    //注册信息
    bus.Attach([](int a){cout<<"no reference"<<a<<endl;});
    bus.Attach([](int& a){cout<<"lvalue reference"<<a<<endl;});
    bus.Attach([](int&& a){cout<<"rvalue reference"<<a<<endl;});
    bus.Attach([](const int& a){cout<<"const lvalue reference"<<a<<endl;});
    bus.Attach([](int a){cout<<"no reference has return value and key"<<a<<endl;return a;},"a");

    int i=2;
    //发送信息
    bus.SendReq<void,int>(2);
    bus.SendReq<int,int>(2,"a");
    bus.SendReq<void,int&>(i);
    bus.SendReq<void,const int&>(2);
    bus.SendReq<void,int&&>(2);

    //移除消息
    bus.Remove<void,int>();
    bus.Remove<int,int>("a");
    bus.Remove<void,int&>();
    bus.Remove<void,const int&>();
    bus.Remove<void,int&&>();

    //发送信息
    bus.SendReq<void,int>(2);
    bus.SendReq<int,int>(2,"a");
    bus.SendReq<void,int&>(i);
    bus.SendReq<void,const int&>(2);
    bus.SendReq<void,int&&>(2);
}

int main()
{
    //TestMsgBus();
    TestBus();
    return 0;
}

