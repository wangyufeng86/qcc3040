/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Implementation for rules used when DFU is pending or in progress

            \implementation The rule set behaviour changes based on whether the 
            current device is using the primary or secondary address. This 
            can be observed in the rule table as the same input event will often 
            have two entries. Only one will fire as the functions used to select
            the rule will only activate for primary/secondary address.
*/

#include "tws_topology_dfu_rules.h"
#include "tws_topology_common_primary_rule_functions.h"
#include "tws_topology_procedure_enable_connectable_peer.h"
#include <bt_device.h>
#include <device_properties.h>
#include <device.h>
#include <device_list.h>
#include <phy_state.h>
#include <rules_engine.h>
#include <connection_manager.h>
#include "handset_service_protected.h"
#include "peer_signalling.h"
#include <device_upgrade.h>
#include "tws_topology_config.h"
#include <logging.h>
#include "tws_topology_goals.h"


#pragma unitsuppress Unused

/*! \{
    Macros for diagnostic output that can be suppressed.*/
#define TWSTOP_DFU_RULE_LOG(...)         DEBUG_LOG(__VA_ARGS__)
/*! \} */

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below */
static rule_action_t twsTopologyDfuRules_CopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.
    Copies the parameters/data into conn_rules where the rules engine can uses
    it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   twsTopologyDfuRules_CopyRunParams(&(x), sizeof(x))

/*! Get pointer to the connection rules task data structure. */
#define TwsTopologyDfuRulesGetTaskData()  (&tws_topology_dfu_rules_task_data)

/*! Storage for the data used by the DFU rules task. */
TwsTopologyDfuRulesTaskData tws_topology_dfu_rules_task_data;

/*! \{
    Rule function prototypes, so we can build the rule tables below. */
DEFINE_RULE(ruleTwsTopDfuInCase);
DEFINE_RULE(ruleTwsTopDfuAlways);
DEFINE_RULE(ruleTwsTopDfuLEPriAbortCleanup);
DEFINE_RULE(ruleTwsTopOutOfCase);
DEFINE_RULE(ruleTwsTopInCase);
DEFINE_RULE(ruleTwsTopDfuPrimary);
DEFINE_RULE(ruleTwsTopDfuSecondary);
DEFINE_RULE(ruleTwsTopDfuSecondaryLinkLoss);
DEFINE_RULE(ruleTwsTopDfuEnableConnectableHandset);
DEFINE_RULE(ruleTwsTopDfuAllowHandsetConnect);
DEFINE_RULE(ruleTwsTopDfuDisablePageScan);
DEFINE_RULE(ruleTwsTopDfuConnectPeerProfiles);
DEFINE_RULE(ruleTwsTopDfuConnectHandsetOnReset);
DEFINE_RULE(ruleTwsTopDfuKickPeerConnect);

/*! \} */

