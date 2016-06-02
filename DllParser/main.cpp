#include <iostream>
#include "DllParser.h"
using namespace std;
int main()
{
    DllParser dllClass;
    bool flag=dllClass.Load("./DllDemo.dll");
    if (!flag)
        return 0;

    auto hello = dllClass.ExcecuteFunc<int()>("hello");
    auto Sum = dllClass.ExcecuteFunc<int(int,int)>("Sum",2,3);
    std::cout<<Sum<<std::endl;
    char pChar[10]={0};
    dllClass.ExcecuteFunc<void(char*)>("GetString",pChar);
    std::cout<<pChar<<std::endl;
    return 0;
}
