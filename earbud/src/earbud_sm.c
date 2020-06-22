/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Application state machine
*/

/* local includes */
#include "app_task.h"
#include "earbud_sm.h"
#include "earbud_sm_marshal_defs.h"
#include "earbud_anc_tuning.h"
#include "init.h"
#include "earbud_sm_private.h"
#include "earbud_led.h"
#include "adk_log.h"
#include "earbud_rules_config.h"
#include "earbud_common_rules.h"
#include "earbud_primary_rules.h"
#include "earbud_secondary_rules.h"
#include "earbud_config.h"
#include "earbud_production_test.h"

/* framework includes */
#include <domain_message.h>
#include <state_proxy.h>
#include <pairing.h>
#include <power_manager.h>
#include <gaia_framework.h>
#include <device_upgrade.h>
#include <device_upgrade_peer.h>
#include <device_db_serialiser.h>
#include <gatt_handler.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <connection_manager_config.h>
#include <handset_service.h>
#include <hfp_profile.h>
#include <logical_input_switch.h>
#include <scofwd_profile.h>
#include <peer_sco.h>
#include <ui.h>
#include <peer_signalling.h>
#include <key_sync.h>
#include <gatt_server_gatt.h>
#include <device_list.h>
#include <device_properties.h>
#include <profile_manager.h>
#include <tws_topology.h>
#include <peer_find_role.h>
#include <unexpected_message.h>
#include <ui_prompts.h>
#include <ui_tones.h>
#include <anc_state_manager.h>
#include <device_test_service.h>
#include <device_test_service_config.h>

/* system includes */
#include <panic.h>
#include <connection.h>
#include <ps.h>
#include <boot.h>
#include <message.h>

#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define TDL_BASE_PSKEY_INDEX        142
#define TDL_INDEX_PSKEY             141
#define TDL_SIZE                    appConfigMaxTrustedDevices()
#define TEST_SERVICE_ENABLE_PSKEY   DeviceTestService_EnablingPskey()

#pragma unitsuppress Unused
/*!< Application state machine. */
smTaskData app_sm;

static uint16 start_ps_free_count = 0;

const message_group_t sm_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
    UI_INPUTS_DEVICE_STATE_MESSAGE_GROUP
};

static void appSmHandleInternalDeleteHandsets(void);

#ifdef INCLUDE_DFU
/*
Ideal flow of states for DFU is as follows:
APP_GAIA_CONNECTED
    ->APP_GAIA_UPGRADE_CONNECTED
        ->APP_GAIA_UPGRADE_ACTIVITY
            ->APP_GAIA_UPGRADE_DISCONNECTED
                ->APP_GAIA_DISCONNECTED

where,
    APP_GAIA_CONNECTED: A GAIA connection has been made
    APP_GAIA_DISCONNECTED: A GAIA connection has been lost
    APP_GAIA_UPGRADE_CONNECTED: An upgrade protocol has connected through GAIA
    APP_GAIA_UPGRADE_DISCONNECTED:An upgrade protocol has disconnected through GAIA
    APP_GAIA_UPGRADE_ACTIVITY: The GAIA module has seen some upgrade activity

Maintain DFU link state based on GAIA upgrade connection and disconnection.to
avoid abrupt GAIA disconnection observed mainly during abort scenarios handling
for DFU over BLE.
*/
uint16 dfu_link_state;
static void appSmEnterDfuOnStartup(bool upgrade_reboot);
static void appSmNotifyUpgradeStarted(void);
static void appSmSetPeerDfuFlag(bool flag);
static void appSmStartDfuTimer(void);
#endif /* INCLUDE_DFU */

static void appSmHandleDfuEnded(bool error);
static unsigned earbudSm_GetCurrentApplicationContext(void);

/*****************************************************************************
 * SM utility functions
 *****************************************************************************/
static void earbudSm_SendCommandToPeer(marshal_type_t type)
{
    earbud_sm_msg_empty_payload_t* msg = PanicUnlessMalloc(sizeof(earbud_sm_msg_empty_payload_t));
    appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                     PEER_SIG_MSG_CHANNEL_APPLICATION,
                                     msg, type);
}

static void earbudSm_CancelAndSendCommandToPeer(marshal_type_t type)
{
    appPeerSigMarshalledMsgChannelTxCancelAll(SmGetTask(),
                                              PEER_SIG_MSG_CHANNEL_APPLICATION,
                                              type);
    earbudSm_SendCommandToPeer(type);

}

/*! \brief Set event on active rule set */
static void appSmRulesSetEvent(rule_events_t event)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_SetEvent(event);
            break;
        case tws_topology_role_primary:
            CommonRules_SetEvent(event);
            PrimaryRules_SetEvent(event);
            break;
        case tws_topology_role_secondary:
            CommonRules_SetEvent(event);
            SecondaryRules_SetEvent(event);
            break;
        default:
            break;
    }
}

/*! \brief Reset event on active rule set */
static void appSmRulesResetEvent(rule_events_t event)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_ResetEvent(event);
            break;
        case tws_topology_role_primary:
            CommonRules_ResetEvent(event);
            PrimaryRules_ResetEvent(event);
            break;
        case tws_topology_role_secondary:
            CommonRules_ResetEvent(event);
            SecondaryRules_ResetEvent(event);
            break;
        default:
            break;
    }
}

/*! \brief Mark rule action complete on active rule set */
static void appSmRulesSetRuleComplete(MessageId message)
{
    switch (SmGetTaskData()->role)
    {
        case tws_topology_role_none:
            CommonRules_SetRuleComplete(message);
            break;
        case tws_topology_role_primary:
            CommonRules_SetRuleComplete(message);
            PrimaryRules_SetRuleComplete(message);
            break;
        case tws_topology_role_secondary:
            CommonRules_SetRuleComplete(message);
            SecondaryRules_SetRuleComplete(message);
            break;
        default:
            break;
    }
}

static void appSmCancelDfuTimers(void)
{
    DEBUG_LOG_DEBUG("appSmCancelDfuTimers Cancelled all SM_INTERNAL_TIMEOUT_DFU_... timers");

    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_ENTRY);
    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_MODE_START);
    MessageCancelAll(SmGetTask(),SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT);
}

extern bool UpgradePSClearStore(void);
static void appSmClearPsStore(void)
{
    for (int i=0; i<TDL_SIZE; i++)
    {
        PsStore(ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(TDL_BASE_PSKEY_INDEX+i, NULL, 0);
    }
    PsStore(TDL_INDEX_PSKEY, NULL, 0);
    PsStore(TEST_SERVICE_ENABLE_PSKEY, NULL, 0);

    /* Clear out any in progress DFU status */
    UpgradePSClearStore();
}

/*! \brief Delete all peer and handset pairing and reboot device. */
static void appSmDeletePairingAndReset(void)
{
    DEBUG_LOG_DEBUG("appSmDeletePairingAndReset");

    /* cancel the link disconnection, may already be gone if it fired to get us here */
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    appSmClearPsStore();

    appPowerReboot();
}

/*! \brief Force disconnection of the specified links.
    \return Bitfield of links that should be disconnected.
            If zero, all links are disconnected.
            If non-zero, caller needs to wait for link disconnection to complete
 * */
static smDisconnectBits appSmDisconnectLinks(smDisconnectBits links_to_disconnect)
{
    smDisconnectBits disconnecting_links = 0;

    DEBUG_LOG_DEBUG("appSmDisconnectLinks links_to_disconnect 0x%x", links_to_disconnect);

    if (links_to_disconnect & SM_DISCONNECT_PEER)
    {
        /* \todo just commented out for the time being, this is a work-in-progress */
//        if (appDeviceIsPeerConnected())
        {
            appPeerSigDisconnect(SmGetTask());
//            appAvDisconnectPeer();
            ScoFwdDisconnectPeer(SmGetTask());
            disconnecting_links |= SM_DISCONNECT_PEER;
        }
    }
    if (links_to_disconnect & SM_DISCONNECT_HANDSET)
    {
        bdaddr handset_addr = {0};

        if (   appDeviceGetHandsetBdAddr(&handset_addr)
            && appDeviceIsHandsetConnected())
        {
            HandsetService_DisconnectRequest(SmGetTask(), &handset_addr);
            disconnecting_links |= SM_DISCONNECT_HANDSET;
        }
    }
    return disconnecting_links;
}


/*! \brief Determine which state to transition to based on physical state.
    \return appState One of the states that correspond to a physical earbud state.
*/
static appState appSmCalcCoreState(void)
{
    bool busy = appAvIsStreaming() || appHfpIsScoActive();

    switch (appSmGetPhyState())
    {
        case PHY_STATE_IN_CASE:
            return APP_STATE_IN_CASE_IDLE;

        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            return busy ? APP_STATE_OUT_OF_CASE_BUSY :
                          APP_STATE_OUT_OF_CASE_IDLE;

        case PHY_STATE_IN_EAR:
            return busy ? APP_STATE_IN_EAR_BUSY :
                          APP_STATE_IN_EAR_IDLE;

        /* Physical state is unknown, default to in-case state */
        case PHY_STATE_UNKNOWN:
            return APP_STATE_IN_CASE_IDLE;

        default:
            Panic();
    }

    return APP_STATE_IN_EAR_IDLE;
}

static void appSmSetCoreState(void)
{
    appState state = appSmCalcCoreState();
    if (state != appSmGetState())
        appSmSetState(state);
}

/*! \brief Set the core app state for the first time. */
static void appSmSetInitialCoreState(void)
{
    bool prompts_enabled = FALSE;
    smTaskData *sm = SmGetTaskData();

    /* Register with physical state manager to get notification of state
     * changes such as 'in-case', 'in-ear' etc */
    appPhyStateRegisterClient(&sm->task);

    /* Get latest physical state */
    sm->phy_state = appPhyStateGetState();

    /* Generate physical state events */
    switch (sm->phy_state)
    {
        case PHY_STATE_IN_CASE:
            appSmRulesSetEvent(RULE_EVENT_IN_CASE);
            break;

        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_OUT_OF_EAR:
            appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
            appSmRulesSetEvent(RULE_EVENT_OUT_EAR);
            break;

        case PHY_STATE_IN_EAR:
            appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
            appSmRulesSetEvent(RULE_EVENT_IN_EAR);

            prompts_enabled = TRUE;
            break;

        default:
            Panic();
    }

    UiTones_SetTonePlaybackEnabled(prompts_enabled);
    UiPrompts_SetPromptPlaybackEnabled(prompts_enabled);

    /* Update core state */
    appSmSetCoreState();
}

/*! \brief Initiate disconnect of all links */
static void appSmInitiateLinkDisconnection(smDisconnectBits links_to_disconnect, uint16 timeout_ms,
                                           smPostDisconnectAction post_disconnect_action)
{
    smDisconnectBits disconnecting_links = appSmDisconnectLinks(links_to_disconnect);
    MESSAGE_MAKE(msg, SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T);
    if (!disconnecting_links)
    {
        appSmDisconnectLockClearLinks(SM_DISCONNECT_ALL);
    }
    else
    {
        appSmDisconnectLockSetLinks(disconnecting_links);
    }
    msg->post_disconnect_action = post_disconnect_action;
    MessageSendConditionally(SmGetTask(), SM_INTERNAL_LINK_DISCONNECTION_COMPLETE,
                             msg, &appSmDisconnectLockGet());

    /* Start a timer to force reset if we fail to complete disconnection */
    MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION, NULL, timeout_ms);
}


static bool appSmStateNewBleConnectionChange(appState last,appState next)
{
    bool ble_was_allowed = appSmStateAreNewBleConnectionsAllowed(last);
    bool ble_now_allowed = appSmStateAreNewBleConnectionsAllowed(next);
    bool state_has_changed = last != next;

    /* If we are BLE connectable and state has changed may need to re-advertise */
    if (state_has_changed && ble_now_allowed)
        return TRUE;

    if (ble_was_allowed != ble_now_allowed)
        return TRUE;

    return FALSE;
}

/*! \brief Cancel A2DP and SCO out of ear timers. */
static void appSmCancelOutOfEarTimers(void)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP);
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO);

    PrimaryRules_SetRuleComplete(CONN_RULES_A2DP_TIMEOUT_CANCEL);
}

/*! \brief Determine if we have an A2DP restart timer pending. */
bool appSmIsA2dpRestartPending(void)
{
    /* try and cancel the A2DP auto restart timer, if there was one
     * then we're inside the time required to automatically restart
     * audio due to earbud being put back in the ear, so we need to
     * send a play request to the handset */
    return MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START);
}

/*****************************************************************************
 * SM state transition handler functions
 *****************************************************************************/
/*! \brief Enter initialising state

    This function is called whenever the state changes to
    APP_STATE_INITIALISING.
    It is reponsible for initialising global aspects of the application,
    i.e. non application module related state.
*/
static void appEnterInitialising(void)
{
    DEBUG_LOG_DEBUG("appEnterInitialising");
}

/*! \brief Exit initialising state.
 */
static void appExitInitialising(void)
{
    DEBUG_LOG_DEBUG("appExitInitialising");
}

/*! \brief Return SM_UPGRADED_DEFINED_REBOOT for defined reboot phase of upgrade
           else SM_UPGRADED_ABRUPT_RESET for abrupt reset.
 */
static smUpgradeRebootReason appGetUpgradeRebootReason(void)
{
    return SmGetTaskData()->dfu_reboot_reason;
}

/*! \brief Set to SM_UPGRADED_DEFINED_REBOOT for defined reboot phase of upgrade
           else SM_UPGRADED_ABRUPT_RESET for abrupt reset.
 */
static void appSetUpgradeRebootReason(smUpgradeRebootReason val)
{
    SmGetTaskData()->dfu_reboot_reason = val;
}

/*! \brief EnterDfuCheck
 */
static void appEnterDfuCheck(void)
{
    bool is_dfu_mode;
    DEBUG_LOG_DEBUG("appEnterDfuCheck");

    /* We only get into DFU check, if there wasn't a pending DFU
       So send a message to go to startup. */

    if(TwsTopology_GetRole() != tws_topology_role_dfu)
    {
        switch(appGetUpgradeRebootReason())
        {
            case SM_UPGRADED_ABRUPT_RESET:
                /*
                 * Indicates abrupt reset as APP_UPGRADE_REQUESTED_IN_PROGRESS
                 * received from upgrade library.
                 */

                DEBUG_LOG_DEBUG("appEnterDfuCheck - switching to DFU topology post upgrade interrupted owing to abrupt reset.");

                /*! We have entered DFU mode. Make sure that the topology catches up */
                TwsTopology_SwitchToDfuRole();
                appSmSetCoreState();
                break;

            case SM_UPGRADED_DEFINED_REBOOT:
                /*
                 * Indicates abrupt reset as APP_UPGRADE_REQUESTED_IN_PROGRESS
                 * received from upgrade library.
                 */

                DEBUG_LOG_DEBUG("appEnterDfuCheck - switching to DFU topology post defined reboot phase of upgrade.");
                appSmEnterDfuOnStartup(TRUE);
                break;

             default:
                DEBUG_LOG_DEBUG("appEnterDfuCheck -No DFU");
                MessageSend(SmGetTask(), SM_INTERNAL_NO_DFU, NULL);
                break;
        }
    }
    else
        MessageSend(SmGetTask(), SM_INTERNAL_NO_DFU, NULL);
}

