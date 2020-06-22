/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      TWS Topology goal handling.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"

#include "tws_topology_dfu_rules.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_secondary_ruleset.h"

#include "tws_topology_goals.h"
#include "tws_topology_rule_events.h"
#include "tws_topology_config.h"

#include "tws_topology_procedures.h"
#include "tws_topology_procedure_acting_primary_role.h"
#include "tws_topology_procedure_allow_handset_connect.h"
#include "tws_topology_procedure_cancel_find_role.h"
#include "tws_topology_procedure_connect_handset.h"
#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_procedure_disconnect_handset.h"
#include "tws_topology_procedure_disconnect_peer_profiles.h"
#include "tws_topology_procedure_disconnect_peer_find_role.h"
#include "tws_topology_procedure_dfu_role.h"
#include "tws_topology_procedure_dfu_secondary_after_boot.h"
#include "tws_topology_procedure_dfu_primary_after_boot.h"
#include "tws_topology_procedure_find_role.h"
#include "tws_topology_procedure_no_role_find_role.h"
#include "tws_topology_procedure_no_role_idle.h"
#include "tws_topology_procedure_pair_peer.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_pri_connect_peer_profiles.h"
#include "tws_topology_procedure_enable_connectable_peer.h"
#include "tws_topology_procedure_primary_addr_find_role.h"
#include "tws_topology_procedure_primary_find_role.h"
#include "tws_topology_procedure_primary_role.h"
#include "tws_topology_procedure_release_peer.h"
#include "tws_topology_procedure_sec_connect_peer.h"
#include "tws_topology_procedure_secondary_role.h"
#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_set_role.h"
#include "tws_topology_procedure_switch_to_secondary.h"
#include "tws_topology_procedure_handover.h"
#include "tws_topology_procedure_primary_static_handover.h"
#include "tws_topology_procedure_primary_static_handover_in_case.h"
#include "tws_topology_procedure_secondary_static_handover.h"
#include "tws_topology_procedure_dfu_in_case.h"
#include "tws_topology_procedure_dynamic_handover.h"
#include "tws_topology_procedure_dynamic_handover_failure.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"

#include <logging.h>

#include <message.h>
#include <panic.h>

#pragma unitsuppress Unused


