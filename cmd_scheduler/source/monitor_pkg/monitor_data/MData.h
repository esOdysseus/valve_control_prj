#ifndef _MONITORING_DATA_H_
#define _MONITORING_DATA_H_

#include <string>
#include <IProc.h>

namespace monitor_pkg {


class Cmsg_packet {
public:
    proc_pkg::IProc *proc_pointer; // instance of process that make this packet.
    std::string asked_from;        // who ask to make this packet.
    std::string request_to;        // who is received this packet.
    uint32_t msg_id;               // MSG-ID of encoded context.
    const void *data;              // encoded context.
    ssize_t data_size;             // encoded context size.

public:
    Cmsg_packet(void) {
        data = NULL;
        data_size = 0;
        clear();
    }

    ~Cmsg_packet(void) {
        clear();
    }

private:
    void clear(void) {
        proc_pointer = NULL;
        asked_from.clear();
        request_to.clear();
        msg_id = 0;
        if (data) {
            delete[] data;
            data = NULL;
        }
        data_size = 0;
    }
};


}


#endif // _MONITORING_DATA_H_