/*! \brief Exit
 */
static void appExitDfuCheck(void)
{
    DEBUG_LOG_DEBUG("appExitDfuCheck");
}

/*! \brief Enter actions when we enter the factory reset state.
    The application will drop all links, delete all pairing and reboot.
 */
static void appEnterFactoryReset(void)
{
    DEBUG_LOG_DEBUG("appEnterFactoryReset");
    appSmInitiateLinkDisconnection(SM_DISCONNECT_ALL, appConfigFactoryResetTimeoutMs(),
                                   POST_DISCONNECT_ACTION_NONE);
}

/*! \brief Exit factory reset. */
static void appExitFactoryReset(void)
{
    /* Should never happen */
    Panic();
}

/*! Actions taken when leaving dedicated test mode
*/
static void appExitDeviceTestMode(void)
{
    DEBUG_LOG("appExitDeviceTestMode");

    DeviceTestService_Stop(SmGetTask());
}

/*! Actions taken when entering dedicated test mode
*/
static void appEnterDeviceTestMode(void)
{
    DEBUG_LOG("appEnterDeviceTestMode");

    DeviceTestService_Start(SmGetTask());
}

/*! \brief Actions to take on entering startup state
 */
static void appEnterStartup(void)
{
    DEBUG_LOG_DEBUG("appEnterStartup");
    TwsTopology_Start(SmGetTask());
}

static void appExitStartup(void)
{
    DEBUG_LOG_DEBUG("appExitStartup");
}

/*! \brief Start process to pairing with handset.
 */
static void appEnterHandsetPairing(void)
{
    DEBUG_LOG_DEBUG("appEnterHandsetPairing");

    if(appSmIsUserPairing())
    {
        TwsTopology_ProhibitHandsetConnection(TRUE);
    }
    else
    {
        appSmInitiateLinkDisconnection(SM_DISCONNECT_HANDSET,
                                   appConfigLinkDisconnectionTimeoutTerminatingMs(),
                                   POST_DISCONNECT_ACTION_HANDSET_PAIRING);
    }
}

/*! \brief Exit actions when handset pairing completed.
 */
static void appExitHandsetPairing(void)
{
    smTaskData *sm = SmGetTaskData();

    DEBUG_LOG_DEBUG("appExitHandsetPairing");

    PeerFindRole_UnregisterPrepareClient(&sm->task);
    appSmRulesSetRuleComplete(CONN_RULES_HANDSET_PAIR);
    Pairing_PairStop(NULL);
    appSmClearUserPairing();
    TwsTopology_ProhibitHandsetConnection(FALSE);
}

/*! \brief Enter
 */
static void appEnterInCaseIdle(void)
{
    DEBUG_LOG_DEBUG("appEnterInCaseIdle");
}

/*! \brief Exit
 */
static void appExitInCaseIdle(void)
{
    DEBUG_LOG_DEBUG("appExitInCaseIdle");
}

/*! \brief Enter
 */
static void appEnterInCaseDfu(void)
{
    bool in_case = !appPhyStateIsOutOfCase();
    bool secondary;
    bool is_device_role_set, is_primary_device;

    if (appGetUpgradeRebootReason() == SM_UPGRADED_DEFINED_REBOOT)
    {
        DEBUG_LOG_DEBUG("Enter In case DFU post defined reboot phase");
        UpgradePeerGetDFUInfo(&is_primary_device, NULL);
        secondary = !is_primary_device;
    }
    else
    {
        secondary = !BtDevice_IsMyAddressPrimary();
    }

    DEBUG_LOG_DEBUG("appEnterInCaseDfu Case:%d Primary:%d", in_case, !secondary);

    SmGetTaskData()->enter_dfu_mode = FALSE;

    if (TwsTopology_GetRole() != tws_topology_role_dfu)
    {
        DEBUG_LOG_DEBUG("appEnterInCaseDfu - switching to DFU topology");

        /*! We have entered DFU mode. Make sure that the topology catches up */
        TwsTopology_SwitchToDfuRole();
    }
    

    /* If it is secondary device, store DFU role and device role in PSKEY so 
     * if it is reset before DFU starts on secondary, it goes back to DFU mode.
     *
     * Note: This API is also called just after DFU reset, by then upgrade peer
     * context is not created. In that case, upgrade peer APIs will return without
     * setting the PSKEYs and earlier stored values of PSKEYs will be used.
     */
    is_device_role_set = UpgradePeerSetDeviceRolePrimary(!secondary);

    /* During DFU File Transfer from AG to Primary, if Secondary is reset, 
     * then on reboot, upgradePeerInfo is still NULL, and
     * BtDevice_IsMyAddressPrimary() sometimes return false value.
     * So, its better to rely on stored PSKEY data during such reset to get
     * the Secondary device information.
     */
    if(!is_device_role_set)
    {
        /* Read the information from the UPGRADE_PEER_LIB_PSKEY */
        UpgradePeerGetDFUInfo(&is_primary_device, NULL);
        secondary = !is_primary_device;
    }

    if(secondary)
    {
        UpgradePeerSetDFUMode(TRUE);

        /*! \todo assuming that when we have entered case as secondary, 
            it is appropriate to cancel DFU timeouts 

            Exiting the case will leave DFU mode cleanly */
        if (in_case)
        {
            appSmCancelDfuTimers();
        }
    }

}

/*! \brief Exit
 */
static void appExitInCaseDfu(void)
{
    DEBUG_LOG_DEBUG("appExitInCaseDfu");

    appSmCancelDfuTimers();
    TwsTopology_EndDfuRole();
    /*
     * Reset after DFU role has ended, so that page scan to peer from secondary
     * can be appropriately disabled.
     */
    SmGetTaskData()->dfu_has_been_restarted = FALSE;
    appUpgradeStoreDfuRebootState(FALSE);
    UpgradeSetIsOutCaseDFU(FALSE);

    /* If it is secondary device, clear DFU role in PSKEY on exit */
    if(!BtDevice_IsMyAddressPrimary())
    {
        UpgradePeerSetDFUMode(FALSE);
    }
}

/*! \brief Enter
 */
static void appEnterOutOfCaseIdle(void)
{
    DEBUG_LOG_DEBUG("appEnterOutOfCaseIdle");

    Ui_InformContextChange(ui_provider_phy_state, context_app_sm_out_of_case_idle);

    if (appConfigIdleTimeoutMs())
    {
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE, NULL, appConfigIdleTimeoutMs());
    }
}

/*! \brief Exit
 */
static void appExitOutOfCaseIdle(void)
{
    DEBUG_LOG_DEBUG("appExitOutOfCaseIdle");

    /* Stop idle on LEDs */
    Ui_InformContextChange(ui_provider_phy_state, context_app_sm_out_of_case_busy);

    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IDLE);
}

/*! \brief Enter
 */
static void appEnterOutOfCaseBusy(void)
{
    DEBUG_LOG_DEBUG("appEnterOutOfCaseBusy");
}

/*! \brief Cancel the audio pause timer.

    May have already fired, which is causing us to exit this state, but
    safe to do so.
 */
static void appExitOutOfCaseBusy(void)
{
    DEBUG_LOG_DEBUG("appExitOutOfCaseBusy");
}

/*! \brief Enter. */
static void appEnterOutOfCaseSoporific(void)
{
    DEBUG_LOG_DEBUG("appEnterOutOfCaseSoporific");
}

/*! \brief Exit. */
static void appExitOutOfCaseSoporific(void)
{
    DEBUG_LOG_DEBUG("appExitOutOfCaseSoporific");
}

/*! \brief Exit. */
static void appExitOutOfCaseSoporificTerminating(void)
{
    DEBUG_LOG_DEBUG("appExitOutOfCaseSoporificTerminating");
}

/*! \brief Enter. */
static void appEnterOutOfCaseSoporificTerminating(void)
{
    DEBUG_LOG_DEBUG("appEnterOutOfCaseSoporificTerminating");
}

/*! \brief Common actions when one of the terminating substates is entered.
 */
static void appEnterSubStateTerminating(void)
{
    DEBUG_LOG_DEBUG("appEnterSubStateTerminating");
    DeviceDbSerialiser_Serialise();

    appSmInitiateLinkDisconnection(SM_DISCONNECT_ALL, appConfigLinkDisconnectionTimeoutTerminatingMs(),
                                   POST_DISCONNECT_ACTION_NONE);
}

/*! \brief Common actions when one of the terminating substates is exited.
 */
static void appExitSubStateTerminating(void)
{
    DEBUG_LOG_DEBUG("appExitSubStateTerminating");
}

/*! \brief Enter
 */
static void appEnterInEarIdle(void)
{
    DEBUG_LOG_DEBUG("appEnterInEarIdle");
}

/*! \brief Exit
 */
static void appExitInEarIdle(void)
{
    DEBUG_LOG_DEBUG("appExitInEarIdle");
}

/*! \brief Enter
 */
static void appEnterInEarBusy(void)
{
    DEBUG_LOG_DEBUG("appEnterInEarBusy");
}

/*! \brief Exit
 */
static void appExitInEarBusy(void)
{
    DEBUG_LOG_DEBUG("appExitInEarBusy");
}

/*! \brief Exit APP_STATE_IN_CASE parent function actions. */
static void appExitInCase(void)
{
    DEBUG_LOG_DEBUG("appExitInCase");

    /* run rules for being taken out of the case */
    appSmRulesResetEvent(RULE_EVENT_IN_CASE);
    appSmRulesSetEvent(RULE_EVENT_OUT_CASE);
}

/*! \brief Exit APP_STATE_OUT_OF_CASE parent function actions. */
static void appExitOutOfCase(void)
{
    DEBUG_LOG_DEBUG("appExitOutOfCase");
}

/*! \brief Exit APP_STATE_IN_EAR parent function actions. */
static void appExitInEar(void)
{
    DEBUG_LOG_DEBUG("appExitInEar");

    /* Run rules for out of ear event */
    appSmRulesResetEvent(RULE_EVENT_IN_EAR);
    appSmRulesSetEvent(RULE_EVENT_OUT_EAR);
}

/*! \brief Enter APP_STATE_IN_CASE parent function actions. */
static void appEnterInCase(void)
{
    DEBUG_LOG_DEBUG("appEnterInCase");

    /* Run rules for in case event */
    appSmRulesResetEvent(RULE_EVENT_OUT_CASE);
    appSmRulesSetEvent(RULE_EVENT_IN_CASE);
}

/*! \brief Enter APP_STATE_OUT_OF_CASE parent function actions. */
static void appEnterOutOfCase(void)
{
    DEBUG_LOG_DEBUG("appEnterOutOfCase");
}

/*! \brief Enter APP_STATE_IN_EAR parent function actions. */
static void appEnterInEar(void)
{
    DEBUG_LOG_DEBUG("appEnterInEar");

    /* Run rules for in ear event */
    appSmRulesResetEvent(RULE_EVENT_OUT_EAR);
    appSmRulesSetEvent(RULE_EVENT_IN_EAR);
}

/*****************************************************************************
 * End of SM state transition handler functions
 *****************************************************************************/

