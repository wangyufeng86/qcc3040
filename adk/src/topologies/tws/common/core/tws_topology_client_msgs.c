/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      TWS Topology utility functions for sending messages to clients.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_client_msgs.h"

#include <logging.h>

#include <panic.h>

void TwsTopology_SendRoleChangedInd(tws_topology_role role)
{
    MAKE_TWS_TOPOLOGY_MESSAGE(TWS_TOPOLOGY_ROLE_CHANGED_IND);

    message->role = role;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_ROLE_CHANGED_IND, message);
}

void TwsTopology_SendStartCfm(tws_topology_status_t sts, tws_topology_role role)
{
    twsTopologyTaskData *twst = TwsTopologyGetTaskData();

    DEBUG_LOG("twsTopology_SendStartCfm sts %u role %u start_cfm_needed %u", sts, role, twst->start_cfm_needed);

    if (twst->start_cfm_needed)
    {
        MAKE_TWS_TOPOLOGY_MESSAGE(TWS_TOPOLOGY_START_CFM);
        DEBUG_LOG("twsTopology_SendStartCfm %u", role);
        twst->start_cfm_needed = FALSE;
        message->status = sts;
        message->role = role;
        MessageSend(twst->app_task, TWS_TOPOLOGY_START_CFM, message);
    }
}

void TwsTopology_SendHandsetDisconnectedIndication(void)
{
    DEBUG_LOG("TwsTopology_SendHandsetDisconnected");
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND);
}
