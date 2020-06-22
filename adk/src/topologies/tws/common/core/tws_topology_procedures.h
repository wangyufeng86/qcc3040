/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of TWS topology specific procedures.
*/

#ifndef TWS_TOPOLOGY_PROCEDURES_H
#define TWS_TOPOLOGY_PROCEDURES_H

#include "procedures.h"


/*! Definition of TWS Topology procedures. 

    A naming convention is followed for important procedures. 
    Following the convention and using in the recommended order
    reduces the possibility of unexpected behaviour.

    The convention applies to functions that enable or disable 
    activity.
    \li allow changes the response if an event happens.
    \li permit starts or stops an activity temporarily
    \li enable Starts or stops an activity permanently

    If several of these functions are called with DISABLE parameters
    it is recommended that they are called in the order 
    allow - permit - enable.
*/
typedef enum
{
    /*! Procedure to pair with peer Earbud. */
    tws_topology_procedure_pair_peer = 1,

    /*! Procedure to determine Primary/Secondary role of Earbud. */
    tws_topology_procedure_find_role,

    /*! Procedure to connect BREDR ACL to Primary Earbud. */
    tws_topology_procedure_sec_connect_peer,

    /*! Procedure to connect BREDR profiles to Secondary Earbud. */
    tws_topology_procedure_pri_connect_peer_profiles,

    /*! Procedure to connect BREDR profiles to Earbud. */
    tws_topology_procedure_disconnect_peer_profiles,

    /*! Procedure to enable page scan for Secondary to establish BREDR ACL to Primary Earbud. */
    tws_topology_procedure_enable_connectable_peer,

    tws_topology_procedure_no_role_idle,
    tws_topology_procedure_set_role,

    tws_topology_procedure_connect_handset,

    tws_topology_procedure_disconnect_handset,

    tws_topology_procedure_enable_connectable_handset,

    tws_topology_procedure_permit_bt,
    tws_topology_procedure_set_address,

    tws_topology_procedure_become_primary,
    tws_topology_procedure_become_secondary,
    tws_topology_procedure_become_acting_primary,
    tws_topology_procedure_set_primary_address_and_find_role,

    tws_topology_procedure_role_switch_to_secondary,
    tws_topology_procedure_no_role_find_role,
    tws_topology_procedure_cancel_find_role,
    tws_topology_procedure_primary_find_role,

    tws_topology_procedure_allow_connection_over_le,
    tws_topology_procedure_allow_connection_over_bredr,

    tws_topology_procedure_pair_peer_script,

    tws_topology_procedure_dfu_role,
    tws_topology_procedure_dfu_primary,
    tws_topology_procedure_dfu_secondary,

    tws_topology_procedure_allow_handset_connection,

    tws_topology_procedure_disconnect_peer_find_role,

    tws_topology_procedure_clean_connections,

    /*! Procedure to release the lock on ACL to the peer and 
        (potentially) start closing the connection */
    tws_topology_procedure_release_peer,

    tws_topology_procedure_command_role_switch,
    tws_topology_procedure_wait_peer_link_drop,
    tws_topology_procedure_secondary_static_handover,
    tws_topology_procedure_primary_static_handover_in_case,

    tws_topology_procedure_primary_static_handover,
    tws_topology_procedure_dfu_in_case,
    /*! Procedure to abort DFU when handover kicks in. */
    tws_topology_procedure_dfu_abort_on_handover,

    /* Procedure for handling dynamic handover  */
    tws_topology_procedure_handover,
    tws_topology_procedure_dynamic_handover,
    tws_topology_procedure_dynamic_handover_failure,

    tws_topology_procedure_enable_le_connectable_handset,
    tws_topology_procedure_disconnect_le_connections,
    tws_topology_procedure_notify_role_change_clients,

    /* Procedure for gaiting UI Inidciations during handovers */
    tws_topology_procedure_enable_audio_indications

} tws_topology_procedure;


#endif /* TWS_TOPOLOGY_PROCEDURES_H */