/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
void appSmSetState(appState new_state)
{
    appState previous_state = SmGetTaskData()->state;

    DEBUG_LOG_STATE("appSmSetState, state 0x%02x to 0x%02x", previous_state, new_state);

    /* Handle state exit functions */
    switch (previous_state)
    {
        case APP_STATE_NULL:
            /* This can occur when DFU is entered during INIT. */
            break;

        case APP_STATE_INITIALISING:
            appExitInitialising();
            break;

        case APP_STATE_DFU_CHECK:
            appExitDfuCheck();
            break;

        case APP_STATE_FACTORY_RESET:
            appExitFactoryReset();
            break;

        case APP_STATE_DEVICE_TEST_MODE:
            appExitDeviceTestMode();
            break;

        case APP_STATE_STARTUP:
            appExitStartup();
            break;

        case APP_STATE_HANDSET_PAIRING:
            appExitHandsetPairing();
            break;

        case APP_STATE_IN_CASE_IDLE:
            appExitInCaseIdle();
            break;

        case APP_STATE_IN_CASE_DFU:
            appExitInCaseDfu();
            break;

        case APP_STATE_OUT_OF_CASE_IDLE:
            appExitOutOfCaseIdle();
            break;

        case APP_STATE_OUT_OF_CASE_BUSY:
            appExitOutOfCaseBusy();
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC:
            appExitOutOfCaseSoporific();
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
            appExitOutOfCaseSoporificTerminating();
            break;

        case APP_STATE_TERMINATING:
            DEBUG_LOG_DEBUG("appExitTerminating");
            break;

        case APP_STATE_IN_CASE_TERMINATING:
            DEBUG_LOG_DEBUG("appExitInCaseTerminating");
            break;

        case APP_STATE_OUT_OF_CASE_TERMINATING:
            DEBUG_LOG_DEBUG("appExitOutOfCaseTerminating");
            break;

        case APP_STATE_IN_EAR_TERMINATING:
            DEBUG_LOG_DEBUG("appExitInEarTerminating");
            break;

        case APP_STATE_IN_CASE_DISCONNECTING:
            DEBUG_LOG_DEBUG("appExitInCaseDisconnecting");
            break;

        case APP_STATE_OUT_OF_CASE_DISCONNECTING:
            DEBUG_LOG_DEBUG("appExitOutOfCaseDisconnecting");
            break;

        case APP_STATE_IN_EAR_DISCONNECTING:
            DEBUG_LOG_DEBUG("appExitInEarDisconnecting");
            break;

        case APP_STATE_IN_EAR_IDLE:
            appExitInEarIdle();
            break;

        case APP_STATE_IN_EAR_BUSY:
            appExitInEarBusy();
            break;

        default:
            DEBUG_LOG_ERROR("Attempted to exit unsupported state 0x%02x", SmGetTaskData()->state);
            Panic();
            break;
    }

    /* if exiting a sleepy state */
    if (appSmStateIsSleepy(previous_state) && !appSmStateIsSleepy(new_state))
        appPowerClientProhibitSleep(SmGetTask());

    /* if exiting a terminating substate */
    if (appSmSubStateIsTerminating(previous_state) && !appSmSubStateIsTerminating(new_state))
        appExitSubStateTerminating();

    /* if exiting the APP_STATE_IN_CASE parent state */
    if (appSmStateInCase(previous_state) && !appSmStateInCase(new_state))
        appExitInCase();

    /* if exiting the APP_STATE_OUT_OF_CASE parent state */
    if (appSmStateOutOfCase(previous_state) && !appSmStateOutOfCase(new_state))
        appExitOutOfCase();

    /* if exiting the APP_STATE_IN_EAR parent state */
    if (appSmStateInEar(previous_state) && !appSmStateInEar(new_state))
        appExitInEar();

    /* Set new state */
    SmGetTaskData()->state = new_state;

    /* transitioning from connectable to not connectable, or vice versa */
    if (appSmStateIsConnectable(previous_state) != appSmStateIsConnectable(new_state))
    {
        /*! \todo move to topology */
        /* Kick the rules engine. The rule for RULE_EVENT_PAGE_SCAN_UPDATE is
           an always evaluated rule, so the event does not need to be reset */
        appSmRulesSetEvent(RULE_EVENT_PAGE_SCAN_UPDATE);
    }

    /* transitioning from BLE connectable to not connectable, or vice versa */
    if (appSmStateNewBleConnectionChange(previous_state,new_state))
    {
        /*! \todo move to topology */
        appSmRulesSetEvent(RULE_EVENT_BLE_CONNECTABLE_CHANGE);
    }

    /* if entering the APP_STATE_IN_CASE parent state */
    if (!appSmStateInCase(previous_state) && appSmStateInCase(new_state))
        appEnterInCase();

    /* if entering the APP_STATE_OUT_OF_CASE parent state */
    if (!appSmStateOutOfCase(previous_state) && appSmStateOutOfCase(new_state))
        appEnterOutOfCase();

    /* if entering the APP_STATE_IN_EAR parent state */
    if (!appSmStateInEar(previous_state) && appSmStateInEar(new_state))
        appEnterInEar();

    /* if entering a terminating substate */
    if (!appSmSubStateIsTerminating(previous_state) && appSmSubStateIsTerminating(new_state))
        appEnterSubStateTerminating();

    /* if entering a sleepy state */
    if (!appSmStateIsSleepy(previous_state) && appSmStateIsSleepy(new_state))
        appPowerClientAllowSleep(SmGetTask());

    /* Handle state entry functions */
    switch (new_state)
    {
        case APP_STATE_INITIALISING:
            appEnterInitialising();
            break;

        case APP_STATE_DFU_CHECK:
            appEnterDfuCheck();
            break;

        case APP_STATE_FACTORY_RESET:
            appEnterFactoryReset();
            break;

        case APP_STATE_DEVICE_TEST_MODE:
            appEnterDeviceTestMode();
            break;

        case APP_STATE_STARTUP:
            appEnterStartup();
            break;

        case APP_STATE_HANDSET_PAIRING:
            appEnterHandsetPairing();
            break;

        case APP_STATE_IN_CASE_IDLE:
            appEnterInCaseIdle();
            break;

        case APP_STATE_IN_CASE_DFU:
            appEnterInCaseDfu();
            break;

        case APP_STATE_OUT_OF_CASE_IDLE:
            appEnterOutOfCaseIdle();
            break;

        case APP_STATE_OUT_OF_CASE_BUSY:
            appEnterOutOfCaseBusy();
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC:
            appEnterOutOfCaseSoporific();
            break;

        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
            appEnterOutOfCaseSoporificTerminating();
            break;

        case APP_STATE_TERMINATING:
            DEBUG_LOG_DEBUG("appEnterTerminating");
            break;

        case APP_STATE_IN_CASE_TERMINATING:
            DEBUG_LOG_DEBUG("appEnterInCaseTerminating");
            break;

        case APP_STATE_OUT_OF_CASE_TERMINATING:
            DEBUG_LOG_DEBUG("appEnterOutOfCaseTerminating");
            break;

        case APP_STATE_IN_EAR_TERMINATING:
            DEBUG_LOG_DEBUG("appEnterInEarTerminating");
            break;

        case APP_STATE_IN_CASE_DISCONNECTING:
            DEBUG_LOG_DEBUG("appEnterInCaseDisconnecting");
            break;

        case APP_STATE_OUT_OF_CASE_DISCONNECTING:
            DEBUG_LOG_DEBUG("appEnterOutOfCaseDisconnecting");
            break;

        case APP_STATE_IN_EAR_DISCONNECTING:
            DEBUG_LOG_DEBUG("appEnterInEarDisconnecting");
            break;

        case APP_STATE_IN_EAR_IDLE:
            appEnterInEarIdle();
            break;

        case APP_STATE_IN_EAR_BUSY:
            appEnterInEarBusy();
            break;

        default:
            DEBUG_LOG_ERROR("Attempted to enter unsupported state 0x%02x", new_state);
            Panic();
            break;
    }

    Ui_InformContextChange(ui_provider_app_sm, earbudSm_GetCurrentApplicationContext());
}

appState appSmGetState(void)
{
    return SmGetTaskData()->state;
}

/*! \brief Modify the substate of the current parent appState and return the new state. */
static appState appSetSubState(appSubState substate)
{
    appState state = appSmGetState();
    state &= ~APP_SUBSTATE_MASK;
    state |= substate;
    return state;
}

static appSubState appGetSubState(void)
{
    return appSmGetState() & APP_SUBSTATE_MASK;
}

/*! \brief Update the disconnecting links lock status */
static void appSmUpdateDisconnectingLinks(void)
{
    bdaddr remote_bdaddr;
    if (appSmDisconnectLockPeerIsDisconnecting())
    {
        DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks disconnecting peer");
        if (!appDeviceGetPeerBdAddr(&remote_bdaddr) || !ConManagerIsConnected(&remote_bdaddr))
        {
            DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks peer disconnected");
            appSmDisconnectLockClearLinks(SM_DISCONNECT_PEER);
        }
    }
    if (appSmDisconnectLockHandsetIsDisconnecting())
    {
        DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks disconnecting handset");
        if (!appDeviceGetHandsetBdAddr(&remote_bdaddr) || !HandsetService_IsBredrConnected(&remote_bdaddr))
        {
            DEBUG_LOG_DEBUG("appSmUpdateDisconnectingLinks handset disconnected");
            appSmDisconnectLockClearLinks(SM_DISCONNECT_HANDSET);
        }
    }
}

/*! \brief Handle notification of (dis)connections. */
static void appSmHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T* ind)
{
    DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd connected:%d", ind->connected);

    if (ind->ble)
    {
        /*! Although we get this, indicating that the BLE link has changed
            state, our booleans that track this are not yet updated, so
            there is no point kicking the peer synchronisation. */
    }

    if (ind->connected)
    {
        if (!ind->ble)
        {
            bdaddr peer_addr;
            /* A BREDR connection won't neccessarily evaluate rules.
               And if we evaluate them now, the conditions needed for Upgrade control
               will not have changed.  Send a message instead. */
            MessageSend(SmGetTask(), SM_INTERNAL_BREDR_CONNECTED, NULL);
            /* If we are here because of an abrupt reset during BDFU which caused
             * peer (dis)connection;.make sure to trigger the event to connect
             * the peer profiles back.
             */
            DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd appGetUpgradeRebootReason %d", appGetUpgradeRebootReason());
            DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd ind->bd_addr: %04x,%02x,%06lx", ind->bd_addr.nap, ind->bd_addr.uap, ind->bd_addr.lap);
            if(UpgradeIsOutCaseDFU() &&
               (appGetUpgradeRebootReason() == SM_UPGRADED_ABRUPT_RESET) &&
               (appDeviceGetPeerBdAddr(&peer_addr) && BdaddrIsSame(&peer_addr, &ind->bd_addr)))
            {
                bool is_primary_device;
                UpgradePeerGetDFUInfo(&is_primary_device, NULL);
                if(is_primary_device)
                     TwsTopology_SetProfilesOnPeerConnect();
            }
        }
        return;
    }

    appSmUpdateDisconnectingLinks();

    if (ind->ble)
    {
        DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd BLE link");
        return;
    }

    switch (appSmGetState())
    {
        default:
            /*! \todo unless we only get this message on peer disconnect,
             * how is this telling us weve disconnect from the peer, and
             * what use is the print other than debug? */
            /* Check if we've disconnected to peer */
            if (appDeviceIsPeer(&ind->bd_addr))
            {
                DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd, disconnected from peer");
            }
            break;
    }

    /* Check if message was sent due to link-loss */
    if (ind->reason == hci_error_conn_timeout)
    {
        /* Generate link-loss events */
        if (appDeviceIsPeer(&ind->bd_addr))
        {
            DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd, peer link loss");
            appSmRulesSetEvent(RULE_EVENT_PEER_LINK_LOSS);
        }
        else if (appDeviceIsHandset(&ind->bd_addr))
        {
            DEBUG_LOG_DEBUG("appSmHandleConManagerConnectionInd, handset link loss");
            appSmRulesSetEvent(RULE_EVENT_HANDSET_LINK_LOSS);
        }
    }

}

/*! \brief Handle notification of handset disconnection. */
static void appSmHandleHandsetServiceDisconnectedInd(HANDSET_SERVICE_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG("appSmHandleHandsetServiceDisconnectedInd status %u", ind->status);

    appSmUpdateDisconnectingLinks();
}

/*! \brief Handle completion of application module initialisation. */
static void appSmHandleInitConfirm(void)
{
    DEBUG_LOG_INFO("appSmHandleInitConfirm: Initialisation is complete");

    switch (appSmGetState())
    {
        case APP_STATE_INITIALISING:
            appSmSetState(APP_STATE_DFU_CHECK);
            appPowerOn();
            break;

        case APP_STATE_IN_CASE_DFU:
            appPowerOn();
            break;

        case APP_STATE_TERMINATING:
            break;

        default:
            Panic();
    }
}

/*! \brief Handle completion of handset pairing. */
static void appSmHandlePairingPairConfirm(PAIRING_PAIR_CFM_T *cfm)
{
    DEBUG_LOG_DEBUG("appSmHandlePairingPairConfirm, status %d", cfm->status);

    switch (appSmGetState())
    {
        case APP_STATE_HANDSET_PAIRING:
            appSmSetCoreState();
            break;

        case APP_STATE_FACTORY_RESET:
            /* Nothing to do, even if pairing with handset succeeded, the final
            act of factory reset is to delete handset pairing */
            break;

        default:
            /* Ignore, paired with handset with known address as requested by peer */
            break;
    }
}

/*! \brief Handle state machine transitions when earbud removed from the ear,
           or transitioning motionless out of the ear to just out of the ear. */
static void appSmHandlePhyStateOutOfEarEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateOutOfEarEvent state=0x%x", appSmGetState());

    UiPrompts_SetPromptPlaybackEnabled(FALSE);
    UiTones_SetTonePlaybackEnabled(FALSE);

    if (appSmIsCoreState())
        appSmSetCoreState();
}

/*! \brief Handle state machine transitions when earbud motionless out of the ear. */
static void appSmHandlePhyStateOutOfEarAtRestEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateOutOfEarAtRestEvent state=0x%x", appSmGetState());

    if (appSmIsCoreState())
        appSmSetCoreState();
}

/*! \brief Handle state machine transitions when earbud is put in the ear. */
static void appSmHandlePhyStateInEarEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateInEarEvent state=0x%x", appSmGetState());

    UiPrompts_SetPromptPlaybackEnabled(TRUE);
    UiTones_SetTonePlaybackEnabled(TRUE);

    if (appSmIsCoreState())
        appSmSetCoreState();
}

/*! \brief Handle state machine transitions when earbud is put in the case. */
static void appSmHandlePhyStateInCaseEvent(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePhyStateInCaseEvent state=0x%x", appSmGetState());

    /*! \todo Need to add other non-core states to this conditional from which we'll
     * permit a transition back to a core state, such as... sleeping? */
    if (appSmIsCoreState() ||
        (appSmGetState() == APP_STATE_HANDSET_PAIRING))
    {
        appSmSetCoreState();
    }

    /* Inject an event to the rules engine to make sure DFU is enabled */
    appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
}

/*! \brief Handle changes in physical state of the earbud. */
static void appSmHandlePhyStateChangedInd(smTaskData* sm, PHY_STATE_CHANGED_IND_T *ind)
{
    UNUSED(sm);

    DEBUG_LOG_DEBUG("appSmHandlePhyStateChangedInd new phy state %d", ind->new_state);

    sm->phy_state = ind->new_state;

    switch (ind->new_state)
    {
        case PHY_STATE_IN_CASE:
            appSmHandlePhyStateInCaseEvent();
            return;
        case PHY_STATE_OUT_OF_EAR:
            appSmHandlePhyStateOutOfEarEvent();
            return;
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appSmHandlePhyStateOutOfEarAtRestEvent();
            return;
        case PHY_STATE_IN_EAR:
            appSmHandlePhyStateInEarEvent();
            return;
        default:
            break;
    }
}

/*! \brief Take action following power's indication of imminent sleep.
    SM only permits sleep when soporific. */
static void appSmHandlePowerSleepPrepareInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerSleepPrepareInd");

    PanicFalse(APP_STATE_OUT_OF_CASE_SOPORIFIC == appSmGetState());

    appSmSetState(APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING);
}

/*! \brief Handle sleep cancellation. */
static void appSmHandlePowerSleepCancelledInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerSleepCancelledInd");
    /* Sleep can only be entered from a core state, so return to the current core state */
    appSmSetCoreState();
}

/*! \brief Take action following power's indication of imminent shutdown.
    Can be received in any state. */
static void appSmHandlePowerShutdownPrepareInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerShutdownPrepareInd");

    appSmSetState(appSetSubState(APP_SUBSTATE_TERMINATING));
}

/*! \brief Handle shutdown cancelled. */
static void appSmHandlePowerShutdownCancelledInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandlePowerShutdownCancelledInd");
    /* Shutdown can be entered from any state (including non core states), so startup again,
       to determine the correct state to enter. */
    appSmSetState(APP_STATE_STARTUP);
}

static void appSmHandleConnRulesHandsetPair(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesHandsetPair");

    switch (appSmGetState())
    {
        case APP_STATE_OUT_OF_CASE_IDLE:
        case APP_STATE_IN_EAR_IDLE:
            DEBUG_LOG_DEBUG("appSmHandleConnRulesHandsetPair, rule said pair with handset");
            appSmClearUserPairing();
            appSmSetState(APP_STATE_HANDSET_PAIRING);
            break;
        default:
            break;
    }
}

static void appSmHandleConnRulesEnterDfu(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesEnterDfu");

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_IDLE:
        case APP_STATE_IN_CASE_DFU:
            DEBUG_LOG_DEBUG("appSmHandleConnRulesEnterDfu, rule said enter DFU, set enter_dfu_in_case = TRUE");
#ifdef INCLUDE_DFU
            appSmEnterDfuMode();
            SmGetTaskData()->enter_dfu_in_case = TRUE;
            appUpgradeStoreAppInCaseDFUMode(TRUE);
#endif
            break;

        default:
            break;
    }
    appSmRulesSetRuleComplete( CONN_RULES_ENTER_DFU);
}

static void appSmHandleConnRulesExitDfu(void)
{
    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_DFU:
            DEBUG_LOG_DEBUG("appSmHandleConnRulesExitDfu. Finishing, set enter_dfu_in_case = FALSE.");
#ifdef INCLUDE_DFU
            UpgradeAbortDuringDeviceDisconnect();

            appSmHandleDfuEnded(TRUE);
            SmGetTaskData()->enter_dfu_in_case = FALSE;
            appUpgradeStoreAppInCaseDFUMode(FALSE);
#endif
            break;

        default:
            DEBUG_LOG_DEBUG("appSmHandleConnRulesExitDfu. Not active.");
            break;
    }
    appSmRulesSetRuleComplete( CONN_RULES_EXIT_DFU);
}

