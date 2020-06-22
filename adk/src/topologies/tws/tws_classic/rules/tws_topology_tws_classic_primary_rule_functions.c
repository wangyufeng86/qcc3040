/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      TWS Classic feature specific rule functions.
*/

#include "tws_topology_common_primary_rule_functions.h"
#include "tws_topology_tws_classic_primary_rule_functions.h"
#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_goals.h"
#include "tws_topology_config.h"

#include <bt_device.h>
#include <device_properties.h>
#include <device.h>
#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <scofwd_profile.h>
#include <peer_find_role.h>
#include <device_upgrade.h>
#include <logging.h>

rule_action_t ruleTwsTopClassicPriFindRole(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicPriFindRole, ignore as in case");
        return rule_action_ignore;
    }

    /* if peer pairing is active ignore */
    if (TwsTopology_IsGoalActive(tws_topology_goal_pair_peer))
    {
        DEBUG_LOG("ruleTwsTopClassicPriFindRole, ignore as peer pairing active");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover_in_case))
    {
        DEBUG_LOG("ruleTwsTopClassicPriFindRole, ignore as static handover (in case) is still in progress ");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover))
    {
        DEBUG_LOG("ruleTwsTopClassicPriFindRole, ignore as static handover is still in progress ");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role))
    {
        DEBUG_LOG("ruleTwsTopClassicPriFindRole, ignore as no role find role in progress ");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopClassicPriFindRole, run as not in case");
    return rule_action_run;
}

rule_action_t ruleTwsTopClassicPriNoRoleIdle(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, ignore as out of case");
        return rule_action_ignore;
    }
    if (TwsTopology_IsGoalActive(tws_topology_goal_pair_peer))
    {
        DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, ignore as peer pairing active");
        return rule_action_ignore;
    }
    
    if (ScoFwdIsSending())
    {
        DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, defer as currently forwarding SCO for peer");
        return rule_action_defer;
    }

    if(!PeerFindRole_HasFixedRole())
    {   /* this permits HDMA to react to the IN_CASE and potentially generate a handover event
         * in the first instance */
        if (TwsTopologyGetTaskData()->hdma_created)
        {
            DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, defer as HDMA is Active and will generate handover recommendation shortly");
            return rule_action_defer;
        }
    }

    /* these two rules prevent IN_CASE stopping an in-progress static handover from continuing to run
     * where we've past the point that HDMA has been destroyed */
    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover_in_case))
    {
        DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, defer already running static handover in case");
        return rule_action_defer;
    }
    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover))
    {
        DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, defer already running static handover");
        return rule_action_defer;
    }

    DEBUG_LOG("ruleTwsTopClassicPriNoRoleIdle, run as primary in case");
    return rule_action_run;
}

rule_action_t ruleTwsTopClassicPriPeerLostFindRole(void)
{
    bdaddr secondary_addr, primary_addr;

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as in case");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_primary)
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as not primary");
        return rule_action_ignore;
    }

    if (   TwsTopology_IsGoalActive(tws_topology_goal_no_role_idle)
        || TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role)
        || TwsTopology_IsGoalActive(tws_topology_goal_role_switch_to_secondary))
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, defer as switching role");
        return rule_action_defer;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover))
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as static handover (out of case) active");
        return rule_action_ignore;
    }
        
    if (TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover_in_case))
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as static handover in case still active");
        return rule_action_ignore;
    }

    if (!appDeviceGetSecondaryBdAddr(&secondary_addr))
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as unknown secondary address");
        return rule_action_ignore;
    }
    if (ConManagerIsConnected(&secondary_addr))
    {
        DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, ignore as still connected to secondary");
        return rule_action_ignore;
    }

    if(appDeviceGetPrimaryBdAddr(&primary_addr) && UpgradeIsInProgress())
    {
        DEBUG_LOG("ruleTwsTopClassisPriPeerLostFindRole, ignore as DFU is in progress");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopClassicPriPeerLostFindRole, run as Primary out of case, and not connected to secondary");
    return rule_action_run;
}

rule_action_t ruleTwsTopClassicPriEnableConnectableHandset(void)
{
    bdaddr handset_addr;
    const ENABLE_CONNECTABLE_HANDSET_PARAMS_T enable_connectable = {.enable = TRUE};

    if (   TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover)
        || TwsTopology_IsGoalActive(tws_topology_goal_primary_static_handover_in_case))
    {
        DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, ignore as static handover (out of case) active");
        return rule_action_ignore;
    }

    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    if (ConManagerIsConnected(&handset_addr))
    {
        DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, ignore as connected to handset");
        return rule_action_ignore;
    }

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, ignore as in case ");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_primary)
    {
        DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, ignore as role is not primary");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopClassicPriEnableConnectableHandset, run as primary out of case not connected to handset");
    return PRIMARY_RULE_ACTION_RUN_PARAM(enable_connectable);
}

rule_action_t ruleTwsTopClassicPriInCaseDisconnectHandset(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicPriInCaseDisconnectHandset, ignore as not in case");
        return rule_action_ignore;
    }

    if (ScoFwdIsSending())
    {
        DEBUG_LOG("ruleTwsTopClassicPriInCaseDisconnectHandset, ignore as we're forwarding SCO to secondary");
        return rule_action_ignore;
    }

    if (!appDeviceIsHandsetAnyProfileConnected())
    {
        DEBUG_LOG("ruleTwsTopClassicPriInCaseDisconnectHandset, ignore as not connected to handset");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopClassicPriInCaseDisconnectHandset, run as in case");
    return rule_action_run;
}

/*! Decide whether to run the handover now. 

    The implementation of this rule works on the basis of the following:

     a) Handover is allowed by application now.
     b) FixedRole is not enabled
*/
rule_action_t ruleTwsTopClassicPriHandoverStart(void)
{
    DEBUG_LOG("ruleTwsTopClassicPriHandoverStart");

    /* Check if Handover is allowed by application */
    if(TwsTopologyGetTaskData()->app_prohibit_handover)
    {
        DEBUG_LOG("ruleTwsTopClassicPriHandoverStart, Ignored as App has blocked ");
        return rule_action_ignore;
    }
    
    if (ScoFwdIsSending())
    {
        DEBUG_LOG("ruleTwsTopClassicPriHandoverStart, defer as currently forwarding SCO for peer");
        return rule_action_defer;
    }

    if(PeerFindRole_HasFixedRole())
    {
        DEBUG_LOG("ruleTwsTopClassicPriHandoverStart, Ignored as have a fixed role ");
        return rule_action_ignore;
    }

    /* Run the rule now */
    return rule_action_run;
}

