/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Procedure for a Primary to participate in static handover and become Secondary.
*/

#include "script_engine.h"

#include "tws_topology_procedure_dfu_abort_on_handover.h"
#include "tws_topology_procedure_allow_handset_connect.h"
#include "tws_topology_procedure_notify_role_change_clients.h"
#include "tws_topology_procedure_command_role_switch.h"
#include "tws_topology_procedure_clean_connections.h"
#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_procedure_enable_connectable_peer.h"
#include "tws_topology_procedure_cancel_find_role.h"
#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_allow_connection_over_bredr.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"
#include "tws_topology_procedure_enable_audio_indications.h"

/*
 * Note: proc_abort_dfu_fns need to preceed handover procedure and GAIA
 *       disconnection with Host/Handset. Hence placed as the 1st procedure
 *       in an handover script and needs to be retained.
 */
#define PRIMARY_STATIC_HANDOVER_SCRIPT(ENTRY) \
    ENTRY(proc_abort_dfu_fns, NO_DATA), \
    ENTRY(proc_enable_audio_indications_fns, PROC_ENABLE_AUDIO_INDICATIONS_DISABLE), \
    ENTRY(proc_notify_role_change_clients_fns, PROC_NOTIFY_ROLE_CHANGE_CLIENTS_FORCE_NOTIFICATION), \
    ENTRY(proc_command_role_switch_fns, NO_DATA), \
    ENTRY(proc_allow_handset_connect_fns, PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_DISABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_DISABLE), \
    ENTRY(proc_enable_connectable_handset_fns, PROC_ENABLE_CONNECTABLE_HANDSET_DATA_DISABLE), \
    ENTRY(proc_enable_connectable_peer_fns, PROC_ENABLE_CONNECTABLE_PEER_DATA_DISABLE), \
    ENTRY(proc_enable_le_connectable_handset_fns, PROC_ENABLE_LE_CONNECTABLE_PARAMS_DISABLE), \
    ENTRY(proc_cancel_find_role_fns, NO_DATA), \
    ENTRY(proc_clean_connections_fns, NO_DATA), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_DISABLE), \
    ENTRY(proc_set_address_fns, PROC_SET_ADDRESS_TYPE_DATA_SECONDARY), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_ENABLE), \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_SECONDARY), \
    ENTRY(proc_enable_audio_indications_fns, PROC_ENABLE_AUDIO_INDICATIONS_ENABLE)

/* Define the primary_static_handover_script */
DEFINE_TOPOLOGY_SCRIPT(primary_static_handover, PRIMARY_STATIC_HANDOVER_SCRIPT);