static void EarbudSm_HandleDfuNotifyPrimary(void)
{
    DEBUG_LOG_DEBUG("EarbudSm_HandleDfuNotifyPrimary");

    earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_ind_dfu_ready_t));
}

/*! \brief Indicate to the peer (Secondary) that DFU has aborted owing to 
           DFU data transfer not initiated with the prescribed timeout period.
*/
static void EarbudSm_HandleDfuStartTimeoutNotifySecondary(void)
{
    DEBUG_LOG_DEBUG("EarbudSm_HandleDfuEndedNotifySecondary");

    earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_ind_dfu_start_timeout_t));
}

static void appSmHandleConnRulesForwardLinkKeys(void)
{
    bdaddr peer_addr;

    DEBUG_LOG_DEBUG("appSmHandleConnRulesForwardLinkKeys");

    /* Can only send this if we have a peer earbud */
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* Attempt to send handset link keys to peer device */
        KeySync_Sync();
    }

    /* In all cases mark rule as done */
    appSmRulesSetRuleComplete(CONN_RULES_PEER_SEND_LINK_KEYS);
}

/*! \brief Start timer to stop A2DP if earbud stays out of the ear. */
static void appSmHandleConnRulesA2dpTimeout(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesA2dpTimeout");

    /* only take action if timeout is configured */
    if (appConfigOutOfEarA2dpTimeoutSecs())
    {
        /* either earbud is out of ear and case */
        if ((appSmIsOutOfCase()) ||
            (StateProxy_IsPeerOutOfCase() && StateProxy_IsPeerOutOfEar())) 
        {
            DEBUG_LOG_DEBUG("appSmHandleConnRulesA2dpTimeout, out of case/ear, so starting %u second timer", appConfigOutOfEarA2dpTimeoutSecs());
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP,
                             NULL, D_SEC(appConfigOutOfEarA2dpTimeoutSecs()));
        }
    }

    appSmRulesSetRuleComplete(CONN_RULES_A2DP_TIMEOUT);
}

/*! \brief Handle decision to start media. */
static void appSmHandleConnRulesMediaPlay(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesMediaPlay media play");

    /* request the handset plays the media player */
    Ui_InjectUiInput(ui_input_play);

    appSmRulesSetRuleComplete(CONN_RULES_MEDIA_PLAY);
}

/*! \brief Start timer to transfer SCO to AG if earbud stays out of the ear. */
static void appSmHandleConnRulesScoTimeout(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesA2dpTimeout");

    if (appSmIsOutOfCase() && appConfigOutOfEarScoTimeoutSecs())
    {
        DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTimeout, out of case/ear, so starting %u second timer", appConfigOutOfEarScoTimeoutSecs());
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO,
                         NULL, D_SEC(appConfigOutOfEarScoTimeoutSecs()));
    }

    appSmRulesSetRuleComplete(CONN_RULES_SCO_TIMEOUT);
}

/*! \brief Accept incoming call if the primary/secondary EB enters in ear */
static void appSmHandleConnRulesAcceptincomingCall(void)
{
    DEBUG_LOG("appSmHandleConnRulesAcceptincomingCall");
    appHfpCallAccept();
    appSmRulesSetRuleComplete(CONN_RULES_ACCEPT_INCOMING_CALL);
}

/*! \brief Transfer SCO from AG as earbud is in the ear. */
static void appSmHandleConnRulesScoTransferToEarbud(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTransferToEarbud, in ear transferring audio this %u peer %u", appSmIsInEar(), StateProxy_IsPeerInEar());
    appHfpTransferToHeadset();
    appSmRulesSetRuleComplete(CONN_RULES_SCO_TRANSFER_TO_EARBUD);
}

static void appSmHandleConnRulesScoTransferToHandset(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesScoTransferToHandset");
    appHfpTransferToAg();
    appSmRulesSetRuleComplete(CONN_RULES_SCO_TRANSFER_TO_HANDSET);
}

/*! \brief Turns LEDs on */
static void appSmHandleConnRulesLedEnable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLedEnable");

    LedManager_Enable(TRUE);
    appSmRulesSetRuleComplete(CONN_RULES_LED_ENABLE);
}

/*! \brief Turns LEDs off */
static void appSmHandleConnRulesLedDisable(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesLedDisable");

    LedManager_Enable(FALSE);
    appSmRulesSetRuleComplete(CONN_RULES_LED_DISABLE);
}

/*! \brief Pause A2DP as an earbud is out of the ear for too long. */
static void appSmHandleInternalTimeoutOutOfEarA2dp(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutOutOfEarA2dp out of ear pausing audio");

    /* Double check we're still out of case when the timeout occurs */
    if (appSmIsOutOfCase() ||
        ((StateProxy_IsPeerOutOfEar()) && (StateProxy_IsPeerOutOfCase())))
    {
        /* request the handset pauses the media player */
        Ui_InjectUiInput(ui_input_pause);

        /* start the timer to autorestart the audio if earbud is placed
         * back in the ear. */
        if (appConfigInEarA2dpStartTimeoutSecs())
        {
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START,
                             NULL, D_SEC(appConfigInEarA2dpStartTimeoutSecs()));
        }
    }
}

/*! \brief Nothing to do. */
static void appSmHandleInternalTimeoutInEarA2dpStart(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutInEarA2dpStart");

    /* no action needed, we're just using the presence/absence of an active
     * timer when going into InEar to determine if we need to restart audio */
}

/*! \brief Transfer SCO to AG as earbud is out of the ear for too long. */
static void appSmHandleInternalTimeoutOutOfEarSco(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutOutOfEarSco out of ear transferring audio");

    /* If we're forwarding SCO, check peer state as transferring SCO on master will
     * also remove SCO on slave */
    if (ScoFwdIsSending())
    {
        /* Double check we're still out of case and peer hasn't gone back in the
         * ear when the timeout occurs */
        if (appSmIsOutOfCase() && !StateProxy_IsPeerInEar())
        {
            /* Transfer SCO to handset */
            appHfpTransferToAg();
        }
    }
    else
    {
        /* Double check we're still out of case */
        if (appSmIsOutOfCase())
        {
            /* Transfer SCO to handset */
            appHfpTransferToAg();
        }
    }
}

/*! \brief Switch which microphone is used during MIC forwarding. */
static void appSmHandleConnRulesSelectMic(CONN_RULES_SELECT_MIC_T* crsm)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesSelectMic selected mic %u", crsm->selected_mic);

    switch (crsm->selected_mic)
    {
        case MIC_SELECTION_LOCAL:
            appKymeraScoUseLocalMic();
            break;
        case MIC_SELECTION_REMOTE:
            appKymeraScoUseRemoteMic();
            break;
        default:
            break;
    }

    appSmRulesSetRuleComplete(CONN_RULES_SELECT_MIC);
}

static void appSmHandleConnRulesPeerScoControl(CONN_RULES_PEER_SCO_CONTROL_T* crpsc)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesPeerScoControl peer sco %u", crpsc->enable);

    if (crpsc->enable)
    {
        PeerSco_Enable();
    }
    else
    {
        PeerSco_Disable();
    }

    appSmRulesSetRuleComplete(CONN_RULES_PEER_SCO_CONTROL);
}

/*! \brief Handle confirmation of SM marhsalled msg TX, free the message memory. */
static void appSm_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    /* Handled Factory reset only */
    UNUSED(cfm);
}

static void appSm_DisconnectAndSwitchRole(bool become_primary)
{
    DEBUG_LOG_DEBUG("appSm_DisconnectAndSwitchRole");

    smPostDisconnectAction post_disconnect_action = become_primary ? POST_DISCONNECT_ACTION_SWITCH_TO_PRIMARY :
                                                                 POST_DISCONNECT_ACTION_SWITCH_TO_SECONDARY;

    appSmInitiateLinkDisconnection(SM_DISCONNECT_ALL,
                                   appConfigLinkDisconnectionTimeoutTerminatingMs(),
                                   post_disconnect_action);
    appSmSetState(appSetSubState(APP_SUBSTATE_DISCONNECTING));
}

/*! \brief Handle message from peer notifying of update of MRU handset. */
static void appSm_HandlePeerMruHandsetUpdate(earbud_sm_ind_mru_handset_t *msg)
{
    DEBUG_LOG_DEBUG("appSm_HandlePeerMruHandsetUpdate");

    if(msg)
    {
        DEBUG_LOG_DEBUG("appSm_HandlePeerMruHandsetUpdate address %04x,%02x,%06lx",
                    msg->bd_addr.lap,
                    msg->bd_addr.uap,
                    msg->bd_addr.nap);

        appDeviceUpdateMruDevice(&msg->bd_addr);
    }
}

/*! \brief Handle message from peer notifying of deleting handset. */
static void appSmHandlePeerDeleteHandsetWhenFull(earbud_sm_req_delete_handset_if_full_t *msg)
{
    DEBUG_LOG("appSmHandlePeerDeleteHandsetWhenFull");

    if(msg)
    {
        DEBUG_LOG("appSmHandlePeerDeleteHandsetWhenFull address %04x,%02x,%06lx", 
                    msg->bd_addr.lap,
                    msg->bd_addr.uap,
                    msg->bd_addr.nap);

        if (!appDeviceDelete(&msg->bd_addr))
        {
            DEBUG_LOG_WARN("appSmHandlePeerDeleteHandsetWhenFull WARNING device is still connected");
        }
    }
}

/*! \brief Handle SM->SM marshalling messages.
    
    Currently the only handling is to make the Primary/Secondary role switch
    decision.
*/
static void appSm_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch(ind->type)
    {
        case MARSHAL_TYPE(earbud_sm_req_dfu_active_when_in_case_t):
            appSmEnterDfuModeInCase(TRUE, FALSE);
            break;

        case MARSHAL_TYPE(earbud_sm_req_dfu_active_when_out_case_t):
            DEBUG_LOG_DEBUG("Background Upgrade Started in Primary - switch Secondary to DFU topology");

            /*! We have entered DFU mode in out of case DFU. Make sure that the topology catches up */
            TwsTopology_SwitchToDfuRole();
            /* Set the various Upgrade specfic variable states appropriately */
            UpgradeSetIsOutCaseDFU(TRUE);
            UpgradePeerSetDFUMode(TRUE);
            UpgradePeerSetDeviceRolePrimary(FALSE);
            break;

        case MARSHAL_TYPE(earbud_sm_ind_dfu_ready_t):
            DEBUG_LOG_DEBUG("earbud_sm_ind_dfu_ready_indication received from Peer");
            break;

        case MARSHAL_TYPE(earbud_sm_ind_dfu_start_timeout_t):
            DEBUG_LOG_DEBUG("earbud_sm_ind_dfu_start_timeout_t ind received from Peer");
            /*
             * Peer (Primary) has indicated that DFU has aborted owing to DFU
             * data transfer not initiated with the prescribed timeout period.
             * Hence exit DFU so that the earbud states are synchronized.
             */
            appSmHandleDfuEnded(TRUE);
            break;

        case MARSHAL_TYPE(earbud_sm_req_stereo_audio_mix_t):
            DEBUG_LOG_DEBUG("earbud_sm_req_stereo_audio_mix_t received from Peer");
            appKymeraSetStereoLeftRightMix(TRUE);
            break;

        case MARSHAL_TYPE(earbud_sm_req_mono_audio_mix_t):
            DEBUG_LOG_DEBUG("earbud_sm_req_mono_audio_mix_t received from Peer");
            appKymeraSetStereoLeftRightMix(FALSE);
            break;

        case MARSHAL_TYPE(earbud_sm_ind_mru_handset_t):
            DEBUG_LOG_DEBUG("earbud_sm_ind_mru_handset_t received from Peer");
            appSm_HandlePeerMruHandsetUpdate((earbud_sm_ind_mru_handset_t*)ind->msg);
            break;

        case MARSHAL_TYPE(earbud_sm_req_delete_handsets_t):
            DEBUG_LOG_DEBUG("earbud_sm_req_delete_handsets_t received from Peer");
            appSmDeleteHandsets();
            break;

        case MARSHAL_TYPE(earbud_sm_req_delete_handset_if_full_t):
            DEBUG_LOG_DEBUG("earbud_sm_req_delete_handset_if_full_t received from Peer");
            appSmHandlePeerDeleteHandsetWhenFull((earbud_sm_req_delete_handset_if_full_t*)ind->msg);
            break;

        default:
            break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

/*! \brief Use pairing activity to generate rule event to check for link keys that may need forwarding. */
static void appSm_HandlePairingActivity(const PAIRING_ACTIVITY_T* pha)
{
    if (pha->status == pairingCompleteVersionChanged)
    {
        appSmRulesSetEvent(RULE_EVENT_PEER_UPDATE_LINKKEYS);
    }
    else if (pha->status == pairingLinkKeyReceived)
    {
        appSmRulesSetEvent(RULE_EVENT_RX_HANDSET_LINKKEY);
    }
}

/*! \brief Idle timeout */
static void appSmHandleInternalTimeoutIdle(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalTimeoutIdle");
    PanicFalse(APP_STATE_OUT_OF_CASE_IDLE == appSmGetState());
    appSmSetState(APP_STATE_OUT_OF_CASE_SOPORIFIC);
}

static void appSmHandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    if (ind->status == peerSigStatusConnected)
    {
        appSmRulesResetEvent(RULE_EVENT_PEER_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_PEER_CONNECTED);
    }
    else
    {
        appSmRulesResetEvent(RULE_EVENT_PEER_CONNECTED);
        appSmRulesSetEvent(RULE_EVENT_PEER_DISCONNECTED);
    }
}

static void appSmHandleAvA2dpConnectedInd(const AV_A2DP_CONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleAvA2dpConnectedInd %04x,%02x,%06lx", ind->bd_addr.nap, ind->bd_addr.uap, ind->bd_addr.lap);

    if (appDeviceIsHandset(&ind->bd_addr))
    {
        appSmRulesSetRuleComplete(CONN_RULES_CONNECT_HANDSET);
        appSmRulesResetEvent(RULE_EVENT_HANDSET_A2DP_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_HANDSET_A2DP_CONNECTED);
    }
}

static void appSmHandleAvA2dpDisconnectedInd(const AV_A2DP_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleAvA2dpDisconnectedInd, reason %u inst %x", ind->reason, ind->av_instance);

    switch (appSmGetState())
    {
        default:
        {
            if (appDeviceIsHandset(&ind->bd_addr))
            {
                /* Clear connected and set disconnected events */
                appSmRulesResetEvent(RULE_EVENT_HANDSET_A2DP_CONNECTED);
                appSmRulesSetEvent(RULE_EVENT_HANDSET_A2DP_DISCONNECTED);
            }
        }
        break;
    }
}

