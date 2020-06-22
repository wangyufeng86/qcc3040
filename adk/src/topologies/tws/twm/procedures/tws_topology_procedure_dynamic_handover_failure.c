/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_procedure_dynamic_handover_failure.h"
#include "script_engine.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"
#include "tws_topology_procedure_notify_role_change_clients.h"

#define DYNAMIC_HANDOVER_FAILURE_SCRIPT(ENTRY) \
    ENTRY(proc_enable_le_connectable_handset_fns, PROC_ENABLE_LE_CONNECTABLE_PARAMS_ENABLE),\
    ENTRY(proc_notify_role_change_clients_fns,PROC_NOTIFY_ROLE_CHANGE_CLIENTS_CANCEL_NOTIFICATION)


/* Define the dynamic handover script */
DEFINE_TOPOLOGY_SCRIPT(dynamic_handover_failure, DYNAMIC_HANDOVER_FAILURE_SCRIPT);

