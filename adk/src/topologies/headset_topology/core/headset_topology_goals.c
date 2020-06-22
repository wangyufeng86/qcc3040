/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      HEADSET Topology goal handling.
*/

#include "headset_topology.h"
#include "headset_topology_private.h"

#include "headset_topology_rules.h"

#include "headset_topology_goals.h"
#include "headset_topology_rule_events.h"
#include "headset_topology_procedures.h"

#include "headset_topology_procedure_allow_handset_connect.h"
#include "headset_topology_procedure_connect_handset.h"
#include "headset_topology_procedure_enable_connectable_handset.h"
#include "headset_topology_procedure_disconnect_handset.h"

#include <logging.h>

#include <message.h>
#include <panic.h>

#pragma unitsuppress Unused


/*! \brief This table defines each goal supported by the topology.

    Each entry links the goal set by a topology rule decision with the procedure required to achieve it.

    Entries also provide the configuration for how the goal should be handled, identifying the following
    characteristics:
     - is the goal exclusive with another, requiring the exclusive goal to be cancelled
     - the contention policy of the goal
        - can cancel other goals
        - can execute concurrently with other goals
        - must wait for other goal completion
     - function pointers to the procedure or script to achieve the goal
     - events to generate back into the role rules engine following goal completion
        - success, failure or timeout are supported
     
    Not all goals require configuration of all parameters so utility macros are used to define a
    goal and set default parameters for unrequired fields.
*/
const goal_entry_t hsgoals[] =
{
    GOAL_WITH_CONCURRENCY(hs_topology_goal_connect_handset, hs_topology_procedure_connect_handset,
                          &hs_proc_connect_handset_fns, hs_topology_goal_disconnect_handset,
                          CONCURRENT_GOALS_INIT(hs_topology_goal_connectable_handset,
                                                hs_topology_goal_allow_handset_connect)),

    GOAL(hs_topology_goal_disconnect_handset, hs_topology_procedure_disconnect_handset,
         &hs_proc_disconnect_handset_fns, hs_topology_goal_connect_handset), 

    GOAL_WITH_CONCURRENCY(hs_topology_goal_connectable_handset, hs_topology_procedure_enable_connectable_handset,
                          &hs_proc_enable_connectable_handset_fns, hs_topology_goal_none,
                          CONCURRENT_GOALS_INIT(hs_topology_goal_connect_handset,
                                                hs_topology_goal_allow_handset_connect)),

    GOAL_WITH_CONCURRENCY(hs_topology_goal_allow_handset_connect, hs_topology_procedure_allow_handset_connection,
                          &hs_proc_allow_handset_connect_fns, hs_topology_goal_none,
                          CONCURRENT_GOALS_INIT(hs_topology_goal_connectable_handset,
                                                    hs_topology_goal_connect_handset))
};                                                                                                                                                                                                                                                                                                                                                                                                                                             


/******************************************************************************
 * Callbacks for procedure confirmations
 *****************************************************************************/

/*! \brief Handle confirmation of procedure start.
    
    Provided as a callback to procedures.
*/
static void headsetTopology_GoalProcStartCfm(procedure_id proc, procedure_result_t result)
{
    DEBUG_LOG("headsetTopology_GoalProcStartCfm proc 0x%x", proc);

    UNUSED(result);
}

/*! \brief Handle completion of a goal.
  
    Provided as a callback for procedures to use to indicate goal completion.

    Remove the goal and associated procedure from the lists tracking
    active ones.
    May generate events into the rules engine based on the completion
    result of the goal.
*/
static void headsetTopology_GoalProcComplete(procedure_id proc, procedure_result_t result)
{
    headsetTopologyTaskData* td = HeadsetTopologyGetTaskData();
    goal_id completed_goal = GoalsEngine_FindGoalForProcedure(td->goal_set, proc);
    rule_events_t complete_event = GoalsEngine_GetGoalCompleteEvent(td->goal_set, completed_goal, result);

    DEBUG_LOG("headsetTopology_GoalProcComplete proc 0x%x for goal %d", proc, completed_goal);

    if (complete_event)
    {
        DEBUG_LOG("headsetTopology_GoalProcComplete generating event 0x%x", complete_event);
        HeadsetTopologyRules_SetEvent(complete_event);
    }

    /* clear the goal from list of active goals, this may cause further
     * goals to be delivered from the pending_goal_queue_task */
    GoalsEngine_ClearGoal(td->goal_set, completed_goal);
}