/*! \brief TWS Topology rules deciding behaviour in a Dfu role. */
const rule_entry_t twstop_dfu_rules_set[] =
{
    /* When Earbud is put dfu in case mode, disconnect HFP */
    RULE(TWSTOP_RULE_EVENT_IN_CASE,          ruleTwsTopDfuInCase,             TWSTOP_DFU_GOAL_IN_CASE),

    /* When Earbud put out of the case, disconnect handset (BR/EDR) profiles and links (BR/EDR and/or LE) when DFU transport is BLE. */
    RULE(TWSTOP_RULE_EVENT_OUT_CASE,         ruleTwsTopDfuLEPriAbortCleanup,  TWSTOP_DFU_GOAL_LE_PRI_ABORT_CLEANUP),

    /* When Earbud put out of the case, terminate DFU */
    RULE(TWSTOP_RULE_EVENT_OUT_CASE,         ruleTwsTopDfuAlways,            TWSTOP_DFU_GOAL_NO_ROLE_FIND_ROLE),

    /*! When a DFU is completed, disable page scan on Secondary that was started as part of Secondary reboot. */
    RULE(TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE, ruleTwsTopDfuDisablePageScan,  TWSTOP_DFU_GOAL_SEC_DISABLE_PAGE_SCAN_TO_PEER),

    /* When a DFU is ended, for whatever reason, leave DFU and find role if out of case */
    RULE(TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE,ruleTwsTopOutOfCase,            TWSTOP_DFU_GOAL_NO_ROLE_FIND_ROLE),

    /* When a DFU is ended, for whatever reason, change role if in case */
    RULE(TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE,ruleTwsTopInCase,               TWSTOP_DFU_GOAL_NO_ROLE_IDLE),

    /* Move to correct state when restarted during a DFU and had primary role */
    RULE(TWSTOP_RULE_EVENT_ROLE_SELECTED_PRIMARY, ruleTwsTopDfuPrimary,      TWSTOP_DFU_GOAL_PRIMARY),

    /* When primary earbud is reset during an active Background DFU, decide if Handset connect should run */
    RULE(TWSTOP_RULE_EVENT_ROLE_SELECTED_PRIMARY, ruleTwsTopDfuConnectHandsetOnReset,      TWSTOP_DFU_GOAL_CONNECT_HANDSET_ON_RESET),

    /*! On linkloss with Secondary Earbud during an active Background DFU, decide which profiles to connect. */
    RULE(TWSTOP_RULE_EVENT_DFU_PEER_CONNECTED, ruleTwsTopDfuConnectPeerProfiles, TWSTOP_DFU_GOAL_CONNECT_PEER_PROFILES),

    /* On linkloss with Primary Earbud, decide if Secondary should run connect peer. */
    RULE(TWSTOP_RULE_EVENT_PEER_LINKLOSS, ruleTwsTopDfuSecondaryLinkLoss,  TWSTOP_DFU_GOAL_LINKLOSS_SECONDARY),

    /* Move to correct state when restarted during a DFU and had secondary role */
    RULE(TWSTOP_RULE_EVENT_ROLE_SELECTED_SECONDARY, ruleTwsTopDfuSecondary,  TWSTOP_DFU_GOAL_SECONDARY),

    /*! When handset BREDR link is disconnected, enable page scan on Primary Earbud. */
    RULE(TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_BREDR, ruleTwsTopDfuEnableConnectableHandset, TWSTOP_DFU_GOAL_CONNECTABLE_HANDSET),

    /*! When handset linkloss occurs, enable page scan on Primary Earbud. */
    RULE(TWSTOP_RULE_EVENT_HANDSET_LINKLOSS, ruleTwsTopDfuEnableConnectableHandset, TWSTOP_DFU_GOAL_CONNECTABLE_HANDSET),

    /*! When the role is changed into dfu, allow handset connections in connection manager. */
    RULE(TWSTOP_RULE_EVENT_ROLE_SWITCH, ruleTwsTopDfuAllowHandsetConnect, TWSTOP_DFU_GOAL_ALLOW_HANDSET_CONNECT),

    /*! On linkloss with Primary Earbud during an active Background DFU, decide which profiles to connect. */
    RULE(TWSTOP_RULE_EVENT_PEER_LINKLOSS, ruleTwsTopDfuConnectPeerProfiles, TWSTOP_DFU_GOAL_CONNECT_PEER_PROFILES),

    /*! Since the Primary reboot phase of DFU is done, kick the queued peer connect to complete the post DFU phase (i.e. commit phase). */
    RULE(TWSTOP_RULE_EVENT_DFU_PRI_REBOOT_DONE, ruleTwsTopDfuKickPeerConnect, TWSTOP_DFU_GOAL_NOP),
};

/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Rule to connect with the  handset after an abrupt reset during
           Background DFU. This rule is only for Primary Device. This rule
           establish all the handset profiles connection lost as a result
           of abrupt reset during DFU.
