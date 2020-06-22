/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to procedure to disconnect peer BREDR profiles. 
*/

#ifndef TWS_TOPOLOGY_PROC_DISCONNECT_PEER_PROFILES_H
#define TWS_TOPOLOGY_PROC_DISCONNECT_PEER_PROFILES_H

#include "tws_topology_procedures.h"

extern const procedure_fns_t proc_disconnect_peer_profiles_fns;

typedef struct
{
    uint8 profiles;
} DISCONNECT_PEER_PROFILES_T;

extern const DISCONNECT_PEER_PROFILES_T proc_disconnect_peer_profiles_all;
#define PROC_DISCONNECT_PEER_PROFILES_DATA_ALL ((Message)&proc_disconnect_peer_profiles_all)

#endif /* TWS_TOPOLOGY_PROC_DISCONNECT_PEER_PROFILES_H */
