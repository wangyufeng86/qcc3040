/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Enable controlling the connection to handset.
*/

#ifndef HEADSET_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T
#define HEADSET_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T

#include "procedures.h"

extern const procedure_fns_t hs_proc_enable_connectable_handset_fns;

/*! Structure definining the parameters for the procedure to enable/disable
    connectability to handset.
*/
typedef struct
{
    bool enable;
} ENABLE_CONNECTABLE_HANDSET_PARAMS_T;

/*! Parameter definition for connectable enable */
extern const ENABLE_CONNECTABLE_HANDSET_PARAMS_T hs_topology_procedure_connectable_handset_enable;
#define PROC_ENABLE_CONNECTABLE_HANDSET_DATA_ENABLE  ((Message)&hs_topology_procedure_connectable_handset_enable)

/*! Parameter definition for connectable disable */
extern const ENABLE_CONNECTABLE_HANDSET_PARAMS_T hs_topology_procedure_connectable_handset_disable;
#define PROC_ENABLE_CONNECTABLE_HANDSET_DATA_DISABLE  ((Message)&hs_topology_procedure_connectable_handset_disable)

#endif /* HEADSET_TOPOLOGY_PROC_ENABLE_CONNECTABLE_HANDSET_T */