*/
static rule_action_t ruleTwsTopDfuConnectHandsetOnReset(void)
{
    bdaddr handset_addr;

    if (upgradeIsPostRebootPhase())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, ignore as its the post reboot DFU commit phase.");
        return rule_action_ignore;
    }

    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, ignore as in case");
        return rule_action_ignore;
    }

    if (!appDeviceGetHandsetBdAddr(&handset_addr))
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, ignore as not paired with handset");
        return rule_action_ignore;
    }

    if(TwsTopologyGetTaskData()->prohibit_connect_to_handset)
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, ignore as handset connection disabled");
        return rule_action_ignore;
    }

    if (BtDevice_GetWasConnected(&handset_addr))
    {
        device_t handset_device = BtDevice_GetDeviceForBdAddr(&handset_addr);
        uint8 profiles = BtDevice_GetLastConnectedProfilesForDevice(handset_device);

        profiles |= DEVICE_PROFILE_HFP | DEVICE_PROFILE_A2DP;
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, run as handset we were connected to before");
        return RULE_ACTION_RUN_PARAM(profiles);
    }
    else
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectHandsetOnReset, ignored as wasn't connected before");
        return rule_action_ignore;
    }
}
/*! \brief Rule to connect profiles after an abrupt reset during Background DFU.
           This rule is only for Primary Device. This rule establish all the
           profiles connection lost as a result of abrupt reset during DFU.
*/
static rule_action_t ruleTwsTopDfuConnectPeerProfiles(void)
{
    uint8 profiles = TwsTopologyConfig_PeerProfiles();
    bdaddr bd_addr;

    appDeviceGetMyBdAddr(&bd_addr);
    if (appPhyStateGetState() == PHY_STATE_IN_CASE)
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectPeerProfiles ignore as in case");
        return rule_action_ignore;
    }

    if(!appUpgradeIsDFUPrimary())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectPeerProfiles, ignore as Secondary address");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuConnectPeerProfiles run as out of case (profiles:x%x)", profiles);
    return RULE_ACTION_RUN_PARAM(profiles);
}

/*! \brief Rule to enable page scan on Primary when Handset BR/EDR is lost
           owing to disconnection or linkloss. So that the Handset BR/EDR
           connection is restored in order to resume DFU.
           But in case of Handset BR/EDR disconnection owing to Handover then
           DFU is already aborted. Hence DFU resume not expected and so page
           scan is not enabled/ignored.

    Note: Event that trigger this rule evaluation are specific to Primary
          and hence main role of Primary need not be distinguished within.
*/
static rule_action_t ruleTwsTopDfuEnableConnectableHandset(void)
{
    const ENABLE_CONNECTABLE_PEER_PARAMS_T enable_connectable =
        {.enable = TRUE, .auto_disable = FALSE};

    if (appUpgradeIsDfuAbortOnHandoverDone())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuEnableConnectableHandset, DFU is aborted owing to Handover and so don't enable page scan on Primary.");
        return rule_action_ignore;
    }

    /*
     * Both tws_topology_goal_no_role_idle/tws_topology_goal_no_role_find_role
     * include disconnect handset and handset connection disallow procedures.
     * So when either of these goal is active, its ideal to ignore handset
     * disconnection indications. Because if made handset connectable then this
     * can cause only handset ACL connected but inbound profile connections
     * being rejected as the handset connection is disallowed.
     * Only when the role switch is completed, the handset connection is
     * allowed but since the handset is already ACL connected, the subsequent
     * outbound handset profile connection won't be done.
     *
     * Additionally handset disconnection can be owing to handset BT off,
     * power-off or out of range. But for these scenarios,
     * tws_topology_goal_no_role_idle/tws_topology_goal_no_role_find_role
     * won't be activated.
     * For DFU, these goals are normally activated on DFU completion.
     *
     * Based on the above, its fine to make handset connectable only when
     * neither of these are active, so that handset profiles are connected
     * post DFU completion depending on the phy state.
     */
    if (TwsTopology_IsGoalActive(tws_topology_goal_no_role_idle) ||
        TwsTopology_IsGoalActive(tws_topology_goal_no_role_find_role))
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuEnableConnectableHandset, ignore as no-role-idle or no-role-find-role is active");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuEnableConnectableHandset, run as primary dfu in case and not connected to handset");
    return RULE_ACTION_RUN_PARAM(enable_connectable);
}

/*! \brief Rule that runs so long as DFU In Case mode is still there 

    Running this rule set is a key indicator that we are in DFU In Case mode */
