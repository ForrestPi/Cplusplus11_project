#ifndef TESTBUS_H
#define TESTBUS_H
#include <iostream>
#include <string>
using namespace std;
#include "MessageBus.h"

MessageBus g_bus;
const string Topic="Drive";

struct Subject{
    void SendReq(const string topic){
        g_bus.SendReq<void,int>(50,topic);
    }
};

struct Car{
    Car(){
        g_bus.Attach([this](int speed){Drive(speed);},Topic);
    }
    void Drive(int speed){
        cout<<"Car drive"<<speed<<endl;
    }
};

struct Bus{
    Bus(){
        g_bus.Attach([this](int speed){Drive(speed);},Topic);
    }

    void Drive(int speed){
        cout<<"Bus drive "<<speed<<endl;
    }
};

struct Truck{
    Truck(){
        g_bus.Attach([this](int speed){Drive(speed);});
    }
    void Drive(int speed){
        cout<<"Truck drive"<<endl;
    }
};


void TestBus(){
    Subject Subject;
    Car car;
    Bus bus;
    Truck Truck;
    Subject.SendReq(Topic);
    cout<<"==========="<<endl;
    Subject.SendReq("");
    cout<<"==========="<<endl;

    g_bus.Remove<void,int>();
    Subject.SendReq("");

}

#endif // TESTBUS_H