static tws_topology_goal_id twsTopology_GetHandoverGoal(hdma_handover_reason_t reason);


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
const goal_entry_t goals[] =
{
    SCRIPT_GOAL(tws_topology_goal_pair_peer, tws_topology_procedure_pair_peer_script,
                &pair_peer_script, tws_topology_goal_none),

    GOAL(tws_topology_goal_find_role, tws_topology_procedure_find_role,
         &proc_find_role_fns, tws_topology_goal_none),

    GOAL_WITH_TIMEOUT_AND_FAIL(tws_topology_goal_secondary_connect_peer, tws_topology_procedure_sec_connect_peer,
                 &proc_sec_connect_peer_fns, tws_topology_goal_none, 
                 TWSTOP_RULE_EVENT_FAILED_PEER_CONNECT,TWSTOP_RULE_EVENT_FAILED_PEER_CONNECT),

    GOAL_WITH_CONCURRENCY_SUCCESS(tws_topology_goal_primary_connect_peer_profiles, tws_topology_procedure_pri_connect_peer_profiles,
                          &proc_pri_connect_peer_profiles_fns, tws_topology_goal_primary_disconnect_peer_profiles,
                          TWSTOP_RULE_EVENT_KICK,
                          CONCURRENT_GOALS_INIT(tws_topology_goal_primary_connectable_peer,
                                                tws_topology_goal_connectable_handset,
                                                tws_topology_goal_connect_handset,
                                                tws_topology_goal_allow_handset_connect,
                                                tws_topology_goal_le_connectable_handset)),

    GOAL(tws_topology_goal_primary_disconnect_peer_profiles, tws_topology_procedure_disconnect_peer_profiles,
         &proc_disconnect_peer_profiles_fns, tws_topology_goal_primary_connect_peer_profiles),

    GOAL_WITH_CONCURRENCY_TIMEOUT(tws_topology_goal_primary_connectable_peer, tws_topology_procedure_enable_connectable_peer,
                                  &proc_enable_connectable_peer_fns, tws_topology_goal_none,
                                  TWSTOP_RULE_EVENT_FAILED_PEER_CONNECT,
                                  CONCURRENT_GOALS_INIT(tws_topology_goal_primary_connect_peer_profiles,
                                                        tws_topology_goal_connect_handset,
                                                        tws_topology_goal_connectable_handset,
                                                        tws_topology_goal_allow_handset_connect,
                                                        tws_topology_goal_le_connectable_handset)),

    SCRIPT_GOAL_CANCEL(tws_topology_goal_no_role_idle, tws_topology_procedure_no_role_idle,
                       &no_role_idle_script, tws_topology_goal_none),

    GOAL(tws_topology_goal_set_role, tws_topology_procedure_set_role,
         &proc_set_role_fns, tws_topology_goal_none),

    GOAL_WITH_CONCURRENCY(tws_topology_goal_connect_handset, tws_topology_procedure_connect_handset,
                          &proc_connect_handset_fns, tws_topology_goal_disconnect_handset,
                          CONCURRENT_GOALS_INIT(tws_topology_goal_primary_connect_peer_profiles,
                                                tws_topology_goal_primary_connectable_peer,
                                                tws_topology_goal_connectable_handset,
                                                tws_topology_goal_allow_handset_connect,
                                                tws_topology_goal_le_connectable_handset)),

    GOAL(tws_topology_goal_disconnect_handset, tws_topology_procedure_disconnect_handset,
         &proc_disconnect_handset_fns, tws_topology_goal_connect_handset),

    GOAL_WITH_CONCURRENCY(tws_topology_goal_connectable_handset, tws_topology_procedure_enable_connectable_handset,
                          &proc_enable_connectable_handset_fns, tws_topology_goal_none,
                          CONCURRENT_GOALS_INIT(tws_topology_goal_primary_connectable_peer,
                                                tws_topology_goal_primary_connect_peer_profiles,
                                                tws_topology_goal_connect_handset,
                                                tws_topology_goal_allow_handset_connect,
                                                tws_topology_goal_le_connectable_handset)),

    SCRIPT_GOAL_CANCEL_SUCCESS(tws_topology_goal_become_primary, tws_topology_procedure_become_primary,
                        &primary_role_script, tws_topology_goal_none, TWSTOP_RULE_EVENT_ROLE_SWITCH),

    SCRIPT_GOAL_CANCEL_SUCCESS(tws_topology_goal_become_secondary, tws_topology_procedure_become_secondary,
                        &secondary_role_script, tws_topology_goal_none, TWSTOP_RULE_EVENT_ROLE_SWITCH),

    SCRIPT_GOAL_SUCCESS(tws_topology_goal_become_acting_primary, tws_topology_procedure_become_acting_primary,
                        &acting_primary_role_script, tws_topology_goal_none, TWSTOP_RULE_EVENT_ROLE_SWITCH),
    
    SCRIPT_GOAL(tws_topology_goal_set_address, tws_topology_procedure_set_address,
                &set_primary_address_script, tws_topology_goal_none),

    SCRIPT_GOAL(tws_topology_goal_set_primary_address_and_find_role, tws_topology_procedure_set_primary_address_and_find_role,
                &primary_address_find_role_script, tws_topology_goal_none),
    
    SCRIPT_GOAL_SUCCESS_TIMEOUT_FAILED(tws_topology_goal_role_switch_to_secondary,
                                       tws_topology_procedure_role_switch_to_secondary,
                                       &switch_to_secondary_script,
                                       tws_topology_goal_none,
                                       TWSTOP_RULE_EVENT_ROLE_SWITCH,
                                       TWSTOP_RULE_EVENT_FAILED_SWITCH_SECONDARY,
                                       TWSTOP_RULE_EVENT_FAILED_SWITCH_SECONDARY),

    SCRIPT_GOAL(tws_topology_goal_no_role_find_role, tws_topology_procedure_no_role_find_role,
                &no_role_find_role_script, tws_topology_goal_none),
    
    GOAL(tws_topology_goal_cancel_find_role, tws_topology_procedure_cancel_find_role,
         &proc_cancel_find_role_fns, tws_topology_goal_none),

    SCRIPT_GOAL(tws_topology_goal_primary_find_role, tws_topology_procedure_primary_find_role,
                &primary_find_role_script, tws_topology_goal_none),

    SCRIPT_GOAL(tws_topology_goal_dfu_role, tws_topology_procedure_dfu_role,
                &dfu_role_script, tws_topology_goal_none),

    SCRIPT_GOAL_SUCCESS(tws_topology_goal_dfu_primary, tws_topology_procedure_dfu_primary,
                &dfu_primary_after_boot_script, tws_topology_goal_none, TWSTOP_RULE_EVENT_DFU_PRI_REBOOT_DONE),

    SCRIPT_GOAL(tws_topology_goal_dfu_secondary, tws_topology_procedure_dfu_secondary,
                &dfu_secondary_after_boot_script, tws_topology_goal_none),

    GOAL(tws_topology_goal_dfu_in_case, tws_topology_procedure_dfu_in_case,
                &proc_dfu_in_case_fns, tws_topology_goal_none),

    SCRIPT_GOAL(tws_topology_goal_disconnect_peer_find_role, tws_topology_procedure_disconnect_peer_find_role,
                &disconnect_peer_find_role_script, tws_topology_goal_none),

    GOAL(tws_topology_goal_release_peer, tws_topology_procedure_release_peer, 
         &proc_release_peer_fns, tws_topology_goal_none),
         
    SCRIPT_GOAL_CANCEL_SUCCESS_TIMEOUT_FAILED(tws_topology_goal_secondary_static_handover,
                                              tws_topology_procedure_secondary_static_handover,
                                              &secondary_static_handover_script, tws_topology_goal_none,
                                              TWSTOP_RULE_EVENT_ROLE_SWITCH,
                                              TWSTOP_RULE_EVENT_STATIC_HANDOVER_FAILED,
                                              TWSTOP_RULE_EVENT_STATIC_HANDOVER_FAILED),

    SCRIPT_GOAL_CANCEL_SUCCESS_FAILED(tws_topology_goal_primary_static_handover_in_case,
                                      tws_topology_procedure_primary_static_handover_in_case,
                                      &primary_static_handover_in_case_script, tws_topology_goal_none,
                                      TWSTOP_RULE_EVENT_ROLE_SWITCH,
                                      TWSTOP_RULE_EVENT_STATIC_HANDOVER_FAILED),
    
    SCRIPT_GOAL_CANCEL_SUCCESS_FAILED(tws_topology_goal_primary_static_handover,
                                      tws_topology_procedure_primary_static_handover,
                                      &primary_static_handover_script, tws_topology_goal_none,
                                      TWSTOP_RULE_EVENT_ROLE_SWITCH,
                                      TWSTOP_RULE_EVENT_STATIC_HANDOVER_FAILED),

    SCRIPT_GOAL_CANCEL_SUCCESS_FAILED(tws_topology_goal_dynamic_handover,
                                      tws_topology_procedure_dynamic_handover,
                                      &dynamic_handover_script, tws_topology_goal_none,
                                      TWSTOP_RULE_EVENT_ROLE_SWITCH,
                                      TWSTOP_RULE_EVENT_HANDOVER_FAILED),
            
    SCRIPT_GOAL_SUCCESS(tws_topology_goal_dynamic_handover_failure, tws_topology_procedure_dynamic_handover_failure,
            &dynamic_handover_failure_script, tws_topology_goal_none, TWSTOP_RULE_EVENT_HANDOVER_FAILURE_HANDLED),

    GOAL_WITH_CONCURRENCY(tws_topology_goal_le_connectable_handset, tws_topology_procedure_enable_le_connectable_handset,
            &proc_enable_le_connectable_handset_fns, tws_topology_goal_none,
            CONCURRENT_GOALS_INIT(tws_topology_goal_allow_handset_connect,
                                  tws_topology_goal_connectable_handset,
                                  tws_topology_goal_connect_handset,
                                  tws_topology_goal_primary_connectable_peer,
                                  tws_topology_goal_primary_connect_peer_profiles)),

    GOAL_WITH_CONCURRENCY(tws_topology_goal_allow_handset_connect, tws_topology_procedure_allow_handset_connection,
                          &proc_allow_handset_connect_fns, tws_topology_goal_none,
                          CONCURRENT_GOALS_INIT(tws_topology_goal_primary_connectable_peer,
                                                tws_topology_goal_connectable_handset,
                                                tws_topology_goal_primary_connect_peer_profiles,
                                                tws_topology_goal_connect_handset,
                                                tws_topology_goal_le_connectable_handset))
};


