/***
 * Enum_common.h
 * Copyright [2019-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 * 
 * This file is part of the Common-Communicator framework.
 */
#ifndef ENUM_COMMON_H_
#define ENUM_COMMON_H_

namespace enum_c
{
    typedef enum AliasType {
        E_ALIAST_NOT_DEFINE = 0,
        E_ALIAST_APP = 1,
        E_ALIAST_PROVIDER = 2
    } AliasType;

    typedef enum AppType {
        E_APPT_NOT_DEFINE = 0,
        E_APPT_SINGLE = 1,
        E_APPT_MULTIPLE = 2
    } AppType;

    typedef enum ProviderType {
        E_PVDT_NOT_DEFINE = 0,
        E_PVDT_TRANS_TCP = 1,
        E_PVDT_TRANS_UDP = 2,
        E_PVDT_TRANS_UDS_TCP = 3,
        E_PVDT_TRANS_UDS_UDP = 4,
        E_PVDT_SERVICE_VSOMEIP = 5,
        E_PVDT_RSC_IOTIVITY = 6
    } ProviderType;

    enum class ProviderMode {
        E_PVDM_NONE = 0,
        E_PVDM_SERVER = 1,
        E_PVDM_CLIENT = 2,
        E_PVDM_BOTH = 3
    };
}

namespace payload
{
    typedef enum E_PAYLOAD_FLAG {   // We will use it as Bit-Masking type flag.
        E_NONE = 0,
        E_KEEP_PAYLOAD_AFTER_TX = 1
    } E_PAYLOAD_FLAG;
}


#endif // ENUM_COMMON_H_