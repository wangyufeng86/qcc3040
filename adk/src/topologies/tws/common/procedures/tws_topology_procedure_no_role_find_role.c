/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to start finding a role when not in the Primary role.
*/

#include "script_engine.h"

#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_allow_connection_over_bredr.h"
#include "tws_topology_procedure_allow_handset_connect.h"
#include "tws_topology_procedure_find_role.h"
#include "tws_topology_procedure_clean_connections.h"
#include "tws_topology_procedure_cancel_find_role.h"

#include <logging.h>

/* The order of items in the \b script should follow the recommendations
    documented against \ref tws_topology_procedure.
*/

#define NO_ROLE_FIND_ROLE_SCRIPT(ENTRY) \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_DISABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_DISABLE), \
    ENTRY(proc_allow_handset_connect_fns, PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_DISABLE), \
    ENTRY(proc_cancel_find_role_fns, NO_DATA), \
    ENTRY(proc_clean_connections_fns, NO_DATA), \
    ENTRY(proc_set_address_fns ,PROC_SET_ADDRESS_TYPE_DATA_PRIMARY), \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_NONE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_ENABLE), \
    ENTRY(proc_find_role_fns, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT)


/* Define the no_role_find_role_script */
DEFINE_TOPOLOGY_SCRIPT(no_role_find_role, NO_ROLE_FIND_ROLE_SCRIPT);