/******************************************************************************
 * Callbacks for procedure confirmations
 *****************************************************************************/

/*! \brief Handle confirmation of procedure start.
    
    Provided as a callback to procedures.
*/
static void twsTopology_GoalProcStartCfm(procedure_id proc, procedure_result_t result)
{
    DEBUG_LOG("twsTopology_GoalProcStartCfm proc 0x%x", proc);

    UNUSED(result);
}

/*! \brief Handle completion of a goal.
  
    Provided as a callback for procedures to use to indicate goal completion.

    Remove the goal and associated procedure from the lists tracking
    active ones.
    May generate events into the rules engine based on the completion
    result of the goal.
*/
static void twsTopology_GoalProcComplete(procedure_id proc, procedure_result_t result)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    goal_id completed_goal = GoalsEngine_FindGoalForProcedure(td->goal_set, proc);
    rule_events_t complete_event = GoalsEngine_GetGoalCompleteEvent(td->goal_set, completed_goal, result);

    DEBUG_LOG("twsTopology_GoalProcComplete proc 0x%x for goal %d", proc, completed_goal);
    
    /* clear the goal from list of active goals, this may cause further
     * goals to be delivered from the pending_goal_queue_task */
    GoalsEngine_ClearGoal(td->goal_set, completed_goal);

    if (complete_event)
    {
        DEBUG_LOG("twsTopology_GoalProcComplete generating event 0x%08lx%08lx", PRINT_ULL(complete_event));
        twsTopology_RulesSetEvent(complete_event);
    }
}

