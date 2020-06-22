/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Private header file for the Headset topology.
*/


#ifndef HEADSET_TOPOLOGY_PRIVATE_H_
#define HEADSET_TOPOLOGY_PRIVATE_H_

#include <task_list.h>
#include <message.h>
#include <rules_engine.h>
#include <goals_engine.h>

/*! Defines the task list initial capacity */
#define MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY 1


/*! @{ */
    /*! The start identifier for general messages used by the topology */
#define HEADSETTOP_INTERNAL_MSG_BASE                    (0x0000)

#define HEADSETTOP_INTERNAL_RULE_MSG_BASE               (0x0100)
/*! @} */

/*! Structure holding information for the Headset Topology task */
typedef struct
{
    /*! Task for handling messages */
    TaskData                task;

    /*! The HEADSET topology goal set */
    goal_set_t              goal_set;

    /*! Queue of goals already decided but waiting to be run. */
    TaskData                pending_goal_queue_task;

    /*! List of clients registered to receive HEADSET_TOPOLOGY_ROLE_CHANGED_IND_T
     * messages */
    TASK_LIST_WITH_INITIAL_CAPACITY(MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY)   message_client_tasks;

    /*! Can be used to control whether topology attempts handset connection */
    bool                prohibit_connect_to_handset;

    /*! Used to indicate decide connectability of headset */
    bool                shutdown_in_progress;

    /*! TODO Variable to indicate the topology start status. This is also to be updated when System state comes in(VMCSA-3767) */
    bool                start_status;
} headsetTopologyTaskData;


/* Make the headset_topology instance visible throughout the component. */
extern headsetTopologyTaskData headset_topology;

/*! Get pointer to the task data */
#define HeadsetTopologyGetTaskData()         (&headset_topology)

/*! Get pointer to the Headset Topology task */
#define HeadsetTopologyGetTask()             (&headset_topology.task)

/*! Get pointer to the Headset Topology client tasks */
#define HeadsetTopologyGetMessageClientTasks() (task_list_flexible_t *)(&headset_topology.message_client_tasks)

/*! Macro to create a Headset topology message. */
#define MAKE_HEADSET_TOPOLOGY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))

#endif /* HEADSET_TOPOLOGY_PRIVATE_H_ */

