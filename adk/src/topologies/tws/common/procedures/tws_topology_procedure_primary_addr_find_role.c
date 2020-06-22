/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to start find role following completion of peer pairing when out of the case.
*/

#include "tws_topology_procedures.h"
#include "script_engine.h"

#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_find_role.h"

/* Define components of primary_address_find_role_script */
#define PRIMARY_ADDRESS_FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_DISABLE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_DISABLE), \
    ENTRY(proc_set_address_fns, PROC_SET_ADDRESS_TYPE_DATA_PRIMARY), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_find_role_fns, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT)

/* Create the primary_address_find_role_script */
DEFINE_TOPOLOGY_SCRIPT(primary_address_find_role, PRIMARY_ADDRESS_FIND_ROLE_SCRIPT);
