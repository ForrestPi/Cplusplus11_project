#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H

#include <string>
#include <functional>
#include <map>
#include "Any.h"
#include "function_traits.h"
#include "NonCopyable.h"

using namespace std;

class MessageBus: NonCopyable
{
public:
    //注册可调用对象
    template<typename F>
    void Attach(F&& f, const string& strTopic="")
    {
        auto func = to_function(std::forward<F>(f));
        Add(strTopic, std::move(func));
    }

    //注册成员函数
    template<class C,class... Args,class P>
    void Attach(const string &strTopic,void(C::*f)(Args...) const,const P& p){
        std::function<void(Args...)> func=[&p,f](Args... args){return (*p.*f)(std::forward<Args>(args)...);};
        Add(strTopic,std::move(func));
    }

    //发送消息
    template<typename R>
    void SendReq(const string& strTopic = "")
    {
        using function_type = std::function<R()>;
        string strMsgType =strTopic+ typeid(function_type).name();
        auto range = m_map.equal_range(strMsgType);
        for (Iterater it = range.first; it != range.second; ++it)
        {
            auto f = it->second.AnyCast < function_type >();
            f();
        }
    }
    template<typename R, typename... Args>
    void SendReq(Args&&... args, const string& strTopic = "")
    {
        using function_type = std::function<R(Args...)>;
        string strMsgType =strTopic+ typeid(function_type).name();
        auto range = m_map.equal_range(strMsgType);
        for (Iterater it = range.first; it != range.second; ++it)
        {
            auto f = it->second.AnyCast < function_type >();
            f(std::forward<Args>(args)...);
        }
    }

    //移除某个主题, 需要主题和消息类型
    template<typename R, typename... Args>
    void Remove(const string& strTopic = "")
    {
        using function_type = std::function<R(Args...)>; //typename function_traits<void(CArgs)>::stl_function_type;

        string strMsgType =strTopic +typeid(function_type).name();
        int count = m_map.count(strMsgType);
        auto range = m_map.equal_range(strMsgType);
        m_map.erase(range.first, range.second);
    }

private:
    template<typename F>
    void Add(const string& strTopic, F&& f)
    {
        string strMsgType = strTopic + typeid(F).name();
        m_map.emplace(std::move(strMsgType), std::forward<F>(f));
    }

private:
    std::multimap<string, Any> m_map;
    typedef std::multimap<string, Any>::iterator Iterater;
};

#endif // MESSAGEBUS_H
