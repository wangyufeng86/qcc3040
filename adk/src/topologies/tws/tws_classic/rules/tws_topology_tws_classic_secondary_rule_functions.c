/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      TWS Classic feature specific rule functions.
*/

#include "tws_topology_tws_classic_secondary_rule_functions.h"
#include "tws_topology_secondary_ruleset.h"
#include "tws_topology_goals.h"

#include <bt_device.h>
#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <peer_find_role.h>
#include <device_upgrade.h>
#include <logging.h>

/*! \brief Rule to decide if Secondary should start role selection on peer linkloss. */
rule_action_t ruleTwsTopClassicSecPeerLostFindRole(void)
{
    bdaddr secondary_addr;
    if (TwsTopology_IsGoalActive(tws_topology_goal_secondary_static_handover))
    {
        DEBUG_LOG("ruleTwsTopClassicSecPeerLostFindRole, ignore as already running secondary static handover");
        return rule_action_ignore;
    }
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopClassicSecPeerLostFindRole, ignore as in case");
        return rule_action_ignore;
    }

    if(appDeviceGetSecondaryBdAddr(&secondary_addr) && UpgradeIsOutCaseDFU())
    {
        DEBUG_LOG("ruleTwsTopClassicSecPeerLostFindRole, ignore as DFU is in progress");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopClassicSecPeerLostFindRole, run as out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopSecStaticHandoverCommand(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopSecStaticHandoverCommand, ignore as in the case");
        return rule_action_ignore;
    }

    if(PeerFindRole_HasFixedRole())
    {
        DEBUG_LOG("ruleTwsTopSecStaticHandoverCommand, ignore as have a fixed role");
        return rule_action_ignore;
    }

    DEBUG_LOG("ruleTwsTopSecStaticHandoverCommand, run as out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopSecStaticHandoverFailedOutCase(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        DEBUG_LOG("ruleTwsTopSecStaticHandoverFailedOutCase, ignore as in the case");
        return rule_action_ignore;
    }
    
    DEBUG_LOG("ruleTwsTopSecStaticHandoverFailedOutCase, run as out of case");
    return rule_action_run;
}

