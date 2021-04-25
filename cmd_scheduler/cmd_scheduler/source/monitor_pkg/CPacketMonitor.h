#ifndef _C_PACKET_MONITOR_H_
#define _C_PACKET_MONITOR_H_

#include <IMonitor.h>
#include <CPacketMD.h>

namespace monitor_pkg {


class CPacketMonitor : public IMonitor<CPacketMD> {
public:
    CPacketMonitor(void);

    ~CPacketMonitor(void);
    
};


}


#endif // _C_PACKET_MONITOR_H_