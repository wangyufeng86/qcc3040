/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Header file for state machine transitions in the TWS topology.
*/

#ifndef TWS_TOPOLOGY_SM_H_
#define TWS_TOPOLOGY_SM_H_

#include "tws_topology.h"

/*! Definition of TWS Topology states. */
typedef enum
{
    TWS_TOPOLOGY_STATE_UNINITIALISED,
    TWS_TOPOLOGY_STATE_SETTING_SDP,
    TWS_TOPOLOGY_STATE_NO_ROLE,
    TWS_TOPOLOGY_STATE_NO_PEER,
    TWS_TOPOLOGY_STATE_PRIMARY_NO_CONNECTION,
    TWS_TOPOLOGY_STATE_PRIMARY_HANDSET_CONNECTED,
    TWS_TOPOLOGY_STATE_PRIMARY_PEER_CONNECTED,
    TWS_TOPOLOGY_STATE_PRIMARY_PEER_HANDSET_CONNECTED,
    TWS_TOPOLOGY_STATE_SECONDARY_NO_CONNECTION,
    TWS_TOPOLOGY_STATE_SECONDARY_PEER_CONNECTED,
    TWS_TOPOLOGY_STATE_SECONDARY_PEER_HANDSET_CONNECTED,
} tws_topology_state;

/*! \brief Change the state of the TWS Topology.
    \param[in] state New state.
*/
void TwsTopology_SetState(tws_topology_state state);

/*! \brief Accessor to get current TWS Topology state. */
#define TwsTopology_GetState() (TwsTopologyGetTaskData()->state)

#endif /* TWS_TOPOLOGY_SM_H_ */