/*! \brief Handle confirmation of goal cancellation.

    Provided as a callback for procedures to use to indicate cancellation has
    been completed.
*/
static void twsTopology_GoalProcCancelCfm(procedure_id proc, procedure_result_t result)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    goal_id goal = GoalsEngine_FindGoalForProcedure(td->goal_set, proc);

    DEBUG_LOG("twsTopology_GoalProcCancelCfm proc 0x%x", proc);

    UNUSED(result);

    GoalsEngine_ClearGoal(td->goal_set, goal);
}

/******************************************************************************
 * Handlers for converting rules decisions to goals
 *****************************************************************************/

/*! \brief Find and return the relevant handover goal,by mapping the 
           HDMA reason code to topology goal. 
    \param[in] HDMA reason code
*/
tws_topology_goal_id twsTopology_GetHandoverGoal(hdma_handover_reason_t reason)
{
    DEBUG_LOG_INFO("twsTopology_GetHandoverGoal for: %d",reason);
    tws_topology_goal_id goal = tws_topology_goal_none;

    switch(reason)
    {
        case HDMA_HANDOVER_REASON_IN_CASE:
            if (TwsTopologyConfig_DynamicHandoverSupported())
            {
                goal = tws_topology_goal_dynamic_handover;
            }
            else
            {
                goal = tws_topology_goal_primary_static_handover_in_case;
            }
            break;

        case HDMA_HANDOVER_REASON_OUT_OF_EAR:
        case HDMA_HANDOVER_REASON_BATTERY_LEVEL:
        case HDMA_HANDOVER_REASON_VOICE_QUALITY:
        case HDMA_HANDOVER_REASON_EXTERNAL:
		case HDMA_HANDOVER_REASON_SIGNAL_QUALITY:
            if (TwsTopologyConfig_DynamicHandoverSupported())
            {
                goal = tws_topology_goal_dynamic_handover;
            }
            break;

        default:
            DEBUG_LOG_ERROR("twsTopology_GetHandoverGoal invalid HDMA handover reason code 0x%x", reason);
            Panic();
            break;
    }

    return goal;
}