static rule_action_t ruleTwsTopDfuInCase(void)
{
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuInCase, DFU IN CASE EVENT RUN Always");
    /* We are here since it is IN_CASE DFU, and its better to update
     * the app_upgrade.APP_STATE_IN_CASE_DFU variable here, so that 
     * during IN_CASE DFU only DFU specific rules are executed.

     * appUpgradeIsAppInCaseDFUMode() and UpgradeIsInProgress() is checked
     * to avoid setting of app_upgrade.APP_STATE_IN_CASE_DFU incorrectly
     * during non-required scenarios such as during Handover.
     */
    if(!appUpgradeIsAppInCaseDFUMode() && !UpgradeIsInProgress())
        appUpgradeStoreAppInCaseDFUState(TRUE);

    return rule_action_run;
}


/*! \brief Rule that runs so long as DFU mode is still selected 

    Running this rule set is a key indicator that we are in DFU mode */
static rule_action_t ruleTwsTopDfuAlways(void)
{
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuAlways, run Always");

    return rule_action_run;
}

/*! \brief Rule that runs to disconnect handset (BR/EDR) profiles and links
           (BR/EDR and/or LE) when DFU transport is BLE.

    Note: As there are no profiles on LE, this is required to synchronize rules
    for handling abort scenarios when DFU transport is BLE.

    Running this rule set ensures that the existing handset profiles
    over BR/EDR are gracefully cleaned up before the links (i.e. BR/EDR and/or
    LE are disconnected. With this approach the earbuds are confirmed to be
    connected in subsequent attempts when both earbuds are moved out of case
    after the abort. */
static rule_action_t ruleTwsTopDfuLEPriAbortCleanup(void)
{
    bool is_ble_connection = ConManagerAnyTpLinkConnected(cm_transport_ble);
    bool is_hs_con_over_ble = HandsetService_Get()->ble_connected;
    bool is_peer_connected;
    bdaddr bd_addr;
    bool is_secondary = FALSE;
    is_peer_connected = appPeerSigIsConnected();
    if (appDeviceGetMyBdAddr(&bd_addr))
        is_secondary = appDeviceIsSecondary((const bdaddr*)&bd_addr);

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, is_ble_connection: %d\n", is_ble_connection);
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, is_hs_con_over_ble: %d\n", is_hs_con_over_ble);
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, is_peer_connected: %d\n", is_peer_connected);
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, self addr: %04x,%02x,%06lx\n", bd_addr.nap, bd_addr.uap, bd_addr.lap);
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, is_secondary: %d\n", is_secondary);

    /** Localize the rule to be run only for the following:
     * - When DFU transport is LE
     *   a) This is an indirect way: is_ble_connection && is_hs_con_over_ble
     *      GAIA_UPGRADE_CONNECT_IND_T is transport agnostic with just the
     *      transport handle AND
     *      Profiles over LE are not integrated into Settings application AND
     *      since in DFU, then
     *      its appropriate to consider that the LE link is setup for DFU.
     *
     * - To handle abort when Primary is put out of case and ignore in case of
     *   handling abort when Secondary is put out of case
     *   a) is_peer_connected && !is_secondary
     *      This cleanup is not required while handling abort when Secondary is
     *      put out of case. Ignoring for Seconday ensures that the earbuds are
     *      connected (including signalling channel) when both earbuds are moved
     *      out of case after the abort.
     */
    if (is_ble_connection && is_hs_con_over_ble &&
        is_peer_connected && !is_secondary)
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, run disconnection of handset (BR/EDR) profiles and links (BR/EDR and/or LE) when DFU transport is BLE.\n");
        return rule_action_run;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuLEPriAbortCleanup, ignored when DFU transport is BR/EDR or handling abort for Secondary out of case when DFU transport is BLE.\n");
    return rule_action_ignore;
}


/*! \brief Rule that runs if out of the case */
static rule_action_t ruleTwsTopOutOfCase(void)
{

    if (!appPhyStateIsOutOfCase())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopOutOfCase, ignore as still in case");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopOutOfCase, run as out of case");
    return rule_action_run;
}


/*! \brief Rule that runs if out of the case */
static rule_action_t ruleTwsTopInCase(void)
{

    if (appPhyStateIsOutOfCase())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopInCase, ignore as out of case");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopInCase, run as in case");
    return rule_action_run;
}


static rule_action_t ruleTwsTopDfuPrimary(void)
{
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuPrimary, run always");

    return rule_action_run;
}


