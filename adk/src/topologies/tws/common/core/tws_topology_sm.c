/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Implementation of state machine transitions for the TWS topology.
*/

#include <logging.h>
#include <panic.h>

#include "tws_topology_private.h"

#include "tws_topology_sm.h"
#include "tws_topology_sdp.h"

void TwsTopology_SetState(tws_topology_state new_state)
{
    twsTopologyTaskData *tws_t_task_data = TwsTopologyGetTaskData();

    tws_topology_state old_state = TwsTopology_GetState();

    if (old_state == new_state)
    {
        DEBUG_LOG_ERROR("TwsTopology_SetState Unexpected attempt to transition to same state:%d",
                   old_state);
//        Panic();    /*! \todo Remove panic once topology implementation stable */
        return;
    }

    DEBUG_LOG_STATE("TwsTopology_SetState Transition %d->%d",
                old_state, new_state);

    /* Pattern is to run functions for exiting state first */
    switch (old_state)
    {
        case TWS_TOPOLOGY_STATE_UNINITIALISED:
            break;
        case TWS_TOPOLOGY_STATE_SETTING_SDP:
            break;
        case TWS_TOPOLOGY_STATE_NO_ROLE:
            break;
        case TWS_TOPOLOGY_STATE_NO_PEER:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_NO_CONNECTION:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_HANDSET_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_PEER_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_PEER_HANDSET_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_NO_CONNECTION:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_PEER_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_PEER_HANDSET_CONNECTED:
            break;
    }

    tws_t_task_data->state = new_state;

    switch (new_state)
    {
        case TWS_TOPOLOGY_STATE_SETTING_SDP:
            DEBUG_LOG("tws_topology_set_state. Entered TWS_TOPOLOGY_SETTING_SDP");
            TwsTopology_RegisterServiceRecord(TwsTopologyGetTask());
            break;

        case TWS_TOPOLOGY_STATE_UNINITIALISED:
            break;
        case TWS_TOPOLOGY_STATE_NO_ROLE:
            break;
        case TWS_TOPOLOGY_STATE_NO_PEER:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_NO_CONNECTION:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_HANDSET_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_PEER_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_PRIMARY_PEER_HANDSET_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_NO_CONNECTION:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_PEER_CONNECTED:
            break;
        case TWS_TOPOLOGY_STATE_SECONDARY_PEER_HANDSET_CONNECTED:
            break;
    }
}

