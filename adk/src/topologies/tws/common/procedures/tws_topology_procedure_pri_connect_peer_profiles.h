/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H
#define TWS_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H

#include "tws_topology_procedures.h"

typedef struct
{
    uint8 profiles;
} TWSTOP_PRIMARY_GOAL_CONNECT_PEER_PROFILES_T;

extern const procedure_fns_t proc_pri_connect_peer_profiles_fns;

#endif /* TWS_TOPOLOGY_PROC_PRI_CONNECT_PEER_PROFILES_H */