static rule_action_t ruleTwsTopDfuSecondary(void)
{
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuSecondary, run always");

    return rule_action_run;
}

static rule_action_t ruleTwsTopDfuSecondaryLinkLoss(void)
{
    bdaddr primary_addr;
    bdaddr bd_addr;

    appDeviceGetMyBdAddr(&bd_addr);

    if(appDeviceIsPrimary(&bd_addr))
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuSecondaryLinkLoss, ignore as primary address");
        return rule_action_ignore;
    }

    if (!appDeviceGetPrimaryBdAddr(&primary_addr))
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuSecondaryLinkLoss, ignore as unknown primary address");
        return rule_action_ignore;
    }

    if (ConManagerIsConnected(&primary_addr))
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuSecondaryLinkLoss, ignore as peer already connected");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuSecondaryLinkLoss, run as secondary during DFU and peer got disconnected");

    return rule_action_run;
}

static rule_action_t ruleTwsTopDfuAllowHandsetConnect(void)
{
    const bool allow_connect = TRUE;

    /* If we're already connected to handset then don't connect */
    if (appDeviceIsHandsetConnected())
    {
        TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuAllowHandsetConnect, ignore as already connected to handset");
        return rule_action_ignore;
    }

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuAllowHandsetConnect, run as not yet connected to handset");
    return RULE_ACTION_RUN_PARAM(allow_connect);
}

/*! \brief Rule that runs on Secondary to disable Page Scan.

    This disable is for the Page Scan started on Secondary Reboot */
static rule_action_t ruleTwsTopDfuDisablePageScan(void)
{
    const ENABLE_CONNECTABLE_PEER_PARAMS_T disable_connectable =
        {.enable = FALSE, .auto_disable = FALSE};
    bdaddr bd_addr;
    bool is_secondary = FALSE;
    bool is_DfuRebootDone = appUpgradeIsDfuRebootDone();


    if (appDeviceGetMyBdAddr(&bd_addr))
        is_secondary = appDeviceIsSecondary((const bdaddr*)&bd_addr);

    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuDisablePageScan is_secondary:%d, is_DfuRebootDone: %d", is_secondary, is_DfuRebootDone);

    if (is_secondary && is_DfuRebootDone)
        return RULE_ACTION_RUN_PARAM(disable_connectable);

    return  rule_action_ignore;
}

static rule_action_t ruleTwsTopDfuKickPeerConnect(void)
{
    TWSTOP_DFU_RULE_LOG("ruleTwsTopDfuKickPeerConnect, kick peer connect as Primary reboot is completed.");
    UpgradeSetPriRebootDone(TRUE);
    return rule_action_run;
}
/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

void TwsTopologyDfuRules_Init(Task result_task)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = twstop_dfu_rules_set;
    rule_params.rules_count = ARRAY_DIM(twstop_dfu_rules_set);
    rule_params.nop_message_id = TWSTOP_DFU_GOAL_NOP;
    rule_params.event_task = result_task;
    dfu_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);
}

rule_set_t TwsTopologyDfuRules_GetRuleSet(void)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    return dfu_rules->rule_set;
}

void TwsTopologyDfuRules_SetEvent(rule_events_t event_mask)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    RulesEngine_SetEvent(dfu_rules->rule_set, event_mask);
}

void TwsTopologyDfuRules_ResetEvent(rule_events_t event)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    RulesEngine_ResetEvent(dfu_rules->rule_set, event);
}

rule_events_t TwsTopologyDfuRules_GetEvents(void)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    return RulesEngine_GetEvents(dfu_rules->rule_set);
}

void TwsTopologyDfuRules_SetRuleComplete(MessageId message)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    RulesEngine_SetRuleComplete(dfu_rules->rule_set, message);
}

void TwsTopologyDfuRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(dfu_rules->rule_set, message, event);
}

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
 */
static rule_action_t twsTopologyDfuRules_CopyRunParams(const void* param, size_t size_param)
{
    TwsTopologyDfuRulesTaskData *dfu_rules = TwsTopologyDfuRulesGetTaskData();
    RulesEngine_CopyRunParams(dfu_rules->rule_set, param, size_param);
    return rule_action_run_with_param;
}
