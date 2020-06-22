/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Private header file for the TWS topology.
*/

#ifndef TWS_TOPOLOGY_PRIVATE_H_
#define TWS_TOPOLOGY_PRIVATE_H_

#include "tws_topology_sm.h"
#include "hdma.h"
#include <rules_engine.h>
#include <goals_engine.h>
#include <task_list.h>
#include <stdlib.h>

#include <message.h>
#include <bdaddr.h>

/*! \name Topology message identifier groups 

    These defines provide the base message identifiers for use within the 
    topology layer.
*/
/*! @{ */
    /*! The start identifier for general messages used by the topology */
#define TWSTOP_INTERNAL_MSG_BASE                    0x0000

    /*! The start identifier for messages related to primary rules */
#define TWSTOP_INTERNAL_PRIMARY_RULE_MSG_BASE       0x0100

    /*! The start identifier for messages related to secondary rules */
#define TWSTOP_INTERNAL_SECONDARY_RULE_MSG_BASE     0x0200

    /*! The start identifier for messages related to DFU rules */
#define TWSTOP_INTERNAL_DFU_RULE_MSG_BASE           0x0300

    /*! The start identifier for messages related to procedures */
#define TWSTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE  0x0600

    /*! The start identifier for messages related to the topology script engine */
#define TWSTOP_INTERNAL_PROC_SCRIPT_ENGINE_MSG_BASE 0x0700

    /*! The start identifier for messages related to topology use of peer signalling. */
#define TWSTOP_INTERNAL_PEER_SIG_MSG_BASE           0x0800
/*! @} */


/*! Defines the roles changed task list initalc capacity */
#define MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY 1

typedef enum
{
    TWSTOP_INTERNAL_START = TWSTOP_INTERNAL_MSG_BASE,
    TWSTOP_INTERNAL_HANDLE_PENDING_GOAL,
    TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED,
    TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION,
    TWSTOP_INTERNAL_PEER_SIG_MSG,
    TWSTOP_INTERNAL_CLEAR_HANDOVER_PLAY,
    TWSTOP_INTERNAL_MSG_MAX,
} tws_topology_internal_message_t;


/*! Structure describing handover data */
typedef struct {
    /*! Cached handover message notification from HDMA  */
    hdma_handover_decision_t hdma_message;
}handover_data_t;

/*! Structure holding information for the TWS Topology task */
typedef struct
{
    /*! Task for handling messages */
    TaskData                task;

    /*! Task for handling goal messages (from the rules engine) */
    TaskData                goal_task;

    /*! Task to be sent all outgoing messages */
    Task                    app_task;

    /*! Current primary/secondary role */
    tws_topology_role       role;

    /*! Whether we are acting in a role until a firm role is determined. */
    bool                    acting_in_role;

    /*! Internal state */
    tws_topology_state      state;

    /*! Whether we have sent a start confirm yet */
    bool                    start_cfm_needed;

    /*! List of clients registered to receive TWS_TOPOLOGY_ROLE_CHANGED_IND_T
     * messages */
    TASK_LIST_WITH_INITIAL_CAPACITY(MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY)   message_client_tasks;

    /*! Task handler for pairing activity notification */
    TaskData                pairing_notification_task;

    /*! Queue of goals already decided but waiting to be run. */
    TaskData                pending_goal_queue_task;

    /*! The TWS topology goal set */
    goal_set_t              goal_set;

    /*! Whether hdma is created or not.TRUE if created. FALSE otherwise */
    bool                    hdma_created;
    
    /*! Whether Handover is allowed or prohibited. controlled by APP */
    bool                    app_prohibit_handover;

    /*! handover related information */
    handover_data_t         handover_info;

    /*! List of internal TWS Topology clients using peer signalling
        with peer TWS Topology component. The core tws topology task 
        receives the messages directly - the list contains procedures
        that may be interested. */
    task_list_capacity_2_t  peer_sig_msg_client_list;

    /*! Can be used to control whether topology attempts handset connection */
    bool                    prohibit_connect_to_handset;

    /*! Flag to track whether topology has already been started */
    bool                    started;
} twsTopologyTaskData;

/* Make the tws_topology instance visible throughout the component. */
extern twsTopologyTaskData tws_topology;

/*! Get pointer to the task data */
#define TwsTopologyGetTaskData()         (&tws_topology)

/*! Get pointer to the TWS Topology task */
#define TwsTopologyGetTask()             (&tws_topology.task)

/*! Get pointer to the TWS Topology task */
#define TwsTopologyGetGoalTask()         (&tws_topology.goal_task)

/*! Get pointer to the TWS Topology role changed tasks */
#define TwsTopologyGetMessageClientTasks() (task_list_flexible_t *)(&tws_topology.message_client_tasks)

/*! Macro to create a TWS topology message. */
#define MAKE_TWS_TOPOLOGY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))

void twsTopology_SetRole(tws_topology_role role);
tws_topology_role twsTopology_GetRole(void);
void twsTopology_SetActingInRole(bool acting);
void twsTopology_RulesSetEvent(rule_events_t event);
void twsTopology_RulesResetEvent(rule_events_t event);
void twsTopology_RulesMarkComplete(MessageId message);
void twsTopology_CreateHdma(void);
void twsTopology_DestroyHdma(void);

/*! Private API used for test functionality

    \return TRUE if topology has been started, FALSE otherwise
 */
bool twsTopology_IsRunning(void);

#endif /* TWS_TOPOLOGY_H_ */
