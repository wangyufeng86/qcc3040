/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_SET_ROLE_H
#define TWS_TOPOLOGY_PROC_SET_ROLE_H

#include "tws_topology.h"
#include "tws_topology_procedures.h"

extern const procedure_fns_t proc_set_role_fns;

typedef struct
{
    tws_topology_role role;
    bool acting_in_role;
} SET_ROLE_TYPE_T;

extern const SET_ROLE_TYPE_T proc_set_role_none;
#define PROC_SET_ROLE_TYPE_DATA_NONE        ((Message)&proc_set_role_none)

extern const SET_ROLE_TYPE_T proc_set_role_primary_role;
#define PROC_SET_ROLE_TYPE_DATA_PRIMARY     ((Message)&proc_set_role_primary_role)

extern const SET_ROLE_TYPE_T proc_set_role_acting_primary_role;
#define PROC_SET_ROLE_TYPE_DATA_ACTING_PRIMARY     ((Message)&proc_set_role_acting_primary_role)

extern const SET_ROLE_TYPE_T proc_set_role_secondary_role;
#define PROC_SET_ROLE_TYPE_DATA_SECONDARY   ((Message)&proc_set_role_secondary_role)

extern const SET_ROLE_TYPE_T proc_set_role_dfu_role;
#define PROC_SET_ROLE_TYPE_DATA_DFU         ((Message)&proc_set_role_dfu_role)

#endif /* TWS_TOPOLOGY_PROC_SET_ROLE_H */
