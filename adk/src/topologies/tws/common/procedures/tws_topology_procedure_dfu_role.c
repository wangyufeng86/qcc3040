/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure for moving to the DFU role.

            This is a simple procedure with the aim of leaving all 
            behaviours active. In time, some behaviours may be identified 
            that require stopping.
*/

#include "tws_topology_procedure_dfu_role.h"

#include "tws_topology_procedure_set_role.h"


#define DFU_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_DFU)

/* Define the dfu_role_script */
DEFINE_TOPOLOGY_SCRIPT(dfu_role, DFU_ROLE_SCRIPT);