static void appSmHandleAvAvrcpConnectedInd(const AV_AVRCP_CONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleAvAvrcpConnectedInd %04x,%02x,%06lx", ind->bd_addr.nap, ind->bd_addr.uap, ind->bd_addr.lap);

    if (appDeviceIsHandset(&ind->bd_addr))
    {
        appSmRulesResetEvent(RULE_EVENT_HANDSET_AVRCP_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_HANDSET_AVRCP_CONNECTED);
    }

    if (appDeviceIsPeer(&ind->bd_addr))
    {
#ifdef INCLUDE_SCOFWD
        /* Use wallclock on this connection to synchronise RINGs */
        ScoFwdSetWallclock(ind->sink);
#endif /* INCLUDE_SCOFWD */
        appSmRulesResetEvent(RULE_EVENT_PEER_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_PEER_CONNECTED);
    }
}

static void appSmHandleAvAvrcpDisconnectedInd(const AV_AVRCP_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleAvAvrcpDisconnectedInd");

    switch (appSmGetState())
    {
        default:
        {
            if (appDeviceIsHandset(&ind->bd_addr))
            {
                appSmRulesResetEvent(RULE_EVENT_HANDSET_AVRCP_CONNECTED);
                appSmRulesSetEvent(RULE_EVENT_HANDSET_AVRCP_DISCONNECTED);
            }

            if (appDeviceIsPeer(&ind->bd_addr))
            {
#ifdef INCLUDE_SCOFWD
                ScoFwdSetWallclock(0);
#endif /* INCLUDE_SCOFWD */
                appSmRulesResetEvent(RULE_EVENT_PEER_CONNECTED);
                appSmRulesSetEvent(RULE_EVENT_PEER_DISCONNECTED);
            }
        }
        break;
    }
}

static void appSmHandleAvStreamingActiveInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleAvStreamingActiveInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
        appSmSetCoreState();
}

static void appSmHandleAvStreamingInactiveInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleAvStreamingInactiveInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
        appSmSetCoreState();
}

static void appSmHandleHfpConnectedInd(APP_HFP_CONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpConnectedInd %04x,%02x,%06lx", ind->bd_addr.nap, ind->bd_addr.uap, ind->bd_addr.lap);

    if (appDeviceIsHandset(&ind->bd_addr))
    {
        appSmRulesSetRuleComplete( CONN_RULES_CONNECT_HANDSET);
        appSmRulesResetEvent(RULE_EVENT_HANDSET_HFP_DISCONNECTED);
        appSmRulesSetEvent(RULE_EVENT_HANDSET_HFP_CONNECTED);
    }
}

static void appSmHandleHfpDisconnectedInd(APP_HFP_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpDisconnectedInd, reason %u", ind->reason);

    switch (appSmGetState())
    {
        default:
        {
            if (appDeviceIsHandset(&ind->bd_addr))
            {
                appSmRulesResetEvent(RULE_EVENT_HANDSET_HFP_CONNECTED);
                appSmRulesSetEvent(RULE_EVENT_HANDSET_HFP_DISCONNECTED);
            }
        }
        break;
    }
}

static void appSmHandleHfpScoConnectedInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpScoConnectedInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
        appSmSetCoreState();

    /* this was getting set by scofwd...
       evaluates whether to turn on SCO forwarding
       \todo decide if this is the correct course or if scofwd should
             have an event listened to by SM that sets this event. */
    appSmRulesSetEvent(RULE_EVENT_SCO_ACTIVE);

#ifdef INCLUDE_DFU
    UpgradeSetScoActive(TRUE);
    DEBUG_LOG("UpgradeSetScoActive state = TRUE");
#endif

}

static void appSmHandleHfpScoDisconnectedInd(void)
{
    DEBUG_LOG_DEBUG("appSmHandleHfpScoDisconnectedInd state=0x%x", appSmGetState());

    /* We only care about this event if we're in a core state,
     * and it could be dangerous if we're not */
    if (appSmIsCoreState())
        appSmSetCoreState();

#ifdef INCLUDE_DFU
    UpgradeSetScoActive(FALSE);
    DEBUG_LOG("UpgradeSetScoActive state = FALSE");
#endif
}

/*! \brief Notify peer of deleting handset */
static void appSmHandleNotifyPeerDeleteHandset(bdaddr bd_addr)
{

    earbud_sm_req_delete_handset_if_full_t* msg = PanicUnlessMalloc(sizeof(earbud_sm_req_delete_handset_if_full_t));

    DEBUG_LOG_DEBUG("appSmHandleNotifyPeerDeleteHandset address %04x,%02x,%06lx", bd_addr.lap, bd_addr.uap, bd_addr.nap);

    msg->bd_addr = bd_addr;

    appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                     PEER_SIG_MSG_CHANNEL_APPLICATION,
                                     msg,
                                     MARSHAL_TYPE(earbud_sm_req_delete_handset_if_full_t));

}

bool appSmHandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    bool handled = FALSE;
    UNUSED(already_handled);

    switch(id)
    {
        case CL_SM_AUTH_DEVICE_DELETED_IND:
            {
                CL_SM_AUTH_DEVICE_DELETED_IND_T *ind = (CL_SM_AUTH_DEVICE_DELETED_IND_T *)message;
                bdaddr *bd_addr = &ind->taddr.addr;
                device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);

                if (device)
                {
                    uint16 flags = 0;
                    Device_GetPropertyU16(device, device_property_flags, &flags);
                    if ((flags & DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS) == 0)
                    {
                        appSmHandleNotifyPeerDeleteHandset(ind->taddr.addr);
                    }
                }

                handled = TRUE;
            }
            break;

        default:
            break;
    }

    return handled;
}

static void appSmHandleInternalPairHandset(void)
{
    if (appSmStateIsIdle(appSmGetState()) && appSmIsPrimary())
    {
        appSmSetUserPairing();
        appSmSetState(APP_STATE_HANDSET_PAIRING);
    }
    else
        DEBUG_LOG_DEBUG("appSmHandleInternalPairHandset can only pair once role has been decided and is in IDLE state");
}

/*! \brief Delete pairing for all handsets.
    \note There must be no connections to a handset for this to succeed. */
static void appSmHandleInternalDeleteHandsets(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalDeleteHandsets");

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_IDLE:
        case APP_STATE_OUT_OF_CASE_IDLE:
        case APP_STATE_IN_EAR_IDLE:
        case APP_STATE_FACTORY_RESET:
        {
            appSmInitiateLinkDisconnection(SM_DISCONNECT_HANDSET, appConfigLinkDisconnectionTimeoutTerminatingMs(),
                                                POST_DISCONNECT_ACTION_DELETE_HANDSET_PAIRING);
            break;
        }
        default:
            DEBUG_LOG_WARN("appSmHandleInternalDeleteHandsets bad state %u",
                                                        appSmGetState());
            break;
    }
}

/*! \brief Handle request to start factory reset. */
static void appSmHandleInternalFactoryReset(void)
{
    if (appSmGetState() >= APP_STATE_STARTUP)
    {
        DEBUG_LOG_DEBUG("appSmHandleInternalFactoryReset");
        appSmSetState(APP_STATE_FACTORY_RESET);
    }
    else
    {
        DEBUG_LOG_WARN("appSmHandleInternalFactoryReset can only factory reset in IDLE state");
    }
}

/*! \brief Handle failure to successfully disconnect links within timeout.
    Manually tear down the ACL and wait for #SM_INTERNAL_LINK_DISCONNECTION_COMPLETE.
*/
static void appSmHandleInternalLinkDisconnectionTimeout(void)
{
    bdaddr addr;

    DEBUG_LOG_DEBUG("appSmHandleInternalLinkDisconnectionTimeout");
    if (appSmDisconnectLockPeerIsDisconnecting())
    {
    if (appDeviceGetPeerBdAddr(&addr) && ConManagerIsConnected(&addr))
        {
            DEBUG_LOG_DEBUG("appSmHandleInternalLinkDisconnectionTimeout PEER IS STILL CONNECTED");
            ConManagerSendCloseAclRequest(&addr, TRUE);
        }
        else
        {
            appSmDisconnectLockClearLinks(SM_DISCONNECT_PEER);
        }
    }
    if (appSmDisconnectLockHandsetIsDisconnecting())
    {
        if (appDeviceGetHandsetBdAddr(&addr) && HandsetService_IsBredrConnected(&addr))
        {
            DEBUG_LOG_DEBUG("appSmHandleInternalLinkDisconnectionTimeout HANDSET IS STILL CONNECTED");
            ConManagerSendCloseAclRequest(&addr, TRUE);
        }
        else
        {
            appSmDisconnectLockClearLinks(SM_DISCONNECT_HANDSET);
        }
    }
}

#ifdef INCLUDE_DFU
/*! Change the state to DFU mode with an optional timeout

    \param timeoutMs Timeout (in ms) after which the DFU state
    may be exited
 */
static void appSmHandleEnterDfuWithTimeout(uint32 timeoutMs)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_ENTRY);
    if (timeoutMs)
    {
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_ENTRY, NULL, timeoutMs);
    }

    /* Set the app state to IN_CASE_DFU only when devices reboot after the
     * upgrade is completed and the state is not set as IN_CASE_DFU.
     * During data transfer from AG to Prim in BDFU, if abrupt reset takes place,
     * don't set the state.
     * The Upgrade state and is it BDFU or not are used to figure that out.
     */
     
    if ((!UpgradeIsOutCaseDFU() || appGetUpgradeRebootReason() != SM_UPGRADED_ABRUPT_RESET) &&
        APP_STATE_IN_CASE_DFU != appSmGetState())
    {
        DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout. restarted SM_INTERNAL_TIMEOUT_DFU_ENTRY - %dms",timeoutMs);

        appSmSetState(APP_STATE_IN_CASE_DFU);
        /* After re-boot during DFU, app_upgrade.APP_STATE_IN_CASE_DFU need to be
         * set, so that only DFU specific rules get executed during IN_CASE DFU
         */
        appUpgradeStoreAppInCaseDFUState(TRUE);
    }
    else
    {
        DEBUG_LOG_DEBUG("appSmHandleEnterDfuWithTimeout restarted SM_INTERNAL_TIMEOUT_DFU_ENTRY. Either Already in IN_CASE_DFU or BDFU in progress - %dms.",
                        timeoutMs);
    }

    appSmRulesSetEvent(RULE_EVENT_DFU_CONNECT);
}

static void appSmHandleEnterDfuUpgraded(void)
{
    appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutToStartAfterRestartMs());

    SmGetTaskData()->dfu_has_been_restarted = TRUE;
    appUpgradeStoreDfuRebootState(TRUE);
}

static void appSmHandleEnterDfuStartup(void)
{
    appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutToStartAfterRebootMs());

    SmGetTaskData()->dfu_has_been_restarted = FALSE;
    appUpgradeStoreDfuRebootState(FALSE);
}

static void appSmHandleDfuEnded(bool error)
{
    DEBUG_LOG_DEBUG("appSmHandleDfuEnded(%d), clear dfu_in_progress and enter_dfu_in_case",error);

    SmGetTaskData()->dfu_in_progress = FALSE;
    SmGetTaskData()->enter_dfu_in_case = FALSE;
    /* As we are ending DFU, clear the DFU state variables */
    SmGetTaskData()->dfu_has_been_restarted = FALSE;
    SmGetTaskData()->peer_dfu_in_progress = FALSE;
    appUpgradeStoreAppInCaseDFUMode(FALSE);

    DEBUG_LOG_DEBUG("appSmHandleDfuEnded, Clear the is_out_case_dfu PSKEY value");
    UpgradeSetIsOutCaseDFU(FALSE);
    DEBUG_LOG_DEBUG("appSmHandleDfuEnded, Reset the reboot reason flag.");
    appSetUpgradeRebootReason(SM_UPGRADED_NONE);

    if (appSmGetState() == APP_STATE_IN_CASE_DFU)
    {
        appSmSetState(APP_STATE_STARTUP);
        /*
         * Once in-case DFU has ended (successfully or aborted), reset the state
         * maintained by upgrade in order to appropriately distinguish out of
         * case and in case DFU especially when these DFUs are alternated.
         */
        appUpgradeStoreAppInCaseDFUState(FALSE);
        DEBUG_LOG("appSmHandleDfuEnded appUpgradeIsAppInCaseDFUState:%d", appUpgradeIsAppInCaseDFUState());

        /* This insignificant for the Secondary and hence skipped. */
        if (BtDevice_IsMyAddressPrimary() && error)
        {
            gaiaFrameworkInternal_GaiaDisconnect();
        }
    }
}


static void appSmHandleUgradePeerStarted(void)
{
    DEBUG_LOG_DEBUG("appSmHandleUgradePeerStarted");
    
    appSmNotifyUpgradeStarted();
}


static void appSmHandleUpgradeConnected(void)
{
    /*
     * Set the main role (Primary/Secondary) so that the DFU Protocol PDUs
     * (UPGRADE_HOST_SYNC_REQ, UPGRADE_HOST_START_REQ &
     * UPGRADE_HOST_START_DATA_REQ) exchanged before the actual data transfer
     * can rightly handled based on its main role especially.
     * This is much required especially after a successful handover where the
     * main roles are swapped and upgrade library won't be aware of the
     * true/effective main roles until the DFU role is setup post the initial
     * DFU Protocol PDU exchange and before actual start of DFU data transfer.
     */
    (void)UpgradePeerSetDeviceRolePrimary(BtDevice_IsMyAddressPrimary());

    switch (appSmGetState())
    {
        case APP_STATE_IN_CASE_DFU:
            /* If we are in the special DFU case mode then start a fresh time
               for dfu mode as well as a timer to trap for an application that
               opens then closes the upgrade connection. */
            DEBUG_LOG_DEBUG("appSmHandleStartUpgrade, valid state to enter DFU");
            appSmStartDfuTimer();

            DEBUG_LOG_DEBUG("appSmHandleUpgradeConnected Start SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT %dms",
                                appConfigDfuTimeoutCheckForSupportMs());
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT,
                                NULL, appConfigDfuTimeoutCheckForSupportMs());
            break;

        default:
            /* In all other states and modes we don't need to do anything.
               Start of an actual upgrade will be blocked if needed,
               see appSmHandleDfuAllow() */
            break;
    }
}


static void appSmHandleUpgradeDisconnected(void)
{
    DEBUG_LOG_DEBUG("appSmHandleUpgradeDisconnected");
    smTaskData* sm = SmGetTaskData();

    if (MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT))
    {
        DEBUG_LOG_DEBUG("appSmHandleUpgradeDisconnected Cancelled SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT timer");

        appSmStartDfuTimer();
    }
    /* Abort the upgrade of Initiator and Peer device if GAIA app disconnection
     * indication comes */
    if(sm->peer_dfu_in_progress)
    {
        appUpgradeAbortDuringDeviceDisconnect();
        /* Set this to FALSE, since Peer DFU has aborted now */
        appSmSetPeerDfuFlag(FALSE);
    }
}

#endif


