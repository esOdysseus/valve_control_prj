#ifndef _C_SERVICE_MONITOR_H_
#define _C_SERVICE_MONITOR_H_

#include <functional>

#include <IMonitor.h>
#include <CSvcMD.h>


namespace monitor_pkg {


class CSvcMonitor : public IMonitor<CSvcMD> {
public:
    using FuncType = std::function<bool(IDType id, double elapsed_time)>;

public:
    CSvcMonitor(void);

    ~CSvcMonitor(void) = default;
    
    void process_with_elapse_time(FuncType lamda);

};


}


#endif // _C_SERVICE_MONITOR_H_