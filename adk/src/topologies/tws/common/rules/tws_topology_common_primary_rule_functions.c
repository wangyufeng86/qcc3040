/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Common primary role TWS topology rule functions.
*/

#include "tws_topology_common_primary_rule_functions.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_goals.h"
#include "tws_topology_config.h"
#include "tws_topology_procedure_enable_le_connectable_handset.h"
#include "tws_topology_procedure_enable_connectable_peer.h"
#include "tws_topology_procedure_enable_connectable_handset.h"

#include <bt_device.h>
#include <device_properties.h>
#include <device.h>
#include <device_list.h>
#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include <scofwd_profile.h>
#include <state_proxy.h>
#include <peer_signalling.h>
#include <peer_find_role.h>
#include <device_upgrade.h>
#include <logging.h>

/*! \{
    Macros for diagnostic output that can be suppressed. */
#define TWSTOP_PRIMARY_RULE_LOG(...)         DEBUG_LOG(__VA_ARGS__)
/*! \} */

/*! Types of event that can initiate a connection rule decision. */
typedef enum
{
    /*! Completion of a role switch. */
    rule_connect_role_switch,
    /*! Earbud taken out of the case. */
    rule_connect_out_of_case,
    /*! Completion of handset pairing. (TWS+) */
    rule_connect_pairing,
    /*! Link loss with handset. */
    rule_connect_linkloss,
} rule_connect_reason_t;

rule_action_t ruleTwsTopPriPeerPairedInCase(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerPairedInCase, ignore as not in case");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerPairedInCase, run as peer paired and in the case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriPeerPairedOutCase(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerPairedOutCase, ignore as in case");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerPairedOutCase, ignore as already finding role");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerPairedOutCase, run as peer paired and out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriPairPeer(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPairPeer, run");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriDisableConnectablePeer(void)
{
    const ENABLE_CONNECTABLE_PEER_PARAMS_T disable_connectable =
    {.enable = FALSE, .auto_disable = FALSE};
    bdaddr secondary_addr;
    
    if (!appDeviceGetSecondaryBdAddr(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisableConnectablePeer, ignore as unknown secondary address");
        return rule_action_ignore;
    }
    if (!ConManagerIsConnected(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisableConnectablePeer, ignore as not connected to peer");
        return rule_action_ignore;
    }
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisableConnectablePeer, run as have connection to secondary peer");
    return PRIMARY_RULE_ACTION_RUN_PARAM(disable_connectable);
}

rule_action_t ruleTwsTopPriEnableConnectablePeer(void)
{
    const ENABLE_CONNECTABLE_PEER_PARAMS_T enable_connectable =
    {.enable = TRUE, .auto_disable = TRUE};
    bdaddr secondary_addr;

    if (!appDeviceGetSecondaryBdAddr(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectablePeer, ignore as unknown secondary address");
        return rule_action_ignore;
    }

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectablePeer ignore as in case");
        return rule_action_ignore;
    }
    if (ConManagerIsConnected(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectablePeer ignore as peer connected");
        return rule_action_ignore;
    }

    if (TwsTopology_IsActingPrimary())
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectablePeer ignore as acting primary");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectablePeer, run as out of case and peer not connected");
    return PRIMARY_RULE_ACTION_RUN_PARAM(enable_connectable);
}

rule_action_t ruleTwsTopPriConnectPeerProfiles(void)
{
    uint8 profiles = TwsTopologyConfig_PeerProfiles();

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectPeerProfiles ignore as in case");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectPeerProfiles run as out of case (profiles:x%x)", profiles);
    return PRIMARY_RULE_ACTION_RUN_PARAM(profiles);
}

rule_action_t ruleTwsTopPriReleasePeer(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriReleasePeer run. Device is now in case");
        return rule_action_run;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriReleasePeer ignore. Device not in case (normal)");
    return rule_action_ignore;
}

rule_action_t ruleTwsTopPriSelectedPrimary(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriSelectedPrimary, ignore as in the case");
        return rule_action_ignore;
    }
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriSelectedPrimary, run as selected as Primary out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriSelectedActingPrimary(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriSelectedActingPrimary, ignore as out of case");
        return rule_action_ignore;
    }
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriSelectedActingPrimary, run as selected as Acting Primary out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriNoRoleSelectedSecondary(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriNoRoleSelectedSecondary, ignore as in case");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_none)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriNoRoleSelectedSecondary, ignore as already have role");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriNoRoleSelectedSecondary, run as selected as Secondary out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriPrimarySelectedSecondary(void)
{
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPrimarySelectedSecondary, ignore as in case");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_primary)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPrimarySelectedSecondary, ignore as not primary");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPrimarySelectedSecondary, run as Primary out of case");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriPeerLostFindRole(void)
{
    bdaddr secondary_addr, primary_addr;

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as in case");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_primary)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as not primary");
        return rule_action_ignore;
    }

    if (   TwsTopology_IsGoalActive(tws_topology_goal_no_role_idle)
        || TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role)
        || TwsTopology_IsGoalActive(tws_topology_goal_role_switch_to_secondary))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, defer as switching role");
        return rule_action_defer;
    }

    if (!appDeviceGetSecondaryBdAddr(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as unknown secondary address");
        return rule_action_ignore;
    }
    if (ConManagerIsConnected(&secondary_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as still connected to secondary");
        return rule_action_ignore;
    }

    if(appDeviceGetPrimaryBdAddr(&primary_addr) && UpgradeIsInProgress())
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, ignore as DFU is in progress");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriPeerLostFindRole, run as Primary out of case, and not connected to secondary");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriEnableConnectableHandset(void)
{
    bdaddr handset_addr;
    const ENABLE_CONNECTABLE_HANDSET_PARAMS_T enable_connectable = {.enable = TRUE};

    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    if (ConManagerIsConnected(&handset_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, ignore as connected to handset");
        return rule_action_ignore;
    }

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, ignore as in case ");
        return rule_action_ignore;
    }

    if (TwsTopology_GetRole() != tws_topology_role_primary)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, ignore as role is not primary");
        return rule_action_ignore;
    }

    if (TwsTopology_IsGoalActive(tws_topology_goal_no_role_idle))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, ignore as no-role-idle goal is active");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableConnectableHandset, run as primary out of case not connected to handset");
    return PRIMARY_RULE_ACTION_RUN_PARAM(enable_connectable);
}

