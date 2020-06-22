/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to start finding a role when already in a Primary (acting) role
*/

#include "script_engine.h"

#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_find_role.h"

#include <logging.h>

#define PRIMARY_FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_ACTING_PRIMARY), \
    ENTRY(proc_find_role_fns, PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS)

/* Define the primary_find_role_script */
DEFINE_TOPOLOGY_SCRIPT(primary_find_role, PRIMARY_FIND_ROLE_SCRIPT);
