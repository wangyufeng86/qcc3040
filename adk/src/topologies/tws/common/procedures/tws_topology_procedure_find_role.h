/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_FIND_ROLE_H
#define TWS_TOPOLOGY_PROC_FIND_ROLE_H

#include "tws_topology_procedures.h"
#include "script_engine.h"

extern const procedure_fns_t proc_find_role_fns;

typedef struct
{
    int32 timeout;
} FIND_ROLE_PARAMS_T;

extern const FIND_ROLE_PARAMS_T proc_find_role_timeout;
#define PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT     ((Message)&proc_find_role_timeout)

extern const FIND_ROLE_PARAMS_T proc_find_role_continuous;
#define PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS  ((Message)&proc_find_role_continuous)

#endif /* TWS_TOPOLOGY_PROC_FIND_ROLE_H */