rule_action_t ruleTwsTopPriEnableLeConnectableHandset(void)
{
    const TWSTOP_PRIMARY_GOAL_ENABLE_LE_CONNECTABLE_HANDSET_T enable_le_adverts =
        {.enable = TRUE};

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableLeConnectableHandset, ignore as in case ");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriEnableLeConnectableHandset, run as primary out of case not connected to handset");
    return PRIMARY_RULE_ACTION_RUN_PARAM(enable_le_adverts);
}

rule_action_t ruleTwsTopPriDisableConnectableHandset(void)
{
    const ENABLE_CONNECTABLE_HANDSET_PARAMS_T disable_connectable = {.enable = FALSE};

    if (!appDeviceIsHandsetConnected())
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisableConnectableHandset, ignore as not connected with handset");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisableConnectableHandset, run as have connection to handset");
    return PRIMARY_RULE_ACTION_RUN_PARAM(disable_connectable);
}

static rule_action_t ruleTwsTopPriConnectHandset(rule_connect_reason_t reason)
{
    bdaddr handset_addr;

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, reason %u", reason);

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, ignore as in case");
        return rule_action_ignore;
    }

    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    if(TwsTopologyGetTaskData()->prohibit_connect_to_handset)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, ignore as handset connection disabled");
        return rule_action_ignore;
    }

    if (   (reason != rule_connect_linkloss)
        &&
           (   appDeviceIsHandsetA2dpConnected()
            || appDeviceIsHandsetAvrcpConnected()
            || appDeviceIsHandsetHfpConnected()))
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, ignore as already connected to handset");
        return rule_action_ignore;
    }

    if (   BtDevice_GetWasConnected(&handset_addr)
        || (reason == rule_connect_out_of_case))
    {
        device_t handset_device = BtDevice_GetDeviceForBdAddr(&handset_addr);
        uint8 profiles = BtDevice_GetLastConnectedProfilesForDevice(handset_device);

        /* always connect HFP and A2DP if out of case or pairing connect */
        if (   (reason == rule_connect_out_of_case)
            || (reason == rule_connect_pairing))
        {
            profiles |= DEVICE_PROFILE_HFP | DEVICE_PROFILE_A2DP;
        }
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, run as handset we were connected to before");
        return PRIMARY_RULE_ACTION_RUN_PARAM(profiles);
    }
    else
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriConnectHandset, ignored as wasn't connected before");
        return rule_action_ignore;
    }
}

rule_action_t ruleTwsTopPriRoleSwitchConnectHandset(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriRoleSwitchConnectHandset");
    return ruleTwsTopPriConnectHandset(rule_connect_role_switch);
}

rule_action_t ruleTwsTopPriOutCaseConnectHandset(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriOutCaseConnectHandset");
    return ruleTwsTopPriConnectHandset(rule_connect_out_of_case);
}

rule_action_t ruleTwsTopPriHandsetLinkLossReconnect(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriHandsetLinkLossReconnect");
    return ruleTwsTopPriConnectHandset(rule_connect_linkloss);
}

rule_action_t ruleTwsTopPriDisconnectHandset(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriDisconnectHandset");
    return rule_action_run;
}

rule_action_t ruleTwsTopPriInCaseDisconnectHandset(void)
{
    if (appPhyStateGetState() != PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriInCaseDisconnectHandset, ignore as not in case");
        return rule_action_ignore;
    }

    if (!appDeviceIsHandsetAnyProfileConnected())
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriInCaseDisconnectHandset, ignore as not connected to handset");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriInCaseDisconnectHandset, run as in case");
    return rule_action_run;
}

/*! Decide whether to allow handset BR/EDR connections */
rule_action_t ruleTwsTopPriAllowHandsetConnect(void)
{
    const bool allow_connect = TRUE;

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriAllowHandsetConnect, ignore as in case");
        return rule_action_ignore;
    }

    /* If role is not any kind of primary don't allow handsets to connect */
    if (!TwsTopology_IsPrimary())
    {
        TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriAllowHandsetConnect, ignore as not a primary role");
        return rule_action_ignore;
    }

    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriAllowHandsetConnect, run as primary out of case");
    return PRIMARY_RULE_ACTION_RUN_PARAM(allow_connect);
}


/*! Decide whether the use of the DFU role is permitted 

    The initial implementation of this rule works on the basis of selecting the 
    role regardless. 

    The rationale for this
    \li Normal topology rules should not care about DFU details
    \li Keeps the application state machine and topology synchronised

    This may be re-addressed.
*/
rule_action_t ruleTwsTopPriSwitchToDfuRole(void)
{
    TWSTOP_PRIMARY_RULE_LOG("ruleTwsTopPriSwitchToDfuRole, run unconditionally");

    return rule_action_run;
}

