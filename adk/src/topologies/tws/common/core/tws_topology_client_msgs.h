/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWS Topology utility functions for sending messages to clients.
*/

#ifndef TWS_TOPOLOGY_CLIENT_MSGS_H
#define TWS_TOPOLOGY_CLIENT_MSGS_H

#include "tws_topology.h"

/*! \brief Send indication to registered clients of the new Earbud role.
    \param[in] role New Earbud role.
*/
void TwsTopology_SendRoleChangedInd(tws_topology_role role);

/*! \brief Send confirmation message to the task which called #TwsTopology_Start().
    \param[in] sts Status of the start operation.
    \param[in] role Current role of the Earbud, may be #tws_topology_none where
                    the topology has been unable to determine the role yet.

    \note It is expected that the task will be the application SM task.
*/
void TwsTopology_SendStartCfm(tws_topology_status_t sts, tws_topology_role role);

/*! \brief Send indication to registered clients that handset disconnected goal has been reached.
*/
void TwsTopology_SendHandsetDisconnectedIndication(void);

#endif /* TWS_TOPOLOGY_CLIENT_MSGS_H */