/*! \brief Handle confirmation of goal cancellation.

    Provided as a callback for procedures to use to indicate cancellation has
    been completed.
*/
static void headsetTopology_GoalProcCancelCfm(procedure_id proc, procedure_result_t result)
{
    headsetTopologyTaskData* td = HeadsetTopologyGetTaskData();
    goal_id goal = GoalsEngine_FindGoalForProcedure(td->goal_set, proc);

    DEBUG_LOG("headsetTopology_GoalProcCancelCfm proc 0x%x", proc);

    UNUSED(result);

    GoalsEngine_ClearGoal(td->goal_set, goal);
}


/******************************************************************************
 * Handlers for converting rules decisions to goals
 *****************************************************************************/

/*! \brief Determine if a goal is currently being executed. */
bool HeadsetTopology_IsGoalActive(headset_topology_goal_id_t goal)
{
    headsetTopologyTaskData* td = HeadsetTopologyGetTaskData();
    return (GoalsEngine_IsGoalActive(td->goal_set, goal));
}


/*! \brief Check if there are any pending goals. */
bool HeadsetTopology_IsAnyGoalPending(void)
{
    headsetTopologyTaskData* td = HeadsetTopologyGetTaskData();
    return (GoalsEngine_IsAnyGoalPending(td->goal_set));
}


/*! \brief Given a new goal decision from a rules engine, find the goal and attempt to start it. */
void HeadsetTopology_HandleGoalDecision(Task task, MessageId id, Message message)
{
    headsetTopologyTaskData* td = HeadsetTopologyGetTaskData();

    DEBUG_LOG("HeadsetTopology_HandleGoalDecision id 0x%x", id);

    switch (id)
    {
        case HSTOP_GOAL_CONNECTABLE_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, hs_topology_goal_connectable_handset, task, id, message, sizeof(HSTOP_GOAL_CONNECTABLE_HANDSET_T));
            break;

        case HSTOP_GOAL_CONNECT_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, hs_topology_goal_connect_handset, task, id, message, sizeof(HSTOP_GOAL_CONNECT_HANDSET_T));
            break;

        case HSTOP_GOAL_ALLOW_HANDSET_CONNECT:
            GoalsEngine_ActivateGoal(td->goal_set, hs_topology_goal_allow_handset_connect, task, id, message, sizeof(HSTOP_GOAL_ALLOW_HANDSET_CONNECT_T));
            break;

        case HSTOP_GOAL_DISCONNECT_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, hs_topology_goal_disconnect_handset, task, id, message, 0);
            break;

        default:
            DEBUG_LOG("HeadsetTopology_HandleGoalDecision, unknown goal decision %d(0x%x)", id, id);
            break;
    }

    /* Always mark the rule as complete, once the goal has been added. */
    HeadsetTopologyRules_SetRuleComplete(id);
}


void HeadsetTopology_GoalsInit(void)
{
    headsetTopologyTaskData *td = HeadsetTopologyGetTaskData();
    goal_set_init_params_t init_params;

    td->pending_goal_queue_task.handler = HeadsetTopology_HandleGoalDecision;

    memset(&init_params, 0, sizeof(init_params));
    init_params.goals = hsgoals;
    init_params.goals_count = ARRAY_DIM(hsgoals);
    init_params.pending_goal_queue_task = &td->pending_goal_queue_task;

    init_params.proc_result_task = HeadsetTopologyGetTask();
    init_params.proc_start_cfm_fn = headsetTopology_GoalProcStartCfm;
    init_params.proc_cancel_cfm_fn = headsetTopology_GoalProcCancelCfm;
    init_params.proc_complete_cfm_fn = headsetTopology_GoalProcComplete;

    td->goal_set = GoalsEngine_CreateGoalSet(&init_params);
}

