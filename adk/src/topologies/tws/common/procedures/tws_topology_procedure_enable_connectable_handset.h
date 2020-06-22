/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T
#define TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T

#include "tws_topology_procedures.h"

extern const procedure_fns_t proc_enable_connectable_handset_fns;

typedef struct
{
    bool enable;
} ENABLE_CONNECTABLE_HANDSET_PARAMS_T; 

/*! Parameter definition for connectable enable */
extern const ENABLE_CONNECTABLE_HANDSET_PARAMS_T proc_enable_connectable_handset_enable;
#define PROC_ENABLE_CONNECTABLE_HANDSET_DATA_ENABLE  ((Message)&proc_enable_connectable_handset_enable)

/*! Parameter definition for connectable disable */
extern const ENABLE_CONNECTABLE_HANDSET_PARAMS_T proc_connectable_handset_disable;
#define PROC_ENABLE_CONNECTABLE_HANDSET_DATA_DISABLE  ((Message)&proc_connectable_handset_disable)

#endif /* TWS_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T */
