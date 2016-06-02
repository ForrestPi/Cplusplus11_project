#ifndef DLLPARSER_H
#define DLLPARSER_H

#include<Windows.h>
#include<string>
#include<map>
#include<functional>
using namespace std;

class DllParser
{
private:
    HMODULE m_hMod;
    std::map<string,FARPROC>m_map;
public:
    DllParser():m_hMod(nullptr){}
    ~DllParser()
    {
        Unload();
    }
    bool Load(const string& dllPath){
        m_hMod=LoadLibraryA(dllPath.data());
        if (nullptr==m_hMod)
        {
            printf("LoadLibrary failed\n");
            return false;
        }
        return true;
    }

    bool Unload()
    {
        if (m_hMod==nullptr)
            return true;
        auto b= FreeLibrary(m_hMod);
        if (!b)
            return false;
        m_hMod=nullptr;
        return true;
    }

    template<typename T>
    std::function<T> GetFunction(const string& funcName)
    {
        auto it= m_map.find(funcName);
        if (it==m_map.end())
        {
            auto addr = GetProcAddress(m_hMod,funcName.c_str());
            if (!addr)
                return nullptr;
            m_map.insert(std::make_pair(funcName,addr));
            it=m_map.find(funcName);
        }
        return std::function<T>((T*)(it->second));
    }

    template <typename T,typename... Args>
    typename std::result_of<std::function<T>(Args...)>::type ExcecuteFunc(const string& funcName,Args&&... args)
    {
        auto f = GetFunction<T>(funcName);
        if (f==nullptr)
        {
            string s = "Can not find function "+funcName;
        }
        return f(std::forward<Args>(args)...);
    }
};

#endif // DLLPARSER_H
