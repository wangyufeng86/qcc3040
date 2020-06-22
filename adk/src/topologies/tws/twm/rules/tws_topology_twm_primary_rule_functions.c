/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      TWS feature specific rule functions.
*/

#include "tws_topology_twm_primary_rule_functions.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_goals.h"

#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <mirror_profile.h>

#include <logging.h>

rule_action_t ruleTwsTopTwmPriFindRole(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopTwmPriFindRole, ignore as in case");
        return rule_action_ignore;
    }

    /* if peer pairing is active ignore */
    if (TwsTopology_IsGoalActive(tws_topology_goal_pair_peer))
    {
        DEBUG_LOG("ruleTwsTopTwmPriFindRole, ignore as peer pairing active");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover) 
        || TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover_failure))
    {
        /* Ignore as there is a handover goal running  */
        DEBUG_LOG("ruleTwsTopTwmPriFindRole, Ignore as dynamic handover is still in progress ");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role))
    {
        DEBUG_LOG("ruleTwsTopTwmPriFindRole, ignore as no role find role in progress ");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopTwmPriFindRole, run as not in case");
    return rule_action_run;
}

rule_action_t ruleTwsTopTwmPriNoRoleIdle(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopTwmPriNoRoleIdle, ignore as out of case");
        return rule_action_ignore;
    }
    if (TwsTopology_IsGoalActive(tws_topology_goal_pair_peer))
    {
        DEBUG_LOG("ruleTwsTopTwmPriNoRoleIdle, ignore as peer pairing active");
        return rule_action_ignore;
    }

    /* this permits HDMA to react to the IN_CASE and potentially generate a handover event
     * in the first instance */
    if (TwsTopologyGetTaskData()->hdma_created)
    {
        DEBUG_LOG("ruleTwsTopTwmPriNoRoleIdle, defer as HDMA is Active and will generate handover recommendation shortly");
        return rule_action_defer;
    }

    /* this prevent IN_CASE stopping an in-progress dynamic handover from continuing to run
     * where we've past the point that HDMA has been destroyed */
    if(TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover) 
       || TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover_failure))
    {
        DEBUG_LOG("ruleTwsTopTwmPriNoRoleIdle, Defer as dynamic handover is Active");
        return rule_action_defer;
    }

    DEBUG_LOG("ruleTwsTopTwmPriNoRoleIdle, run as primary in case");
    return rule_action_run;
}

/*! Decide whether to run the handover now. 

    The implementation of this rule works on the basis of the following:

     a) Handover is allowed by application now.
     b) No active goals executing.
*/
rule_action_t ruleTwsTopTwmHandoverStart(void)
{
    if (TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover) 
         || TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover_failure))
    {
        /* Ignore any further handover requests as there is already one in progress */
        DEBUG_LOG("ruleTwsTopTwmHandoverStart, Ignored as dynamic handover is still in progress ");
        return rule_action_ignore;
    }

    /* Check if Handover is allowed by application */
    if(TwsTopologyGetTaskData()->app_prohibit_handover)
    {
        DEBUG_LOG("ruleTwsTopTwmHandoverStart, defer as App has blocked ");
        return rule_action_defer;
    }

    if (   TwsTopology_IsGoalActive(tws_topology_goal_primary_connect_peer_profiles)
        || !MirrorProfile_IsConnected())
    {
        DEBUG_LOG("ruleTwsTopTwmHandoverStart, defer as handover profiles not ready");
        return rule_action_defer;
    }

    /* Run the rule now */
    DEBUG_LOG("ruleTwsTopTwmHandoverStart, run");
    return rule_action_run;
}

/*! A previous handover failed. Run failed rule. */ 
rule_action_t ruleTwsTopTwmHandoverFailed(void)
{
    DEBUG_LOG("ruleTwsTopTwmHandoverFailed");

    /* Run the rule now */
    return rule_action_run;
}

/*! A previous handover failure has been handled. Run failure handled rule. */ 
rule_action_t ruleTwsTopTwmHandoverFailureHandled(void)
{
    DEBUG_LOG("ruleTwsTopTwmHandoverFailureHandled");

    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopTwmHandoverFailureHandled, ignore as not in case anymore");
        return rule_action_ignore;
    }
    /* Run the rule now */
    return rule_action_run;

}

