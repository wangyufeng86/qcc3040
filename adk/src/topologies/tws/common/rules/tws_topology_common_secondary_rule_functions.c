/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Common rule functions for all TWS modes.
*/

#include "tws_topology_common_secondary_rule_functions.h"
#include "tws_topology_secondary_ruleset.h"

#include <bt_device.h>
#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <device_upgrade.h>
#include <logging.h>

#define TWSTOP_SECONDARY_RULE_LOG(...)         DEBUG_LOG(__VA_ARGS__)

/*! \brief Rule to decide if Secondary should start role selection on peer linkloss. */
rule_action_t ruleTwsTopSecPeerLostFindRole(void)
{
    bdaddr secondary_addr;
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecPeerLostFindRole, ignore as in case");
        return rule_action_ignore;
    }

    if(appDeviceGetSecondaryBdAddr(&secondary_addr) && UpgradeIsOutCaseDFU())
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as DFU is in progress");
        return rule_action_ignore;
    }

    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecPeerLostFindRole, run as out of case");
    return rule_action_run;
}

/*! \brief Rule to decide if Secondary should connect to Primary. */
rule_action_t ruleTwsTopSecRoleSwitchPeerConnect(void)
{
    bdaddr primary_addr;
    
    if (!appDeviceGetPrimaryBdAddr(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecRoleSwitchPeerConnect, ignore as unknown primary address");
        return rule_action_ignore;
    }

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecRoleSwitchPeerConnect, ignore as in case");
        return rule_action_ignore;
    }

    if (ConManagerIsConnected(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecRoleSwitchPeerConnect, ignore as peer already connected");
        return rule_action_ignore;
    }

    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecRoleSwitchPeerConnect, run as secondary out of case and peer not connected");
    return rule_action_run;
}

rule_action_t ruleTwsTopSecNoRoleIdle(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecNoRoleIdle, ignore as out of case");
        return rule_action_ignore;
    }
    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecNoRoleIdle, run as secondary in case");
    return rule_action_run;
}

/*! Decide whether the use of the DFU role is permitted 

    The initial implementation of this rule works on the basis of selecting the 
    role regardless. 

    The rationale for this
    \li Normal topology rules should not care about DFU details
    \li Keeps the application state machine and topology synchronised

    This may be re-addressed.
*/
rule_action_t ruleTwsTopSecSwitchToDfuRole(void)
{
    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecSwitchToDfuRole, run unconditionally");

    return rule_action_run;
}


rule_action_t ruleTwsTopSecFailedConnectFindRole(void)
{
    bdaddr primary_addr;

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedConnectFindRole, ignore as in the case");
        return rule_action_ignore;
    }
    
    if (!appDeviceGetPrimaryBdAddr(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedConnectFindRole, ignore as unknown primary address");
        return rule_action_ignore;
    }

    if (ConManagerIsConnected(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedConnectFindRole, ignore as peer already connected");
        return rule_action_ignore;
    }

    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedConnectFindRole, run as secondary out of case with no peer link");
    return rule_action_run;
}

rule_action_t ruleTwsTopSecFailedSwitchSecondaryFindRole(void)
{
    bdaddr primary_addr;

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedSwitchSecondaryFindRole, ignore as in the case");
        return rule_action_ignore;
    }

    if (!appDeviceGetPrimaryBdAddr(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedSwitchSecondaryFindRole, ignore as unknown primary address");
        return rule_action_ignore;
    }

    if((TwsTopology_GetRole() == tws_topology_role_secondary) && ConManagerIsConnected(&primary_addr))
    {
        TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedSwitchSecondaryFindRole, ignore as have secondary role and connected to primary");
        return rule_action_ignore;
    }

    TWSTOP_SECONDARY_RULE_LOG("ruleTwsTopSecFailedSwitchSecondaryFindRole, run as out of case and not a secondary with peer link");
    return rule_action_run;
}