/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to disconnect peer and start find role.
*/

#include "tws_topology_procedures.h"
#include "tws_topology_procedure_disconnect_peer_profiles.h"
#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_find_role.h"

/* Define components of script disconnect_peer_find_role_script */
#define DISCONNECT_PEER_FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_disconnect_peer_profiles_fns, PROC_DISCONNECT_PEER_PROFILES_DATA_ALL), \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_NONE), \
    ENTRY(proc_find_role_fns, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT),

/* Create the script disconnect_peer_find_role_script */
DEFINE_TOPOLOGY_SCRIPT(disconnect_peer_find_role, DISCONNECT_PEER_FIND_ROLE_SCRIPT);