static void appSmHandleNoDfu(void)
{

    if (appSmGetState() != APP_STATE_IN_CASE_DFU)
    {
        DEBUG_LOG_DEBUG("appSmHandleNoDfu. Not gone into DFU, so safe to assume we can continue");

        if (!DeviceTestService_TestMode())
        {
            appSmSetState(APP_STATE_STARTUP);
        }
        else
        {
            appSmSetState(APP_STATE_DEVICE_TEST_MODE);
        }
    }
    else
    {
        DEBUG_LOG_DEBUG("appSmHandleNoDfu. We are in DFU mode !");
    }
}

/*! \brief Reboot the earbud, no questions asked. */
static void appSmHandleInternalReboot(void)
{
    BootSetMode(BootGetMode());
}

static void appSm_ContinuePairingHandsetAfterDisconnect(void)
{
    smTaskData *sm = SmGetTaskData();
    PeerFindRole_RegisterPrepareClient(&sm->task);
    Pairing_Pair(SmGetTask(), appSmIsUserPairing());
}

/*! \brief Handle indication all requested links are now disconnected. */
static void appSmHandleInternalAllRequestedLinksDisconnected(SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T *msg)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalAllRequestedLinksDisconnected 0x%x", appSmGetState());
    MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION);

    switch (msg->post_disconnect_action)
    {
        case POST_DISCONNECT_ACTION_HANDOVER_AND_PLAY_MEDIA:
        case POST_DISCONNECT_ACTION_HANDOVER:
        break;
        case POST_DISCONNECT_ACTION_HANDSET_PAIRING:
            appSm_ContinuePairingHandsetAfterDisconnect();
            break;
        case POST_DISCONNECT_ACTION_DELETE_HANDSET_PAIRING:
            BtDevice_DeleteAllHandsetDevices();
            break;
        case POST_DISCONNECT_ACTION_NONE:
        default:
        break;
    }

    switch (appSmGetState())
    {
        case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
            appPowerSleepPrepareResponse(SmGetTask());
        break;
        case APP_STATE_FACTORY_RESET:
            appSmDeletePairingAndReset();
        break;
        default:
            switch (appGetSubState())
            {
                case APP_SUBSTATE_TERMINATING:
                    appPowerShutdownPrepareResponse(SmGetTask());
                    break;
                case APP_SUBSTATE_DISCONNECTING:
                    appSmSetCoreState();
                    break;
                default:
                    break;
            }
        break;
    }
}

static void appSm_HandleTwsTopologyRoleChange(tws_topology_role new_role)
{
    tws_topology_role old_role = SmGetTaskData()->role;

    switch (new_role)
    {
        case tws_topology_role_none:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange NONE. DFU role:%d, DFU state:%d",
                        old_role == tws_topology_role_dfu,
                        APP_STATE_IN_CASE_DFU == appSmGetState());

            /* May need additional checks here */
            if (APP_STATE_IN_CASE_DFU != appSmGetState())
            {
                SmGetTaskData()->role = tws_topology_role_none;

                TaskList_MessageSendId(SmGetTaskData()->client_tasks, EARBUD_ROLE_PRIMARY);

                SecondaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
                PrimaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            }
            else
            {
                DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange staying in role:%d, DFU was in progress (app)", old_role);
            }
            break;

        case tws_topology_role_primary:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange PRIMARY");
//            appSm_SwitchRole(TRUE);
            SmGetTaskData()->role = tws_topology_role_primary;
            LogicalInputSwitch_SetRerouteToPeer(FALSE);
            StateProxy_SetRole(TRUE);
            UiPrompts_GenerateUiEvents(TRUE);
            UiTones_GenerateUiEvents(TRUE);

            TaskList_MessageSendId(SmGetTaskData()->client_tasks, EARBUD_ROLE_PRIMARY);

            PrimaryRules_SetEvent(RULE_EVENT_ROLE_SWITCH);
            if (appPhyStateGetState() != PHY_STATE_IN_CASE)
            {
                PrimaryRules_SetEvent(RULE_EVENT_OUT_CASE);
            }
            break;

        case tws_topology_role_secondary:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange SECONDARY");
//            appSm_SwitchRole(FALSE);
            SmGetTaskData()->role = tws_topology_role_secondary;
            LogicalInputSwitch_SetRerouteToPeer(TRUE);
            StateProxy_SetRole(FALSE);
            UiPrompts_GenerateUiEvents(FALSE);
            UiTones_GenerateUiEvents(FALSE);

            TaskList_MessageSendId(SmGetTaskData()->client_tasks, EARBUD_ROLE_SECONDARY);

            SecondaryRules_SetEvent(RULE_EVENT_ROLE_SWITCH);
            if (appPhyStateGetState() != PHY_STATE_IN_CASE)
            {
                SecondaryRules_SetEvent(RULE_EVENT_OUT_CASE);
            }

            /* As a primary the earbud may have been in a non-core state, in
               particular, it may have been handset pairing. When transitioning
               to secondary, force a transition to a core state so any primary
               role activities are cancelled as a result of the state transition
             */
            if (old_role == tws_topology_role_primary)
            {
                if (!appSmIsCoreState())
                {
                    appSmSetCoreState();
                }
            }
            break;

        case tws_topology_role_dfu:
            DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChange DFU. Make no changes");
            if (SmGetTaskData()->dfu_has_been_restarted)
            {
                bool is_primary_device;
                UpgradePeerGetDFUInfo(&is_primary_device, NULL);
                TwsTopology_SelectPrimaryAddress(is_primary_device);
            }
            break;

        default:
            break;
    }
}

static void appSm_HandleTwsTopologyStartCfm(TWS_TOPOLOGY_START_CFM_T* cfm)
{
    DEBUG_LOG_STATE("appSm_HandleTwsTopologyStartCfm sts %u role %u", cfm->status, cfm->role);

    if (appSmGetState() == APP_STATE_STARTUP)
    {
        /* topology up and running, SM can move on from STARTUP state */
        appSmSetInitialCoreState();

        if (cfm->status == tws_topology_status_success)
        {
            appSm_HandleTwsTopologyRoleChange(cfm->role);
        }
    }
    else
    {
        DEBUG_LOG_WARN("appSm_HandleTwsTopologyStartCfm unexpected state %u", appSmGetState());
    }
}

static void appSm_HandleTwsTopologyRoleChangedInd(TWS_TOPOLOGY_ROLE_CHANGED_IND_T* ind)
{
    DEBUG_LOG_STATE("appSm_HandleTwsTopologyRoleChangedInd role %u", ind->role);

    appSm_HandleTwsTopologyRoleChange(ind->role);
}


static void appSmHandleDfuAllow(const CONN_RULES_DFU_ALLOW_T *dfu)
{
#ifdef INCLUDE_DFU
    appUpgradeAllowUpgrades(dfu->enable);
#else
    UNUSED(dfu);
#endif
}


static void appSmHandleInternalBredrConnected(void)
{
    DEBUG_LOG_DEBUG("appSmHandleInternalBredrConnected");

    /* Kick the state machine to see if this changes DFU state.
       A BREDR connection for upgrade can try an upgrade before we
       allow it. Always evaluate events do not need resetting. */
    appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
}


/*! \brief provides Earbud Application state machine context to the User Interface module.

    \param[in]  void

    \return     current_sm_ctxt - current context of sm module.
*/
static unsigned earbudSm_GetCurrentPhyStateContext(void)
{
    phy_state_provider_context_t context = BAD_CONTEXT;

    if (appSmIsOutOfCase())
    {
        context = context_phy_state_out_of_case;
    }
    else
    {
        context = context_phy_state_in_case;
    }

    return (unsigned)context;
}

static unsigned earbudSm_GetCurrentApplicationContext(void)
{
    app_sm_provider_context_t context = BAD_CONTEXT;
    switch(SmGetTaskData()->state)
    {
    /*!< Initial state before state machine is running. */
    case APP_STATE_NULL:
    case APP_STATE_INITIALISING:
    case APP_STATE_DFU_CHECK:
    case APP_STATE_FACTORY_RESET:
    case APP_STATE_TERMINATING:
    case APP_STATE_STARTUP:
    case APP_STATE_HANDSET_PAIRING:
        context = context_app_sm_non_core_state;
        break;

    /*! Earbud is in the case, parent state */
    case APP_STATE_IN_CASE:
    case APP_STATE_IN_CASE_TERMINATING:
    case APP_STATE_IN_CASE_IDLE:
    case APP_STATE_IN_CASE_DFU:
    case APP_STATE_IN_CASE_DISCONNECTING:
        context = context_app_sm_in_case;
        break;

    /*!< Earbud is out of the case, parent state */
    case APP_STATE_OUT_OF_CASE:
    case APP_STATE_OUT_OF_CASE_TERMINATING:
    case APP_STATE_OUT_OF_CASE_SOPORIFIC:
    case APP_STATE_OUT_OF_CASE_SOPORIFIC_TERMINATING:
    case APP_STATE_OUT_OF_CASE_IDLE:
        {
            bool is_mru_handset = TRUE;
            device_t handset_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &is_mru_handset, sizeof(uint8));
            if (HandsetService_Connected(handset_device))
            {
                context = context_app_sm_out_of_case_idle_connected;
            }
            else
            {
                context = context_app_sm_out_of_case_idle;
            }
        }
        break;
    case APP_STATE_OUT_OF_CASE_BUSY:
        context = context_app_sm_out_of_case_busy;
        break;
    case APP_STATE_OUT_OF_CASE_DISCONNECTING:
        context = context_app_sm_out_of_case_idle_connected;
        break;

    /*!< Earbud in in ear, parent state */
    case APP_STATE_IN_EAR:
    case APP_STATE_IN_EAR_TERMINATING:
    case APP_STATE_IN_EAR_IDLE:
    case APP_STATE_IN_EAR_BUSY:
    case APP_STATE_IN_EAR_DISCONNECTING:
        context = context_app_sm_in_ear;
        break;

    default:
        break;
    }
    return (unsigned)context;
}

/*! \brief handles sm module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void appSmHandleUiInput(MessageId  ui_input)
{
    switch (ui_input)
    {
        case ui_input_connect_handset:
            appSmConnectHandset();
            break;
        case ui_input_sm_pair_handset:
            appSmPairHandset();
            break;
        case ui_input_sm_delete_handsets:
            earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_delete_handsets_t));
            appSmDeleteHandsets();
            break;
        case ui_input_factory_reset_request:
            appSmFactoryReset();
            break;
        case ui_input_dfu_active_when_in_case_request:
            appSmEnterDfuModeInCase(TRUE, TRUE);
            break;
#ifdef PRODUCTION_TEST_MODE
        case ui_input_production_test_mode_request:
            appSmEnterProductionTestMode();
            break;
#endif
#ifdef QCC3020_FF_ENTRY_LEVEL_AA
        case ui_input_force_reset:
            Panic();
            break;
#endif
        default:
            break;
    }
}

static void appSmGeneratePhyStateRulesEvents(phy_state_event event)
{
    switch(event)
    {
        case phy_state_event_in_case:
            appSmRulesSetEvent(RULE_EVENT_PEER_IN_CASE);
            break;
        case phy_state_event_out_of_case:
            appSmRulesSetEvent(RULE_EVENT_PEER_OUT_CASE);
            break;
        case phy_state_event_in_ear:
            appSmRulesSetEvent(RULE_EVENT_PEER_IN_EAR);
            break;
        case phy_state_event_out_of_ear:
            appSmRulesSetEvent(RULE_EVENT_PEER_OUT_EAR);
            break;
        default:
            DEBUG_LOG_WARN("appSmGeneratePhyStateRulesEvents unhandled event %u", event);
            break;
    }
}

static void appSmGenerateA2dpRulesEvents(bool connected)
{
    if (!StateProxy_IsPeerHandsetAvrcpConnected() &&
        !StateProxy_IsPeerHandsetHfpConnected())
    {
        appSmRulesSetEvent(connected ? RULE_EVENT_PEER_HANDSET_CONNECTED :
                                         RULE_EVENT_PEER_HANDSET_DISCONNECTED);
    }
}

static void appSmGenerateAvrcpRulesEvents(bool connected)
{
    if (!StateProxy_IsPeerHandsetA2dpConnected() &&
        !StateProxy_IsPeerHandsetHfpConnected())
    {
        appSmRulesSetEvent(connected ? RULE_EVENT_PEER_HANDSET_CONNECTED :
                                         RULE_EVENT_PEER_HANDSET_DISCONNECTED);
    }
}

static void appSmGenerateHfpRulesEvents(bool connected)
{
    if (!StateProxy_IsPeerHandsetA2dpConnected() &&
        !StateProxy_IsPeerHandsetAvrcpConnected())
    {
        appSmRulesSetEvent(connected ? RULE_EVENT_PEER_HANDSET_CONNECTED :
                                         RULE_EVENT_PEER_HANDSET_DISCONNECTED);
    }
}

static void appSmGeneratePairingRulesEvents(PAIRING_ACTIVITY_T* pha)
{
    if (pha->status == pairingInProgress)
    {
        appSmRulesSetEvent(RULE_EVENT_PEER_PAIRING);
    }
    else
    {
        appSmRulesResetEvent(RULE_EVENT_PEER_PAIRING);
    }
}

static void appSmHandleStateProxyEvent(const STATE_PROXY_EVENT_T* sp_event)
{
    DEBUG_LOG_DEBUG("appSmHandleStateProxyEvent source %u type %u", sp_event->source, sp_event->type);
    switch(sp_event->type)
    {
        case state_proxy_event_type_phystate:
            {
                smTaskData* sm = SmGetTaskData();
                /* Get latest physical state */
                sm->phy_state = appPhyStateGetState();
                if (sp_event->source == state_proxy_source_remote)
                {
                    DEBUG_LOG("appSmHandleStateProxyEvent phystate new-state %d event %d", sp_event->event.phystate.new_state, sp_event->event.phystate.event);
                    appSmGeneratePhyStateRulesEvents(sp_event->event.phystate.event);
                }
            }
            break;
        case state_proxy_event_type_a2dp_conn: 
            appSmGenerateA2dpRulesEvents(TRUE);
            break;
        case state_proxy_event_type_a2dp_discon: 
            appSmGenerateA2dpRulesEvents(FALSE);
            break;
        case state_proxy_event_type_avrcp_conn: 
            appSmGenerateAvrcpRulesEvents(TRUE);
            break;
        case state_proxy_event_type_avrcp_discon: 
            appSmGenerateAvrcpRulesEvents(FALSE);
            break;
        case state_proxy_event_type_hfp_conn: 
            appSmGenerateHfpRulesEvents(TRUE);
            break;
        case state_proxy_event_type_hfp_discon: 
            appSmGenerateHfpRulesEvents(FALSE);
            break;
        case state_proxy_event_type_a2dp_supported:
            appSmRulesSetEvent(RULE_EVENT_PEER_A2DP_SUPPORTED);
            break;
        case state_proxy_event_type_avrcp_supported:
            appSmRulesSetEvent(RULE_EVENT_PEER_AVRCP_SUPPORTED);
            break;
        case state_proxy_event_type_hfp_supported:
            appSmRulesSetEvent(RULE_EVENT_PEER_HFP_SUPPORTED);
            break;
        case state_proxy_event_type_handset_linkloss:
            appSmRulesSetEvent(RULE_EVENT_PEER_HANDSET_LINK_LOSS);
            break;
        case state_proxy_event_type_pairing:
            if (sp_event->source == state_proxy_source_remote)
            {
                appSmGeneratePairingRulesEvents(&sp_event->event.handset_activity);
            }
            break;
        default:
            break;
    }
}