/*! \brief Determine if a goal is currently being executed. */
bool TwsTopology_IsGoalActive(tws_topology_goal_id goal)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    return (GoalsEngine_IsGoalActive(td->goal_set, goal));
}

/*! \brief Check if there are any pending goals. */
bool TwsTopology_IsAnyGoalPending(void)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    return (GoalsEngine_IsAnyGoalPending(td->goal_set));
}

/*! \brief Given a new goal decision from a rules engine, find the goal and attempt to start it. */
void TwsTopology_HandleGoalDecision(Task task, MessageId id, Message message)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();

    DEBUG_LOG("TwsTopology_HandleGoalDecision id 0x%x", id);

    switch (id)
    {
        case TWSTOP_PRIMARY_GOAL_STATIC_HANDOVER_IN_CASE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_primary_static_handover_in_case, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_PAIR_PEER:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_pair_peer, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_SET_PRIMARY_ADDRESS:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_set_address, task, id, PROC_SET_ADDRESS_TYPE_DATA_PRIMARY, sizeof(SET_ADDRESS_TYPE_T));
            break;

        case TWSTOP_PRIMARY_GOAL_SET_PRIMARY_ADDRESS_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_set_primary_address_and_find_role, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_find_role, task, id, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT, sizeof(FIND_ROLE_PARAMS_T));
            break;

        case TWSTOP_PRIMARY_GOAL_CANCEL_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_cancel_find_role, task, id, NULL, 0);
            break;

        case TWSTOP_SECONDARY_GOAL_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_find_role, task, id, PROC_FIND_ROLE_TIMEOUT_DATA_TIMEOUT, sizeof(FIND_ROLE_PARAMS_T));
            break;

        case TWSTOP_SECONDARY_GOAL_CONNECT_PEER:
        case TWSTOP_DFU_GOAL_LINKLOSS_SECONDARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_secondary_connect_peer, task, id, NULL, 0);
            break;

        case TWSTOP_SECONDARY_GOAL_NO_ROLE_IDLE:
        case TWSTOP_DFU_GOAL_NO_ROLE_IDLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_no_role_idle, task, id, NULL, 0);
            break;

        case TWSTOP_DFU_GOAL_IN_CASE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_dfu_in_case, task, id, NULL, 0);
            break;

        case TWSTOP_SECONDARY_GOAL_NO_ROLE_FIND_ROLE:
        case TWSTOP_DFU_GOAL_NO_ROLE_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_no_role_find_role, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_NO_ROLE_IDLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_no_role_idle, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_BECOME_PRIMARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_become_primary, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_BECOME_SECONDARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_become_secondary, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_BECOME_ACTING_PRIMARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_become_acting_primary, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_ROLE_SWITCH_TO_SECONDARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_role_switch_to_secondary, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_PRIMARY_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_primary_find_role, task, id, PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS, sizeof(FIND_ROLE_PARAMS_T));
            break;

        case TWSTOP_PRIMARY_GOAL_CONNECT_PEER_PROFILES:
        case TWSTOP_DFU_GOAL_CONNECT_PEER_PROFILES:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_primary_connect_peer_profiles, task, id, message, sizeof(TWSTOP_PRIMARY_GOAL_CONNECT_PEER_PROFILES_T));
            break;

        case TWSTOP_DFU_GOAL_SEC_DISABLE_PAGE_SCAN_TO_PEER:
            /*
             * Goal to disable page scan on the Secondary post DFU completion.
             * In case of DFU, the earbuds retain the main roles across the 
             * DFU reboot phase. For this the Secondary uses the Primary
             * specific goal to activate page scan, so disable now because with
             * DFU completion, Secondary need not continue page scan.
             */
             /* FALL-THROUGH */
        case TWSTOP_PRIMARY_GOAL_CONNECTABLE_PEER:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_primary_connectable_peer, task, id, message, sizeof(ENABLE_CONNECTABLE_PEER_PARAMS_T));
            break;

        case TWSTOP_PRIMARY_GOAL_DISCONNECT_PEER_PROFILES:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_primary_disconnect_peer_profiles, task, id, message, sizeof(DISCONNECT_PEER_PROFILES_T));
            break;

        case TWSTOP_PRIMARY_GOAL_RELEASE_PEER:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_release_peer, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_CONNECTABLE_HANDSET:
        case TWSTOP_DFU_GOAL_CONNECTABLE_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_connectable_handset, task, id, message, sizeof(ENABLE_CONNECTABLE_HANDSET_PARAMS_T));
            break;

        case TWSTOP_PRIMARY_GOAL_LE_CONNECTABLE_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_le_connectable_handset, task, id, message, sizeof(TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T));
            break;

        case TWSTOP_PRIMARY_GOAL_CONNECT_HANDSET:
        case TWSTOP_DFU_GOAL_CONNECT_HANDSET_ON_RESET:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_connect_handset, task, id, message, sizeof(TWSTOP_PRIMARY_GOAL_CONNECT_HANDSET_T));
            break;

        case TWSTOP_PRIMARY_GOAL_ALLOW_HANDSET_CONNECT:
        case TWSTOP_DFU_GOAL_ALLOW_HANDSET_CONNECT:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_allow_handset_connect, task, id, message, sizeof(ALLOW_HANDSET_CONNECT_PARAMS_T));
            break;

        case TWSTOP_PRIMARY_GOAL_DISCONNECT_PEER_FIND_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_disconnect_peer_find_role, task, id, NULL, 0);
            break;
        
        case TWSTOP_PRIMARY_GOAL_HANDOVER_START:
            GoalsEngine_ActivateGoal(td->goal_set, twsTopology_GetHandoverGoal(td->handover_info.hdma_message.reason), task, id, NULL, 0);
            break;
            
        case TWSTOP_PRIMARY_GOAL_HANDOVER_FAILED:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_dynamic_handover_failure, task, id, NULL, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_DISCONNECT_HANDSET:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_disconnect_handset, task, id, message, 0);
            break;

        case TWSTOP_PRIMARY_GOAL_DFU_ROLE:
        case TWSTOP_SECONDARY_GOAL_DFU_ROLE:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_dfu_role, task, id, message, 0);
            break;

        case TWSTOP_DFU_GOAL_LE_PRI_ABORT_CLEANUP:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_disconnect_handset, task, id, NULL, 0);
            break;

        case TWSTOP_DFU_GOAL_SECONDARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_dfu_secondary, task, id, message, 0);
            break;

        case TWSTOP_DFU_GOAL_PRIMARY:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_dfu_primary, task, id, message, 0);
            break;

        case TWSTOP_SECONDARY_GOAL_STATIC_HANDOVER:
            GoalsEngine_ActivateGoal(td->goal_set, tws_topology_goal_secondary_static_handover, task, id, message, 0);
            break;

        default:
            DEBUG_LOG("TwsTopology_HandleGoalDecision, unknown goal decision %d(0x%x)", id, id);
            break;
    }

    /* Always mark the rule as complete, once the goal has been added.
     * Important to do it now, as some goals may change the role and therefore
     * the rule engine which generated the goal and in which the completion must
     * be marked. */
    twsTopology_RulesMarkComplete(id);
}

void TwsTopology_GoalsInit(void)
{
    twsTopologyTaskData *td = TwsTopologyGetTaskData();
    goal_set_init_params_t init_params;

    td->pending_goal_queue_task.handler = TwsTopology_HandleGoalDecision;

    memset(&init_params, 0, sizeof(init_params));
    init_params.goals = goals;
    init_params.goals_count = ARRAY_DIM(goals);
    init_params.pending_goal_queue_task = &td->pending_goal_queue_task;

    init_params.proc_result_task = TwsTopologyGetTask();
    init_params.proc_start_cfm_fn = twsTopology_GoalProcStartCfm;
    init_params.proc_cancel_cfm_fn = twsTopology_GoalProcCancelCfm;
    init_params.proc_complete_cfm_fn = twsTopology_GoalProcComplete;

    td->goal_set = GoalsEngine_CreateGoalSet(&init_params);
}
