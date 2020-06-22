/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      
*/

#include "script_engine.h"

#include "tws_topology_procedure_allow_handset_connect.h"
#include "tws_topology_procedure_disconnect_handset.h"
#include "tws_topology_procedure_disconnect_peer_profiles.h"
#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_allow_connection_over_bredr.h"
#include "tws_topology_procedure_sec_connect_peer.h"
#include "tws_topology_procedure_clean_connections.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"
#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_procedure_enable_audio_indications.h"

#include <logging.h>

/*! procedures and data for the switch to secondary script

    \note Items are ordered logically, so we block connections before 
        we disconnect. This avoids the small chance that a fresh 
        connection can be made while we are disconnecting.
        Clean_connections is last in the clean-up.
*/
#define SWITCH_TO_SECONDARY_SCRIPT(ENTRY) \
    ENTRY(proc_enable_audio_indications_fns, PROC_ENABLE_AUDIO_INDICATIONS_DISABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_DISABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_DISABLE), \
    ENTRY(proc_allow_handset_connect_fns, PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_DISABLE), \
    ENTRY(proc_enable_connectable_handset_fns, PROC_ENABLE_CONNECTABLE_HANDSET_DATA_DISABLE), \
    ENTRY(proc_enable_le_connectable_handset_fns,PROC_ENABLE_LE_CONNECTABLE_PARAMS_DISABLE), \
    ENTRY(proc_disconnect_handset_fns, NO_DATA), \
    ENTRY(proc_disconnect_peer_profiles_fns, PROC_DISCONNECT_PEER_PROFILES_DATA_ALL), \
    ENTRY(proc_clean_connections_fns, NO_DATA), \
    ENTRY(proc_set_address_fns, PROC_SET_ADDRESS_TYPE_DATA_SECONDARY), \
    ENTRY(proc_set_role_fns, PROC_SET_ROLE_TYPE_DATA_SECONDARY), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_allow_connection_over_bredr_fns, PROC_ALLOW_CONNECTION_OVER_BREDR_ENABLE), \
    ENTRY(proc_sec_connect_peer_fns, NO_DATA), \
    ENTRY(proc_enable_audio_indications_fns, PROC_ENABLE_AUDIO_INDICATIONS_ENABLE)

/*! Define the switch_to_secondary_script */
DEFINE_TOPOLOGY_SCRIPT(switch_to_secondary, SWITCH_TO_SECONDARY_SCRIPT);

