/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "script_engine.h"

#include "tws_topology_procedure_set_role.h"

const procedure_fns_t* primary_role_procs[] = {
    &proc_set_role_fns,
};

const Message primary_role_procs_data[] = {
    PROC_SET_ROLE_TYPE_DATA_PRIMARY,
};

const procedure_script_t primary_role_script = {
    primary_role_procs,
    primary_role_procs_data,
    ARRAY_DIM(primary_role_procs),
};