/*! \brief Enter ANC tuning mode */
static void appSmHandleConnRulesAncEnterTuning(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncEnterTuning ANC Enter Tuning Mode");

    /* Inject UI input ANC enter tuning */
    Ui_InjectUiInput(ui_input_anc_enter_tuning_mode);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_TUNING_START);
}

/*! \brief Exit ANC tuning mode */
static void appSmHandleConnRulesAncExitTuning(void)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesAncExitTuning ANC Exit Tuning Mode");

    /* Inject UI input ANC exit tuning */
    Ui_InjectUiInput(ui_input_anc_exit_tuning_mode);

    appSmRulesSetRuleComplete(CONN_RULES_ANC_TUNING_STOP);
}

/*! \brief Handle setting remote audio mix */
static void appSmHandleConnRulesSetRemoteAudioMix(CONN_RULES_SET_REMOTE_AUDIO_MIX_T *remote)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesSetRemoteAudioMix stereo_mix=%d", remote->stereo_mix);

    if (appPeerSigIsConnected())
    {
        marshal_type_t type = remote->stereo_mix ?
                                MARSHAL_TYPE(earbud_sm_req_stereo_audio_mix_t) :
                                MARSHAL_TYPE(earbud_sm_req_mono_audio_mix_t);
        earbudSm_CancelAndSendCommandToPeer(type);
    }

    appSmRulesSetRuleComplete(CONN_RULES_SET_REMOTE_AUDIO_MIX);
}

/*! \brief Handle setting local audio mix */
static void appSmHandleConnRulesSetLocalAudioMix(CONN_RULES_SET_LOCAL_AUDIO_MIX_T *local)
{
    DEBUG_LOG_DEBUG("appSmHandleConnRulesSetLocalAudioMix stereo_mix=%d", local->stereo_mix);

    appKymeraSetStereoLeftRightMix(local->stereo_mix);

    appSmRulesSetRuleComplete(CONN_RULES_SET_LOCAL_AUDIO_MIX);
}

/*! \brief Notify peer of MRU handset */
static void appSm_HandleNotifyPeerMruHandset(void)
{
    bdaddr bd_addr;

    if(appDeviceGetHandsetBdAddr(&bd_addr))
    {
        earbud_sm_ind_mru_handset_t* msg = PanicUnlessMalloc(sizeof(earbud_sm_ind_mru_handset_t));

        DEBUG_LOG_DEBUG("appSmHandleNotifyPeerHandsetConnection address %04x,%02x,%06lx", bd_addr.lap, bd_addr.uap, bd_addr.nap);

        msg->bd_addr = bd_addr;

        appPeerSigMarshalledMsgChannelTx(SmGetTask(),
                                         PEER_SIG_MSG_CHANNEL_APPLICATION,
                                         msg,
                                         MARSHAL_TYPE(earbud_sm_ind_mru_handset_t));
    }
    else
    {
        DEBUG_LOG_DEBUG("appSmHandleNotifyPeerHandsetConnection, No Handset registered");
    }
}


/*! \brief Handle PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION message */
static void appSm_HandlePeerFindRolePrepareForRoleSelection(void)
{
    appState state= appSmGetState();

    DEBUG_LOG_DEBUG("appSm_HandlePeerFindRolePrepareForRoleSelection state=%d", state);

    if(state == APP_STATE_HANDSET_PAIRING)
    {
        /* Cancel the pairing */
        smTaskData *sm = SmGetTaskData();
        Pairing_PairStop(&sm->task);
    }
    else
    {  /* Shouldn't be possible as we only register for this event whilst in
           handset pairing state */
        PeerFindRole_PrepareResponse();
    }
}

/*! \brief Handle PAIRING_STOP_CFM message */
static void appSm_HandlePairingStopCfm(void)
{
    DEBUG_LOG_DEBUG("appSm_HandlePairingStopCfm");

    PeerFindRole_PrepareResponse();
}

static void appSm_HandleDeviceTestServiceEnded(void)
{
    appSmSetState(APP_STATE_STARTUP);
}


/*! \brief Application state machine message handler. */
void appSmHandleMessage(Task task, MessageId id, Message message)
{
    smTaskData* sm = (smTaskData*)task;

    if (isMessageUiInput(id))
    {
        appSmHandleUiInput(id);
        return;
    }

    switch (id)
    {
        case APPS_COMMON_INIT_CFM:
            appSmHandleInitConfirm();
            break;

        /* Pairing completion confirmations */
        case PAIRING_PAIR_CFM:
            appSmHandlePairingPairConfirm((PAIRING_PAIR_CFM_T *)message);
            break;

        /* Connection manager status indications */
        case CON_MANAGER_CONNECTION_IND:
            appSmHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

        /* Handset Service status change indications */
        case HANDSET_SERVICE_DISCONNECTED_IND:
            appSmHandleHandsetServiceDisconnectedInd((HANDSET_SERVICE_DISCONNECTED_IND_T *)message);
            break;

        /* AV status change indications */
        case AV_A2DP_CONNECTED_IND:
            appSmHandleAvA2dpConnectedInd((AV_A2DP_CONNECTED_IND_T *)message);
            break;
        case AV_A2DP_DISCONNECTED_IND:
            appSmHandleAvA2dpDisconnectedInd((AV_A2DP_DISCONNECTED_IND_T *)message);
            break;
        case AV_AVRCP_CONNECTED_IND:
            appSmHandleAvAvrcpConnectedInd((AV_AVRCP_CONNECTED_IND_T *)message);
            break;
        case AV_AVRCP_DISCONNECTED_IND:
            appSmHandleAvAvrcpDisconnectedInd((AV_AVRCP_DISCONNECTED_IND_T *)message);
            break;
        case AV_STREAMING_ACTIVE_IND:
            appSmHandleAvStreamingActiveInd();
            break;
        case AV_STREAMING_INACTIVE_IND:
            appSmHandleAvStreamingInactiveInd();
            break;

        /* HFP status change indications */
        case APP_HFP_CONNECTED_IND:
            appSmHandleHfpConnectedInd((APP_HFP_CONNECTED_IND_T *)message);
            break;
        case APP_HFP_DISCONNECTED_IND:
            appSmHandleHfpDisconnectedInd((APP_HFP_DISCONNECTED_IND_T *)message);
            break;
        case APP_HFP_SCO_CONNECTED_IND:
            appSmHandleHfpScoConnectedInd();
            break;
        case APP_HFP_SCO_DISCONNECTED_IND:
            appSmHandleHfpScoDisconnectedInd();
            break;

        /* Physical state changes */
        case PHY_STATE_CHANGED_IND:
            appSmHandlePhyStateChangedInd(sm, (PHY_STATE_CHANGED_IND_T*)message);
            break;

        /* Power indications */
        case APP_POWER_SLEEP_PREPARE_IND:
            appSmHandlePowerSleepPrepareInd();
            break;
        case APP_POWER_SLEEP_CANCELLED_IND:
            appSmHandlePowerSleepCancelledInd();
            break;
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            appSmHandlePowerShutdownPrepareInd();
            break;
        case APP_POWER_SHUTDOWN_CANCELLED_IND:
            appSmHandlePowerShutdownCancelledInd();
            break;

        /* Connection rules */
        case CONN_RULES_PEER_SEND_LINK_KEYS:
            appSmHandleConnRulesForwardLinkKeys();
            break;
        case CONN_RULES_A2DP_TIMEOUT:
            appSmHandleConnRulesA2dpTimeout();
            break;
        case CONN_RULES_A2DP_TIMEOUT_CANCEL:
            appSmCancelOutOfEarTimers();
            break;
        case CONN_RULES_MEDIA_PLAY:
            appSmHandleConnRulesMediaPlay();
            break;
        case CONN_RULES_SCO_TIMEOUT:
            appSmHandleConnRulesScoTimeout();
            break;
        case CONN_RULES_ACCEPT_INCOMING_CALL:
            appSmHandleConnRulesAcceptincomingCall();
            break;
        case CONN_RULES_SCO_TRANSFER_TO_EARBUD:
            appSmHandleConnRulesScoTransferToEarbud();
            break;
        case CONN_RULES_SCO_TRANSFER_TO_HANDSET:
            appSmHandleConnRulesScoTransferToHandset();
            break;
        case CONN_RULES_LED_ENABLE:
            appSmHandleConnRulesLedEnable();
            break;
        case CONN_RULES_LED_DISABLE:
            appSmHandleConnRulesLedDisable();
            break;
        case CONN_RULES_HANDSET_PAIR:
            appSmHandleConnRulesHandsetPair();
            break;
        case CONN_RULES_ENTER_DFU:
            appSmHandleConnRulesEnterDfu();
            break;
        case CONN_RULES_EXIT_DFU:
            appSmHandleConnRulesExitDfu();
            break;
        case CONN_RULES_SELECT_MIC:
            appSmHandleConnRulesSelectMic((CONN_RULES_SELECT_MIC_T*)message);
            break;
        case CONN_RULES_PEER_SCO_CONTROL:
            appSmHandleConnRulesPeerScoControl((CONN_RULES_PEER_SCO_CONTROL_T*)message);
            break;
        case CONN_RULES_ANC_TUNING_START:
            appSmHandleConnRulesAncEnterTuning();
            break;
        case CONN_RULES_ANC_TUNING_STOP:
            appSmHandleConnRulesAncExitTuning();
            break;
        case CONN_RULES_SET_REMOTE_AUDIO_MIX:
            appSmHandleConnRulesSetRemoteAudioMix((CONN_RULES_SET_REMOTE_AUDIO_MIX_T*)message);
            break;
        case CONN_RULES_SET_LOCAL_AUDIO_MIX:
            appSmHandleConnRulesSetLocalAudioMix((CONN_RULES_SET_LOCAL_AUDIO_MIX_T*)message);
            break;

        case CONN_RULES_NOTIFY_PEER_MRU_HANDSET:
            appSm_HandleNotifyPeerMruHandset();
            break;

        case CONN_RULES_DFU_ABORT:
            UpgradeHandleAbortDuringUpgrade();
            break;

        case CONN_RULES_DFU_ALLOW:
            appSmHandleDfuAllow((const CONN_RULES_DFU_ALLOW_T*)message);
            break;

        case SEC_CONN_RULES_ENTER_DFU:
            appSmHandleConnRulesEnterDfu();
            EarbudSm_HandleDfuNotifyPrimary();
            break;

        /* Peer signalling messages */
        case PEER_SIG_CONNECTION_IND:
            appSmHandlePeerSigConnectionInd((const PEER_SIG_CONNECTION_IND_T *)message);
            break;

        /* TWS Topology messages */
        case TWS_TOPOLOGY_START_CFM:
            appSm_HandleTwsTopologyStartCfm((TWS_TOPOLOGY_START_CFM_T*)message);
            break;

        case TWS_TOPOLOGY_ROLE_CHANGED_IND:
            appSm_HandleTwsTopologyRoleChangedInd((TWS_TOPOLOGY_ROLE_CHANGED_IND_T*)message);
            break;

        case TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND:
            DEBUG_LOG_DEBUG("appSmHandleMessage TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND");
            if(appSmIsUserPairing())
            {
                appSm_ContinuePairingHandsetAfterDisconnect();
            }
            break;

#ifdef INCLUDE_DFU

        case APP_GAIA_DISCONNECTED:
            if (dfu_link_state == APP_GAIA_UPGRADE_DISCONNECTED)
            {
                appSmHandleDfuEnded(FALSE);
            }
            else if (dfu_link_state == APP_GAIA_UPGRADE_CONNECTED)
            {
                DEBUG_LOG_DEBUG("appSmHandleMessage Abrupt disconnect.");
            }
            
            break;

        case APP_GAIA_UPGRADE_CONNECTED:
            dfu_link_state = APP_GAIA_UPGRADE_CONNECTED;
            appSmHandleUpgradeConnected();
            break;

        case APP_GAIA_UPGRADE_DISCONNECTED:
            dfu_link_state = APP_GAIA_UPGRADE_DISCONNECTED;
            appSmHandleUpgradeDisconnected();

            /*
             * Reset now as graceful cleanup for the subsequent upgrade attempt 
             */
            if (appUpgradeIsDfuAbortOnHandoverDone())
                appUpgradeSetDfuAbortOnHandoverState(FALSE);
            break;

        /* Messages from UPGRADE */
        case APP_UPGRADE_REQUESTED_TO_CONFIRM:
            appSetUpgradeRebootReason(SM_UPGRADED_DEFINED_REBOOT);
            break;

        case APP_UPGRADE_REQUESTED_IN_PROGRESS:
            appSmEnterDfuOnStartup(TRUE);
            appSetUpgradeRebootReason(SM_UPGRADED_ABRUPT_RESET);
            break;

        case APP_UPGRADE_ACTIVITY:
            /* We do not monitor activity at present.
               Might be a use to detect long upgrade stalls without a disconnect */
            break;

        case APP_UPGRADE_STARTED:
            appSmNotifyUpgradeStarted();
            break;

        case APP_UPGRADE_COMPLETED:
            appSmHandleDfuEnded(FALSE);
            GattServerGatt_SetGattDbChanged();
            break;

        /* Clean up DFU state variables after abort */
        case APP_UPGRADE_CLEANUP_ON_ABORT:
            appSmHandleDfuEnded(TRUE);
            break;

        case DEVICE_UPGRADE_PEER_STARTED:
        {
            /* During ongoing DFU, for initiator device, set the Peer DFU flag 
             * once we reach here. And for peer device, trigger
             * appSmCancelDfuTimers() to extend the DFU timeout.
             */
            if(BtDevice_IsMyAddressPrimary())
            {
                /* Set the Peer DFU flag once the peer DFU started */
                appSmSetPeerDfuFlag(TRUE);
            }
            else
            {
                /* Trigger appSmCancelDfuTimers() to extend DFU timeout */
                appSmNotifyUpgradeStarted();
            }

            /* Inject an event to the rules engine to make sure DFU is enabled */
            appSmRulesSetEvent(RULE_EVENT_CHECK_DFU);
        }
            break;
        case DEVICE_UPGRADE_PEER_DISCONNECT:
            /* Clear the Peer DFU flag once the peer device is disconnected */
            appSmSetPeerDfuFlag(FALSE);
            break;
#endif /* INCLUDE_DFU */

        case STATE_PROXY_EVENT:
            appSmHandleStateProxyEvent((const STATE_PROXY_EVENT_T*)message);
            break;

        /* SM internal messages */
        case SM_INTERNAL_PAIR_HANDSET:
            appSmHandleInternalPairHandset();
            break;
        case SM_INTERNAL_DELETE_HANDSETS:
            appSmHandleInternalDeleteHandsets();
            break;
        case SM_INTERNAL_FACTORY_RESET:
            appSmHandleInternalFactoryReset();
            break;
        case SM_INTERNAL_TIMEOUT_LINK_DISCONNECTION:
            appSmHandleInternalLinkDisconnectionTimeout();
            break;

#ifdef INCLUDE_DFU
        case SM_INTERNAL_ENTER_DFU_UI:
            appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutAfterEnteringCaseMs());
            break;

        case SM_INTERNAL_ENTER_DFU_UPGRADED:
            appSmHandleEnterDfuUpgraded();
            break;

        case SM_INTERNAL_ENTER_DFU_STARTUP:
            appSmHandleEnterDfuStartup();
            break;

        case SM_INTERNAL_TIMEOUT_DFU_ENTRY:
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_ENTRY");
            EarbudSm_HandleDfuStartTimeoutNotifySecondary();
            appSmHandleDfuEnded(TRUE);
            break;

        case SM_INTERNAL_TIMEOUT_DFU_MODE_START:
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_MODE_START");

            appSmEnterDfuModeInCase(FALSE, FALSE);
            break;

        case SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT:
            /* No action needed for this timer */
            DEBUG_LOG_DEBUG("appSmHandleMessage SM_INTERNAL_TIMEOUT_DFU_AWAIT_DISCONNECT");
            break;
#endif

        case SM_INTERNAL_NO_DFU:
            appSmHandleNoDfu();
            break;

        case SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP:
            appSmHandleInternalTimeoutOutOfEarA2dp();
            break;

        case SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START:
            appSmHandleInternalTimeoutInEarA2dpStart();
            break;

        case SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO:
            appSmHandleInternalTimeoutOutOfEarSco();
            break;

        case SM_INTERNAL_TIMEOUT_IDLE:
            appSmHandleInternalTimeoutIdle();
            break;

        case SM_INTERNAL_REBOOT:
            appSmHandleInternalReboot();
            break;

        case SM_INTERNAL_LINK_DISCONNECTION_COMPLETE:
            appSmHandleInternalAllRequestedLinksDisconnected((SM_INTERNAL_LINK_DISCONNECTION_COMPLETE_T *)message);
            break;

        case SM_INTERNAL_BREDR_CONNECTED:
            appSmHandleInternalBredrConnected();
            break;

            /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            appSm_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            appSm_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

        case PAIRING_ACTIVITY:
            appSm_HandlePairingActivity((PAIRING_ACTIVITY_T*)message);
            break;

        case PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION:
            appSm_HandlePeerFindRolePrepareForRoleSelection();
            break;

        case PAIRING_STOP_CFM:
            appSm_HandlePairingStopCfm();
            break;

#ifdef PRODUCTION_TEST_MODE
        case SM_INTERNAL_ENTER_PRODUCTION_TEST_MODE:
            appSmHandleInternalEnterProductionTestMode();
            break;

        case SM_INTERNAL_ENTER_DUT_TEST_MODE:
            appSmHandleInternalEnterDUTTestMode();
            break;
#endif

        case DEVICE_TEST_SERVICE_ENDED:
            appSm_HandleDeviceTestServiceEnded();
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}

