/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
\brief      Interface to procedure to enable/disable BREDR connectable (pagescan) for peer connectivity.
*/

#ifndef TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_PEER_H
#define TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_PEER_H

#include "tws_topology_procedures.h"

extern const procedure_fns_t proc_enable_connectable_peer_fns;

typedef struct
{
    bool enable;
    bool auto_disable;
} ENABLE_CONNECTABLE_PEER_PARAMS_T; 

/*! Parameter definition for connectable enable */
extern const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_enable;
#define PROC_ENABLE_CONNECTABLE_PEER_DATA_ENABLE  ((Message)&proc_enable_connectable_peer_enable)

/*! Parameter definition for connectable enable */
extern const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_enable_no_auto_disable;
#define PROC_ENABLE_CONNECTABLE_PEER_DATA_ENABLE_NO_AUTO_DISABLE  ((Message)&proc_enable_connectable_peer_enable_no_auto_disable)

/*! Parameter definition for connectable disable */
extern const ENABLE_CONNECTABLE_PEER_PARAMS_T proc_enable_connectable_peer_disable;
#define PROC_ENABLE_CONNECTABLE_PEER_DATA_DISABLE   ((Message)&proc_enable_connectable_peer_disable)

#endif /* TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_PEER_H */
