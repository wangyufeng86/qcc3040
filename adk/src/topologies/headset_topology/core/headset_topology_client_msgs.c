/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Headset Topology utility functions for sending messages to clients.
*/

#include "headset_topology.h"
#include "headset_topology_private.h"
#include "headset_topology_client_msgs.h"

#include <logging.h>
#include<task_list.h>

#include <panic.h>

void HeadsetTopology_SendHandsetDisconnectedIndication(void)
{
    DEBUG_LOG("HeadsetTopology_SendHandsetDisconnectedIndication");
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(HeadsetTopologyGetMessageClientTasks()), HEADSET_TOPOLOGY_HANDSET_DISCONNECTED_IND);
}
