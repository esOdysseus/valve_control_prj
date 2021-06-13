#ifndef _INTERFACE_MONITOR_H_
#define _INTERFACE_MONITOR_H_

#include <map>
#include <memory>
#if __cplusplus > 201402L
    #include <shared_mutex> // for c++17
#else
    #include <shared_mutex_kes.h>   // for c++11
#endif


namespace monitor_pkg {


template <typename MONITORING_DATA>
class IMonitor {
public:
    using IDType = typename MONITORING_DATA::IDType;
    using FlagType = typename MONITORING_DATA::FlagType;
    using ErrType = typename MONITORING_DATA::ErrType;
    using E_MNT_FLAG = typename MONITORING_DATA::E_MNT_FLAG;
    using E_MNT_ERROR = typename MONITORING_DATA::E_MNT_ERROR;
    using DataType = typename MONITORING_DATA::DataType;

protected:
    using MapType = std::map<IDType, std::shared_ptr<MONITORING_DATA>>;

public:
    IMonitor(void);

    ~IMonitor(void);

    ssize_t size(void);

    bool is_there(IDType id);

    bool insert(IDType id, std::shared_ptr<DataType> &data);

    bool update(IDType id, E_MNT_FLAG pos, FlagType value);

    bool update(IDType id, E_MNT_ERROR pos, ErrType value);

    bool remove(IDType id);

    FlagType get(IDType id, E_MNT_FLAG pos);

    ErrType get(IDType id, E_MNT_ERROR pos);

    struct timespec get_time(IDType id);

    double get_elapsed_time(IDType id);

private:
    void clear(void);

protected:
    std::map<IDType, std::shared_ptr<MONITORING_DATA>> _data_;

    std::shared_mutex _mtx_data_;

};


}


#endif // _INTERFACE_MONITOR_H_