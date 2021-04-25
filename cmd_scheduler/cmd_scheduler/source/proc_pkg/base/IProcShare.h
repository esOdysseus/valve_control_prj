#ifndef _INTERFACE_PROCESS_SHARE_H_
#define _INTERFACE_PROCESS_SHARE_H_

#include <map>
#include <string>
#if __cplusplus > 201402L
    #include <shared_mutex> // for c++17
#else
    #include <shared_mutex_kes.h>   // for c++11
#endif


namespace proc_pkg {

class IProc;

class IProcShare {
public:
    IProcShare(void);

    ~IProcShare(void);

    bool insert(IProc &instance);

    bool remove(IProc &instance);

    IProc* get(std::string &proc_name);

private:
    bool remove(std::string &proc_name);

private:
    std::map<std::string, IProc*> _proc_map_;

    std::shared_mutex _mtx_proc_map_;

};


}

#endif // _INTERFACE_PROCESS_SHARE_H_