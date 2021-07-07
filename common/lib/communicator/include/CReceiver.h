/***
 * CReceiver.h
 * Copyright [2019-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 * 
 * This file is part of the Common-Communicator framework.
 */
#ifndef _C_RECEIVER_H_
#define _C_RECEIVER_H_

#include <string>
#include <functional>

#include <Enum_common.h>
#include <CPayload.h>

class CReceiver {
public:
    using InitialCB_Type = std::function<void(enum_c::ProviderType provider_type, bool flag_init)>;
    using ConnectionCB_Type = std::function<void(std::string /*peer_app_path*/, std::string /*peer_pvd_path*/, bool flag_connect)>;
    using MessagePayloadCB_Type = std::function<void(std::string /*peer_app_path*/, std::string /*peer_pvd_path*/, std::shared_ptr<payload::CPayload> /*payload*/)>;
    using QuitCB_Type = std::function<void(const std::exception &e)>;

public:
    InitialCB_Type cb_initialization_handle;

    ConnectionCB_Type cb_connection_handle;

    MessagePayloadCB_Type cb_message_payload_handle;

    QuitCB_Type cb_quit_handle;

public:
    CReceiver(void): cb_initialization_handle(NULL), 
                     cb_connection_handle(NULL),
                     cb_message_payload_handle(NULL),
                     cb_quit_handle(NULL) {};

    ~CReceiver(void) {
        cb_initialization_handle = NULL;
        cb_connection_handle = NULL;
        cb_message_payload_handle = NULL;
        cb_quit_handle = NULL;
    }
};

#endif // _C_RECEIVER_H_