bool appSmIsInEar(void)
{
    smTaskData *sm = SmGetTaskData();
    return sm->phy_state == PHY_STATE_IN_EAR;
}

bool appSmIsOutOfEar(void)
{
    smTaskData *sm = SmGetTaskData();
    return (sm->phy_state >= PHY_STATE_IN_CASE) && (sm->phy_state <= PHY_STATE_OUT_OF_EAR_AT_REST);
}

bool appSmIsInCase(void)
{
    smTaskData *sm = SmGetTaskData();
    return (sm->phy_state == PHY_STATE_IN_CASE) || (sm->phy_state == PHY_STATE_UNKNOWN);
}

bool appSmIsOutOfCase(void)
{
    smTaskData *sm = SmGetTaskData();
    DEBUG_LOG_DEBUG("appSmIsOutOfCase Current State %d", sm->phy_state);
    return (sm->phy_state >= PHY_STATE_OUT_OF_EAR) && (sm->phy_state <= PHY_STATE_IN_EAR);
}

bool appSmIsConnectable(void)
{
    return appSmStateIsConnectable(appSmGetState());
}

bool appSmIsInDfuMode(void)
{
    /* Check if EB is in DFU mode. This is used for in-case DFU to know if EB
     * is in DFU mode before entering in-case
     * Variable enter_dfu_mode is set while entering in DFU mode through UI
     * and gets reset when entering in-case DFU, appEnterInCaseDfu().
     * App state gets set to APP_STATE_IN_CASE_DFU after this.
     * So to check the DFU mode, check if either enter_dfu_mode is set or app
     * state is APP_STATE_IN_CASE_DFU.
     * This is not used for out-of-case DFU.
     */
#ifdef INCLUDE_DFU
    return (SmGetTaskData()->enter_dfu_mode
           || APP_STATE_IN_CASE_DFU == appSmGetState());
#else
    return FALSE;
#endif
}

bool appSmIsInCaseDfuPending(void)
{
#ifdef INCLUDE_DFU
    return (   SmGetTaskData()->enter_dfu_in_case
           || APP_STATE_IN_CASE_DFU == appSmGetState());
#else
    return FALSE;
#endif
}

bool appSmIsOutofCaseDfu(void)
{
    /* For DFU, we need to know if it is out of case DFU so that primary/secondary
     * rules are also executed with DFU rules.
     * Primary Secondary rules are run if app state is not in-case DFU and
     * either phy state is out case or there is no in-case DFU pending.
     * No in case DFU pending condition is to make sure that in-case events
     * during out-of-case DFU also trigger these rules.
     */
#ifdef INCLUDE_DFU
    DEBUG_LOG_DEBUG("appSmIsOutofCaseDfu: app_state:0x%x, out-case:%d, InCase DFU Pending:%d",
               appSmGetState(), appPhyStateIsOutOfCase(), appSmIsInCaseDfuPending());
    return ((appSmGetState() != APP_STATE_IN_CASE_DFU) &&
            (appPhyStateIsOutOfCase() || !appSmIsInCaseDfuPending()));
#else
    return FALSE;
#endif
}

bool appSmIsDFUPrimary(void)
{
    /* While in DFU role, we need to know if EB is DFU primary or not.
     * Role can be found out from app state and also from DFU PSKEY.
     */
    bool is_primary_device;
    UpgradePeerGetDFUInfo(&is_primary_device, NULL);
    DEBUG_LOG_DEBUG("appSmIsDFUPrimary: appSmIsPrimary() %d, is_primary_device (read from PSKEY) %d",
               appSmIsPrimary(), is_primary_device);
    return (appSmIsPrimary() || is_primary_device);
}


void appSmPairHandset(void)
{
    smTaskData *sm = SmGetTaskData();
    MessageSend(&sm->task, SM_INTERNAL_PAIR_HANDSET, NULL);
}

void appSmDeleteHandsets(void)
{
    smTaskData *sm = SmGetTaskData();
    MessageSend(&sm->task, SM_INTERNAL_DELETE_HANDSETS, NULL);
}

#ifdef INCLUDE_DFU
void appSmEnterDfuMode(void)
{
    MessageSend(SmGetTask(),SM_INTERNAL_ENTER_DFU_UI, NULL);
}

static void appSmStartDfuTimer(void)
{
    appSmHandleEnterDfuWithTimeout(appConfigDfuTimeoutAfterGaiaConnectionMs());
}

void appSmEnterDfuModeInCase(bool enable, bool inform_peer)
{
    MessageCancelAll(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_MODE_START);
    SmGetTaskData()->enter_dfu_mode = enable;

    if (enable)
    {
        DEBUG_LOG_DEBUG("appSmEnterDfuModeInCase (re)start SM_INTERNAL_TIMEOUT_DFU_MODE_START %dms",
                            appConfigDfuTimeoutToPlaceInCaseMs());
        MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_DFU_MODE_START,
                            NULL, appConfigDfuTimeoutToPlaceInCaseMs());

        /*! We will be entering DFU mode. Make sure that the topology is aware */
        TwsTopology_SwitchToDfuRole();
    }

    if (inform_peer)
        earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_dfu_active_when_in_case_t));
}


/*! \brief Tell the state machine to enter DFU mode following a reboot.
    \param upgrade_reboot If TRUE, indicates that DFU triggered the reboot.
                          If FALSE, indicates the device was rebooted whilst
                          an upgrade was in progress.
*/
static void appSmEnterDfuOnStartup(bool upgrade_reboot)
{
    MessageSend(SmGetTask(),
                upgrade_reboot ? SM_INTERNAL_ENTER_DFU_UPGRADED
                               : SM_INTERNAL_ENTER_DFU_STARTUP,
                NULL);
}


/*! \brief Notification to the state machine of Upgrade start */
static void appSmNotifyUpgradeStarted(void)
{
    /* Set the topology role, device role and DFU mode when upgrade starts
       For in case DFU it is already set using appEnterInCaseDfu.
       For out of case DFU, it is set here.
     */
    if (TwsTopology_GetRole() != tws_topology_role_dfu)
    {
        DEBUG_LOG_DEBUG("appSmNotifyUpgradeStarted - switching to DFU topology");

        /*! We have entered DFU mode. Make sure that the topology catches up */
        TwsTopology_SwitchToDfuRole();
        /*! Set the is_out_case_dfu information in UPGRADE_LIB_PSKEY */
        UpgradeSetIsOutCaseDFU(TRUE);
        /*! Also, set the DFU mode for Secondary device to be in sync.  */
        earbudSm_SendCommandToPeer(MARSHAL_TYPE(earbud_sm_req_dfu_active_when_out_case_t));
    }
    /*
     * ToDo: Set Role can now be deprecated because this is done early on
     * GAIA connection and before the initial DFU Protocol PDU exchange.
     * Refer: appSmHandleUpgradeConnected() for an elaborated reason.
     */
    UpgradePeerSetDeviceRolePrimary(BtDevice_IsMyAddressPrimary());

    UpgradePeerSetDFUMode(TRUE);

    SmGetTaskData()->dfu_in_progress = TRUE;
    appSmCancelDfuTimers();
}

/*! \brief Set the Peer DFU flag during Peer DFU */
static void appSmSetPeerDfuFlag(bool flag)
{
    SmGetTaskData()->peer_dfu_in_progress = flag;
}
#endif /* INCLUDE_DFU */

/*! \brief provides state manager(sm) task to other components

    \param[in]  void

    \return     Task - sm task.
*/
Task SmGetTask(void)
{
  return &app_sm.task;
}

/*! \brief Initialise the main application state machine.
 */
bool appSmInit(Task init_task)
{
    smTaskData* sm = SmGetTaskData();
    start_ps_free_count = 0;

    memset(sm, 0, sizeof(*sm));
    sm->task.handler = appSmHandleMessage;
    sm->state = APP_STATE_NULL;
    sm->phy_state = appPhyStateGetState();

    /* configure rule sets */
    sm->primary_rules = PrimaryRules_GetRuleSet();
    sm->secondary_rules = SecondaryRules_GetRuleSet();
    sm->role = tws_topology_role_none;

    sm->client_tasks = TaskList_Create();
    /* register with connection manager to get notification of (dis)connections */
    ConManagerRegisterConnectionsClient(&sm->task);

    /* register with HFP for changes in state */
    appHfpStatusClientRegister(&sm->task);

    /* register with AV to receive notifications of A2DP and AVRCP activity */
    appAvStatusClientRegister(&sm->task);

    /* register with peer signalling to connect/disconnect messages */
    appPeerSigClientRegister(&sm->task);

    /* register with power to receive sleep/shutdown messages. */
    appPowerClientRegister(&sm->task);

    /* set 'always run' events.
       setting the event isn't required, but it sets the task of the SM
       as the one to send the rule action message to. */
    appSmRulesSetEvent(RULE_EVENT_PAGE_SCAN_UPDATE);

    /* get remote phy state events */
    StateProxy_EventRegisterClient(&sm->task, appConfigStateProxyRegisteredEventsDefault());

    /* TODO: Need to find right place */
    AncStateManager_PostInitSetup();

    /* Register with peer signalling to use the State Proxy msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(SmGetTask(), 
                                               PEER_SIG_MSG_CHANNEL_APPLICATION,
                                               earbud_sm_marshal_type_descriptors,
                                               NUMBER_OF_SM_MARSHAL_OBJECT_TYPES);

    /* Register to get pairing activity reports */
    Pairing_ActivityClientRegister(&sm->task);

    /* Register for role changed indications from TWS Topology */
    TwsTopology_RegisterMessageClient(SmGetTask());

    appSmSetState(APP_STATE_INITIALISING);

    /* Register sm as ui provider*/
    Ui_RegisterUiProvider(ui_provider_phy_state, earbudSm_GetCurrentPhyStateContext);

    Ui_RegisterUiProvider(ui_provider_app_sm, earbudSm_GetCurrentApplicationContext);

    Ui_RegisterUiInputConsumer(SmGetTask(), sm_ui_inputs, ARRAY_DIM(sm_ui_inputs));

    UNUSED(init_task);
    return TRUE;
}

void appSmConnectHandset(void)
{
    bdaddr handset_addr = {0};
    if (appDeviceGetHandsetBdAddr(&handset_addr))
    {
        uint8 profiles_to_connect = BtDevice_GetLastConnectedProfiles(&handset_addr);

        if (!profiles_to_connect)
        {
            device_t handset_device = BtDevice_GetDeviceForBdAddr(&handset_addr);
            PanicNull(handset_device);
            Device_GetPropertyU8(handset_device,device_property_supported_profiles, &profiles_to_connect);
        }

        DEBUG_LOG_DEBUG("appSmConnectHandset lap=%6x profiles=%02x", handset_addr.lap, profiles_to_connect);

        HandsetService_ConnectAddressRequest(SmGetTask(), &handset_addr, profiles_to_connect);
    }
}

/*! \brief Request a factory reset. */
void appSmFactoryReset(void)
{
    MessageSend(SmGetTask(), SM_INTERNAL_FACTORY_RESET, NULL);
}

/*! \brief Reboot the earbud. */
extern void appSmReboot(void)
{
    /* Post internal message to action the reboot, this breaks the call
     * chain and ensures the test API will return and not break. */
    MessageSend(SmGetTask(), SM_INTERNAL_REBOOT, NULL);
}

/*! \brief Start earbud handset handover */
extern void appSmInitiateHandover(void)
{
    appSmRulesSetEvent(RULE_EVENT_HANDOVER_DISCONNECT);
}

/*! \brief Determine if this Earbud is Primary.

    \todo this will move to topology.
*/
bool appSmIsPrimary(void)
{
    return SmGetTaskData()->role == tws_topology_role_primary;
}

static void earbudSm_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == EARBUD_ROLE_MESSAGE_GROUP);
    TaskList_AddTask(SmGetTaskData()->client_tasks, task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(EARBUD_ROLE, earbudSm_RegisterMessageGroup, NULL);
