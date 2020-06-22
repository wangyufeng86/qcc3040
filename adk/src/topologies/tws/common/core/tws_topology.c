/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      TWS Topology component core.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_secondary_ruleset.h"
#include "tws_topology_dfu_rules.h"
#include "tws_topology_client_msgs.h"
#include "tws_topology_role_change_client_notifier.h"
#include "tws_topology_goals.h"
#include "tws_topology_procedure_pair_peer.h"
#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_find_role.h"
#include "tws_topology_config.h"
#include "tws_topology_sdp.h"
#include "tws_topology_marshal_typedef.h"
#include "tws_topology_peer_sig.h"
#include <device_upgrade.h>
#include <av.h>
#include <peer_find_role.h>
#include <peer_pair_le.h>
#include <state_proxy.h>
#include <phy_state.h>
#include <bt_device.h>
#include <bredr_scan_manager.h>
#include <key_sync.h>
#include <mirror_profile.h>
#include <hfp_profile.h>
#include <hdma.h>
#include <handset_service.h>
#include <peer_signalling.h>
#include <telephony_messages.h>
#include <le_advertising_manager.h>

#include <task_list.h>
#include <logging.h>

#include <panic.h>
#include <bdaddr.h>

/*! Instance of the TWS Topology. */
twsTopologyTaskData tws_topology = {0};

void twsTopology_RulesSetEvent(rule_events_t event)
{
    switch (TwsTopologyGetTaskData()->role)
    {
        case tws_topology_role_none:
            /* fall-thru, use primary rules in 'none' role */
        case tws_topology_role_primary:
            TwsTopologyPrimaryRules_SetEvent(event);
            break;

        case tws_topology_role_secondary:
            TwsTopologySecondaryRules_SetEvent(event);
            break;

        case tws_topology_role_dfu:
            /* First run DFU rules */
            DEBUG_LOG("twsTopology_RulesSetEvent: Run DFU rules");
            TwsTopologyDfuRules_SetEvent(event);

            /* Decide if need to run primary/secondary rules, or not */
            if (appUpgradeIsOutofCaseDfu())
            {
                DEBUG_LOG("twsTopology_RulesSetEvent: Run Primary/Secondary rules");
                if (appUpgradeIsDFUPrimary())
                {                    
                    /* Subsequently run Primary rule set */
                    TwsTopologyGetTaskData()->role = tws_topology_role_primary;
                    TwsTopologyPrimaryRules_SetEvent(event);
                    /* Restore DFU role */
                    TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
                }
                else
                {
                    /* Subsequently run Secondary rule set */
                    TwsTopologyGetTaskData()->role = tws_topology_role_secondary;
                    TwsTopologySecondaryRules_SetEvent(event);
                    /* Restore DFU role */
                    TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
                }
            }
            break;

        default:
            break;
    }
}

void twsTopology_RulesResetEvent(rule_events_t event)
{
    switch (TwsTopologyGetTaskData()->role)
    {
        case tws_topology_role_none:
            /* fall-thru, use primary rules in 'none' role */
        case tws_topology_role_primary:
            TwsTopologyPrimaryRules_ResetEvent(event);
            break;

        case tws_topology_role_secondary:
            TwsTopologySecondaryRules_ResetEvent(event);
            break;

        case tws_topology_role_dfu:
            /* First run DFU rules */
            DEBUG_LOG("twsTopology_RulesResetEvent: Run DFU rules");
            TwsTopologyDfuRules_ResetEvent(event);

            /* Decide if need to run primary/secondary rules, or not */
            if (appUpgradeIsOutofCaseDfu())
            {
                DEBUG_LOG("twsTopology_RulesResetEvent: Run Primary/Secondary rules");
                /* First run the DFU rule set */
                TwsTopologyDfuRules_ResetEvent(event);
                if (appUpgradeIsDFUPrimary())
                {                    
                    /* Subsequently run Primary rule set */
                    TwsTopologyGetTaskData()->role = tws_topology_role_primary;
                    TwsTopologyPrimaryRules_ResetEvent(event);
                    /* Restore DFU role */
                    TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
                }
                else
                {
                    /* Subsequently run Secondary rule set */
                    TwsTopologyGetTaskData()->role = tws_topology_role_secondary;
                    TwsTopologySecondaryRules_ResetEvent(event);
                    /* Restore DFU role */
                    TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
                }
            }
            break;

        default:
            break;
    }
}

void twsTopology_RulesMarkComplete(MessageId message)
{
    switch (TwsTopologyGetTaskData()->role)
    {
        case tws_topology_role_none:
            /* fall-thru, use primary rules in 'none' role */
        case tws_topology_role_primary:
            TwsTopologyPrimaryRules_SetRuleComplete(message);
            break;

        case tws_topology_role_secondary:
            TwsTopologySecondaryRules_SetRuleComplete(message);
            break;

        case tws_topology_role_dfu:
            TwsTopologyDfuRules_SetRuleComplete(message);
            break;

        default:
            break;
    }
}

tws_topology_role twsTopology_GetRole(void)
{
    return TwsTopologyGetTaskData()->role;
}

/*! \brief Re evaluate deferred Events after a Set-Role and re-inject them to rules 
    engine if evaluation succeeded
 */
static void twsTopology_ReEvaluateDeferredEvents(rule_events_t event_mask)
{
    phyState current_phy_state = appPhyStateGetState();
    
    /* If the defer occur because of dynamic handover, re-inject physical state */
    if(TwsTopology_IsGoalActive(tws_topology_goal_dynamic_handover) &&
        (event_mask & TWSTOP_RULE_EVENT_IN_CASE)&&(current_phy_state == PHY_STATE_IN_CASE))
        {
            DEBUG_LOG("twsTopology_ReEvaluateDeferredEvents : Set In Case Event");
            twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_IN_CASE);
        }

}

static void twsTopology_EvaluatePhyState(rule_events_t event_mask)
{
    if (   (event_mask & TWSTOP_RULE_EVENT_IN_CASE)
        && (appPhyStateGetState() == PHY_STATE_IN_CASE))
    {
        DEBUG_LOG("twsTopology_EvaluatePhyState setting unhandled IN_CASE event in new rule set");
        twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_OUT_CASE);
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_IN_CASE);
    }
    else if (   (event_mask & TWSTOP_RULE_EVENT_OUT_CASE)
             && (appPhyStateGetState() != PHY_STATE_IN_CASE))
    {
        DEBUG_LOG("twsTopology_EvaluatePhyState setting unhandled OUT_CASE event in new rule set");
        twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_IN_CASE);
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_OUT_CASE);
    }
}

void twsTopology_SetRole(tws_topology_role role)
{
    tws_topology_role current_role = TwsTopologyGetTaskData()->role;
    rule_events_t pri_event_mask = TwsTopologyPrimaryRules_GetEvents();
    rule_events_t sec_event_mask = TwsTopologySecondaryRules_GetEvents();

    DEBUG_LOG("twsTopology_SetRole Current role %u -> New role %u", current_role, role);

    /* inform clients of role change */
    TwsTopology_SendRoleChangedInd(role);

    /* only need to change role if actually changes */
    if (current_role != role)
    {
        TwsTopologyGetTaskData()->role = role;

        /* when going to no role always reset rule engines */
        if (role == tws_topology_role_none)
        {
            TwsTopologyPrimaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            TwsTopologySecondaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            twsTopology_EvaluatePhyState(current_role == tws_topology_role_primary ? pri_event_mask:sec_event_mask);
        }

        if(role == tws_topology_role_secondary)
        {
            appUpgradeStoreMainRoleAsPrimary(FALSE);
            Av_SetupForSecondaryRole();
            HfpProfile_SetRole(FALSE);
            MirrorProfile_SetRole(FALSE);
            TwsTopologyPrimaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            twsTopology_ReEvaluateDeferredEvents(pri_event_mask);            
        }
        else if(role == tws_topology_role_primary)
        {
            appUpgradeStoreMainRoleAsPrimary(TRUE);
            Av_SetupForPrimaryRole();
            HfpProfile_SetRole(TRUE);
            MirrorProfile_SetRole(TRUE);
            TwsTopologySecondaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);        
        }

        if(role == tws_topology_role_dfu)
        {
            DEBUG_LOG("twsTopology_SetRole Set DFU role.");
            DEBUG_LOG("twsTopology_SetRole main_role: %d", current_role);

            /*
             * At DFU role setting its still not finalized whether upgrade
             * shall proceed as out-of-case (back-ground) DFU or in-case
             * (fore-ground).
             *
             * Also both for back-ground or fore-ground DFU, post upgrade
             * reboot phases (i.e. commit phase) is executed as in-case so
             * main role (i.e. Primary or Secondary) specific configuration is
             * not required because all related profiles (like
             * HFP, A2DP and/or AVRP) won't be in-use when in-case. So isolating
             * these concurrent scenarios is recommended and acceptable.
             *
             * Normally DFU role is transitioned into from a main role
             * (i.e. Primary or Secondary) whereas post upgrade reboot phases of
             * DFU, DFU role is directly transitioned into from no role.
             *
             * With the above constraints, the following approach is acceptable:
             *      Load the main role specific configuration and later
             *      disconnect related profiles when transition to in-case both
             *      for fore-groundor back-ground DFU.
             */
            if (current_role == tws_topology_role_primary)
            {
                Av_SetupForPrimaryRole();
                HfpProfile_SetRole(TRUE);
                MirrorProfile_SetRole(TRUE);

                /* Run the Primary rule set */
                TwsTopologyGetTaskData()->role = tws_topology_role_primary;

                TwsTopologySecondaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
                twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SWITCH);

                /* Restore DFU role */
                TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
            }
            else if (current_role == tws_topology_role_secondary)
            {
                Av_SetupForSecondaryRole();
                HfpProfile_SetRole(FALSE);
                MirrorProfile_SetRole(FALSE);

                /* Run the Secondary rule set */
                TwsTopologyGetTaskData()->role = tws_topology_role_secondary;

                TwsTopologyPrimaryRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
                twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SWITCH);
                twsTopology_ReEvaluateDeferredEvents(pri_event_mask);

                /* Restore DFU role */
                TwsTopologyGetTaskData()->role = tws_topology_role_dfu;
            }
            else
            {
                DEBUG_LOG("twsTopology_SetRole, Post reboot or Abrupt Reset DFU phase for BDFU)");
                /* Is it BDFU? */
                if(UpgradeIsOutCaseDFU())
                {
                     /* Set the right role for Av_Setup, Hfp and Mirroring for Pri Device */
                    if(appUpgradeIsDFUPrimary())
                    {
                        DEBUG_LOG("twsTopology_SetRole, Setup Av, Hfp and Mirroring for Primary)");
                        Av_SetupForPrimaryRole();
                        HfpProfile_SetRole(TRUE);
                        MirrorProfile_SetRole(TRUE);
                    }
                    /* Set the right role for Av_Setup, Hfp and Mirroring for Sec Device */
                    else
                    {
                        DEBUG_LOG("twsTopology_SetRole, Setup Av, Hfp and Mirroring for Secondary)");
                        Av_SetupForSecondaryRole();
                        HfpProfile_SetRole(FALSE);
                        MirrorProfile_SetRole(FALSE);
                    }
                }
            }

        }
    }
}

void twsTopology_SetActingInRole(bool acting)
{
    TwsTopologyGetTaskData()->acting_in_role = acting;
}

void twsTopology_CreateHdma(void)
{
    /* Initialize the handover information to none */
    memset(&TwsTopologyGetTaskData()->handover_info,0,sizeof(handover_data_t));
    TwsTopologyGetTaskData()->hdma_created = TRUE;
    Hdma_Init(TwsTopologyGetTask());
}

void twsTopology_DestroyHdma(void)
{
    twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_HANDOVER);
    TwsTopologyGetTaskData()->hdma_created = FALSE;
    Hdma_Destroy();
}

static void twsTopology_StartHdma(void)
{
    /*  Mirror ACL connection established, Invoke Hdma_Init  */
    if(TwsTopology_IsPrimary())
    {
        twsTopology_CreateHdma();
    }
}

static void twsTopology_StopHdma(void)
{
    /*  Mirror ACL connection disconnected, Invoke HDMA_Destroy  */
    twsTopology_DestroyHdma();
}

/*! \brief Handle failure to find a role due to not having a paired peer Earbud.
 */
static void twsTopology_HandlePeerFindRoleNoPeer(void)
{
    DEBUG_LOG("twsTopology_HandlePeerFindRoleNoPeer");
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_NO_PEER);
}

static void twsTopology_HandlePeerFindRoleTimeout(void)
{
    if (TwsTopologyGetTaskData()->start_cfm_needed)
    {
        TwsTopology_SendStartCfm(tws_topology_status_success, tws_topology_role_primary);
    }
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SELECTED_ACTING_PRIMARY);
}

static void twsTopology_HandlePeerFindRolePrimary(void)
{
    if (TwsTopologyGetTaskData()->start_cfm_needed)
    {
        TwsTopology_SendStartCfm(tws_topology_status_success, tws_topology_role_primary);
    }
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SELECTED_PRIMARY);
}

static void twsTopology_HandlePeerFindRoleSecondary(void)
{
    DEBUG_LOG("twsTopology_HandlePeerFindRoleSecondary");

    if (TwsTopologyGetTaskData()->start_cfm_needed)
    {
        TwsTopology_SendStartCfm(tws_topology_status_success, tws_topology_role_secondary);
    }
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SELECTED_SECONDARY);
}

static void twsTopology_Start(void)
{
    bdaddr bd_addr_secondary;

    if (appDeviceGetSecondaryBdAddr(&bd_addr_secondary))
    {
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_PEER_PAIRED);
    }
    else
    {
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_NO_PEER);
    }
}

static void twsTopology_HandleInternalStart(void)
{
    DEBUG_LOG("twsTopology_HandleInternalStart");
    twsTopology_Start();
}


static void twsTopology_HandleClearHandoverPlay(void)
{
    DEBUG_LOG("twsTopology_HandleClearHandoverPlay");

    appAvPlayOnHandsetConnection(FALSE);
}


static void twsTopology_HandleProcPeerPairResult(PROC_PAIR_PEER_RESULT_T* pppr)
{
    if (pppr->success == TRUE)
    {
        DEBUG_LOG("twsTopology_HandleProcPeerPairResult PEER PAIR SUCCESS");
    }
    else
    {
        DEBUG_LOG("twsTopology_HandleProcPeerPairResult PEER PAIR FAILED");
    }
    twsTopology_Start();
}

/* \brief Generate physical state related events into rules engine. */
static void twsTopology_HandlePhyStateChangedInd(PHY_STATE_CHANGED_IND_T* ind)
{
    DEBUG_LOG("twsTopology_HandlePhyStateChangedInd ev %u", ind->event);

    switch (ind->event)
    {
        case phy_state_event_out_of_case:
            /* Reset the In case rule event set out of case rule event */
            twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_IN_CASE);
            twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_OUT_CASE);
            break;
        case phy_state_event_in_case:
            /* Reset the out of case rule event set in case rule event */
            twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_OUT_CASE);
            twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_IN_CASE);
            break;
        default:
        break;
    }
}

/* \brief Update the handover data to record  HDMA notification message in topology */
static void twsTopology_UpdateHandoverInfo(hdma_handover_decision_t* message)
{
    /* Store the HDMA recommendation in topology */
    TwsTopologyGetTaskData()->handover_info.hdma_message.timestamp = message->timestamp;
    TwsTopologyGetTaskData()->handover_info.hdma_message.reason = message->reason;
    TwsTopologyGetTaskData()->handover_info.hdma_message.urgency = message->urgency;
}

/* \brief Trigger a handover event to the rules engine */
static void twsTopology_TriggerHandoverEvent(void)
{
    switch (TwsTopologyGetTaskData()->handover_info.hdma_message.reason)
    {
        case HDMA_HANDOVER_REASON_IN_CASE:
#ifdef INCLUDE_MIRRORING
        case HDMA_HANDOVER_REASON_OUT_OF_EAR:
        case HDMA_HANDOVER_REASON_BATTERY_LEVEL:
        case HDMA_HANDOVER_REASON_VOICE_QUALITY:
        case HDMA_HANDOVER_REASON_EXTERNAL:
		case HDMA_HANDOVER_REASON_SIGNAL_QUALITY:
#endif
        {
        	DEBUG_LOG("Reason: %d\n", TwsTopologyGetTaskData()->handover_info.hdma_message.reason);
            twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_HANDOVER);
        }   
        break;
        default:
        break;
    }
}

/* \brief Handle HDMA notifications */
static void twsTopology_HandleHDMARequest(hdma_handover_decision_t* message)
{
    DEBUG_LOG("twsTopology_HandleHDMARequest");

    /* Store the HDMA recommendation message in topology */
    twsTopology_UpdateHandoverInfo(message);

    /* Check and trigger a handover event to the rules engine */
    twsTopology_TriggerHandoverEvent();   
}

/* \brief Handle HDMA cancel notification */
static void twsTopology_HandleHDMACancelHandover(void)
{
    DEBUG_LOG("twsTopology_HandleHDMACancelHandover");

    twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_HANDOVER);

    if(TwsTopologyGetTaskData()->app_prohibit_handover)
    {
        /* Initialize the handover information to none */
        memset(&TwsTopologyGetTaskData()->handover_info,0,sizeof(handover_data_t));
    }
}

/*! \brief Generate handset related Connection events into rule engine. */
static void twsTopology_HandleHandsetServiceConnectedInd(const HANDSET_SERVICE_CONNECTED_IND_T* ind)
{

    DEBUG_LOG("twsTopology_HandleHandsetConnectedInd %04x,%02x,%06lx", ind->addr.nap,
                                                                       ind->addr.uap,
                                                                       ind->addr.lap);
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_HANDSET_CONNECTED_BREDR);
}

/*! \brief Generate handset related disconnection events into rule engine. */
static void twsTopology_HandleHandsetServiceDisconnectedInd(const HANDSET_SERVICE_DISCONNECTED_IND_T* ind)
{
    DEBUG_LOG("twsTopology_HandleHandsetDisconnectedInd %04x,%02x,%06lx status %u", ind->addr.nap,
                                                                                    ind->addr.uap,
                                                                                    ind->addr.lap,
                                                                                    ind->status);

    if (ind->status == handset_service_status_link_loss)
    {
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_HANDSET_LINKLOSS);
    }
    else
    {
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_BREDR);
    }
}

/*! \brief Start or stop HDMA depending on Earbud state.
 
    HDMA is enabled on if the Earbud has connection to handset and peer Earbud and
    State Proxy has received initial state from peer to be synchronised.
*/
static void twsTopology_CheckHdmaRequired(void)
{
    bool handset_connected = appDeviceIsHandsetConnected();
    bool peer_connected = appDeviceIsPeerConnected();
    bool state_proxy_rx = StateProxy_InitialStateReceived();

    DEBUG_LOG("twsTopology_CheckHdmaRequired handset %u peer %u stateproxy %u",
                                handset_connected, peer_connected, state_proxy_rx);

    if (handset_connected && peer_connected && state_proxy_rx)
    {
        if (!TwsTopologyGetTaskData()->hdma_created)
        {

            DEBUG_LOG("twsTopology_CheckHdmaRequired start HDMA");
            twsTopology_StartHdma();
        }
    }
    else
    {
        if (TwsTopologyGetTaskData()->hdma_created)
        {
            DEBUG_LOG("twsTopology_CheckHdmaRequired stop HDMA");
            twsTopology_StopHdma();
        }
    }
}

/*! \brief Generate connection related events into rule engine. */
static void twsTopology_HandleConManagerConnectionInd(const CON_MANAGER_CONNECTION_IND_T* ind)
{
    DEBUG_LOG("twsTopology_HandleConManagerConnectionInd Conn:%u BLE:%u %04x,%02x,%06lx", ind->connected,
                                                                                          ind->ble,
                                                                                          ind->bd_addr.nap,
                                                                                          ind->bd_addr.uap,
                                                                                          ind->bd_addr.lap);
    if(!ind->ble)
    {
        /* start or stop HDMA as BREDR links have changed. */
        twsTopology_CheckHdmaRequired();

        if (appDeviceIsPeer(&ind->bd_addr))
        {   /* generate peer BREDR connection events into rules engines */
            if (ind->connected)
            {
                DEBUG_LOG("twsTopology_HandleConManagerConnectionInd PEER BREDR Connected");
                twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_PEER_CONNECTED_BREDR);
            }
            else
            {
                if (ind->reason == hci_error_conn_timeout)
                {
                    DEBUG_LOG("twsTopology_HandleConManagerConnectionInd PEER BREDR LINKLOSS");
                    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_PEER_LINKLOSS);
                }
                else
                {
                    DEBUG_LOG("twsTopology_HandleConManagerConnectionInd PEER BREDR DISCONNECTED");
                    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_PEER_DISCONNECTED_BREDR);
                }
            }
        }
        else if (appDeviceIsHandset(&ind->bd_addr))
        {
            if (ind->connected)
            {
                DEBUG_LOG("twsTopology_HandleConManagerConnectionInd Handset ACL Connected");
                twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_HANDSET_ACL_CONNECTED);
            }
        }
    }
    else
    {
        DEBUG_LOG("twsTopology_HandleConManagerConnectionInd not interested in BLE events atm");
    }
}

static void twsTopology_HandleMirrorProfileConnectedInd(void)
{
    /* this message indicates the mirroring ACL is setup, this
     * may have occurred after HDMA has issued a handover decision
     * that was deferred by the primary rules (due to mirroring
     * not being setup yet), so need to kick the rules to reevaluate
     * if a handover needs to be started */
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_KICK);
}

static void twsTopology_HandlePowerSleepPrepareInd(void)
{
    DEBUG_LOG("twsTopology_HandlePowerSleepPrepareInd");
    /* nothing to prepare, responsd immediately */
    appPowerSleepPrepareResponse(TwsTopologyGetTask());
}

static void twsTopology_HandlePowerSleepCancelledInd(void)
{
    DEBUG_LOG("twsTopology_HandlePowerSleepCancelledInd");
}

static void twsTopology_HandlePowerShutdownPrepareInd(void)
{
    DEBUG_LOG("twsTopology_HandlePowerShutdownPrepareInd");

    TwsTopology_SecondaryStaticHandoverCommand();

    /* todo shutdown role is transitional - it is used to prevent subsequent rule execution
       till we Power Off (as this causes issues with PFR and other normal system operation). */
    twsTopology_SetRole(tws_topology_role_shutdown);
}

static void twsTopology_HandlePowerShutdownCancelledInd(void)
{
    DEBUG_LOG("twsTopology_HandlePowerShutdownCancelledInd");

    /* todo Leave the shutdown role and run the find role procedure -
     * this will be achieved by gating rule execution of new events, a
     * different mechanism which is currently wip. This function will
     * remove the gate and run the no_role_find_role procedure
    twsTopology_SetRole(tws_topology_role_none);

    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SWITCH);
    */
}

static void twsTopology_HandleStateProxyInitialStateReceived(void)
{
    DEBUG_LOG("twsTopology_HandleStateProxyInitialStateReceived");
    twsTopology_CheckHdmaRequired();
}


/*! \brief TWS Topology message handler.
 */
static void twsTopology_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    if (!twsTopology_IsRunning())
    {
        DEBUG_LOG("twsTopology_HandleMessage. Not yet started. Message %d(0x%x)",id,id);
        switch (id)
        {
            case CL_SDP_REGISTER_CFM:
                TwsTopology_HandleClSdpRegisterCfm(TwsTopologyGetTask(), (CL_SDP_REGISTER_CFM_T *)message);
                break;

            default:
                DEBUG_LOG("twsTopology_HandleMessage. Not yet started. Message %d(0x%x)",id,id);
                break;
        }
        return;
    }

    /* handle all other messages */
    switch (id)
    {
            /* ROLE SELECT SERVICE */
        case PEER_FIND_ROLE_NO_PEER:
            DEBUG_LOG("twsTopology_HandleMessage: PEER_FIND_ROLE_NO_PEER");
            twsTopology_HandlePeerFindRoleNoPeer();
            break;
        case PEER_FIND_ROLE_ACTING_PRIMARY:
            DEBUG_LOG("twsTopology_HandleMessage: PEER_FIND_ROLE_ACTING_PRIMARY");
            twsTopology_HandlePeerFindRoleTimeout();
            break;
        case PEER_FIND_ROLE_PRIMARY:
            DEBUG_LOG("twsTopology_HandleMessage: PEER_FIND_ROLE_PRIMARY");
            twsTopology_HandlePeerFindRolePrimary();
            break;
        case PEER_FIND_ROLE_SECONDARY:
            DEBUG_LOG("twsTopology_HandleMessage: PEER_FIND_ROLE_SECONDARY");
            twsTopology_HandlePeerFindRoleSecondary();
            break;
        case PEER_FIND_ROLE_CANCELLED:
            /* no action required */
            DEBUG_LOG("twsTopology_HandleMessage: PEER_FIND_ROLE_CANCELLED");
            break;

        case PROC_PAIR_PEER_RESULT:
            twsTopology_HandleProcPeerPairResult((PROC_PAIR_PEER_RESULT_T*)message);
            break;

            /* STATE PROXY MESSAGES */
        case STATE_PROXY_EVENT_INITIAL_STATE_RECEIVED:
            twsTopology_HandleStateProxyInitialStateReceived();
            break;

        /* Mirror Profile messages */
        case MIRROR_PROFILE_CONNECT_IND:
            twsTopology_HandleMirrorProfileConnectedInd();
            break;

        case MIRROR_PROFILE_DISCONNECT_IND:
            /* No action required */
            break;

        /* PHY STATE MESSAGES */
        case PHY_STATE_CHANGED_IND:
            twsTopology_HandlePhyStateChangedInd((PHY_STATE_CHANGED_IND_T*)message);
            break;

        case CON_MANAGER_CONNECTION_IND:
            twsTopology_HandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

            /* Peer Signalling */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            TwsTopology_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            TwsTopology_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

            /* Power indications */
        case APP_POWER_SLEEP_PREPARE_IND:
            twsTopology_HandlePowerSleepPrepareInd();
            break;

        case APP_POWER_SLEEP_CANCELLED_IND:
            twsTopology_HandlePowerSleepCancelledInd();
            break;

        case APP_POWER_SHUTDOWN_PREPARE_IND:
            twsTopology_HandlePowerShutdownPrepareInd();
            break;

        case APP_POWER_SHUTDOWN_CANCELLED_IND:
            twsTopology_HandlePowerShutdownCancelledInd();
            break;

        case TELEPHONY_AUDIO_DISCONNECTED:
            /* fall-through */
        case TELEPHONY_CALL_ENDED:
            /* kick rules to evaluate any deferred events again, we may have
             * deferred handover due to the on-going call now ended */
            twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_KICK);
            break;

            /* INTERNAL MESSAGES */
        case TWSTOP_INTERNAL_START:
            twsTopology_HandleInternalStart();
            break;

        case TWSTOP_INTERNAL_CLEAR_HANDOVER_PLAY:
            twsTopology_HandleClearHandoverPlay();
            break;

        case CL_SDP_UNREGISTER_CFM:
            TwsTopology_HandleClSdpUnregisterCfm(TwsTopologyGetTask(), (CL_SDP_UNREGISTER_CFM_T *)message);
            break;
        
        case HDMA_HANDOVER_NOTIFICATION:
            twsTopology_HandleHDMARequest((hdma_handover_decision_t*)message);
            break;

        case HDMA_CANCEL_HANDOVER_NOTIFICATION:
            twsTopology_HandleHDMACancelHandover();
            break;

        case HANDSET_SERVICE_CONNECTED_IND:
            twsTopology_HandleHandsetServiceConnectedInd((HANDSET_SERVICE_CONNECTED_IND_T*)message);
            break;

        case HANDSET_SERVICE_DISCONNECTED_IND:
            twsTopology_HandleHandsetServiceDisconnectedInd((HANDSET_SERVICE_DISCONNECTED_IND_T*)message);
            break;

        default:
            DEBUG_LOG("twsTopology_HandleMessage. Unhandled message %d(0x%x)",id,id);
            break;
    }
}

static void twsTopologyHandlePairingSuccessMessage(void)
{
    KeySync_Sync();

    /* just completed pairing, check if we need to start HDMA, necessary
     * because the normal checks to start HDMA performed on CON_MANAGER_CONNECTION_INDs
     * will not succeed immediately after pairing because the link type
     * would not have been known to be a handset */
    twsTopology_CheckHdmaRequired();
}

static void twsTopology_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    DEBUG_LOG("twsTopology_HandlePairingActivity status=%d", message->status);

    switch(message->status)
    {
        case pairingSuccess:
            twsTopologyHandlePairingSuccessMessage();
            break;

        default:
            break;
    }
}

static void twsTopology_HandlePairingActivityNotification(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case PAIRING_ACTIVITY:
            DEBUG_LOG("TwsTopology PAIRING_ACTIVITY");
            twsTopology_HandlePairingActivity(message);
            break;
            
        default:
            break;
    }
}

static void twsTopology_RegisterForStateProxyEvents(void)
{
    if (TwsTopologyConfig_StateProxyRegisterEvents())
    {
        StateProxy_EventRegisterClient(TwsTopologyGetTask(), TwsTopologyConfig_StateProxyRegisterEvents());
    }
}

static void twsTopology_RegisterBtParameters(void)
{
    DEBUG_LOG("twsTopology_RegisterBtParameters");

    BredrScanManager_PageScanParametersRegister(&page_scan_params);
    BredrScanManager_InquiryScanParametersRegister(&inquiry_scan_params);
    PanicFalse(LeAdvertisingManager_ParametersRegister(&le_adv_params));
}

static void twsTopology_SelectBtParameters(void)
{
    DEBUG_LOG("twsTopology_SelectBtParameters");

    PanicFalse(LeAdvertisingManager_ParametersSelect(0));
}

bool TwsTopology_Init(Task init_task)
{
    twsTopologyTaskData *tws_taskdata = TwsTopologyGetTaskData();

    UNUSED(init_task);

    tws_taskdata->task.handler = twsTopology_HandleMessage;
    tws_taskdata->goal_task.handler = TwsTopology_HandleGoalDecision;

    tws_taskdata->role = tws_topology_role_none;

    /* Set hdma_created to FALSE */
    tws_taskdata->hdma_created = FALSE;

    /* Handover is allowed by default, app may prohibit handover by calling
    TwsTopology_ProhibitHandover() function with TRUE parameter */
    tws_taskdata->app_prohibit_handover = FALSE;

    tws_taskdata->prohibit_connect_to_handset = FALSE;

    TwsTopologyPrimaryRules_Init(TwsTopologyGetGoalTask());
    TwsTopologySecondaryRules_Init(TwsTopologyGetGoalTask());
    TwsTopologyDfuRules_Init(TwsTopologyGetGoalTask());

    TwsTopology_GoalsInit();

    PeerFindRole_RegisterTask(TwsTopologyGetTask());
    StateProxy_StateProxyEventRegisterClient(TwsTopologyGetTask());

    /* Register to enable interested state proxy events */
    twsTopology_RegisterForStateProxyEvents();

    /* Register for connect / disconnect events from mirror profile */
    MirrorProfile_ClientRegister(TwsTopologyGetTask());

    /* Register for telephony events to have better control of handover during calls */
    Telephony_RegisterForMessages(TwsTopologyGetTask());

    /* Register with power to receive sleep/shutdown messages and 
     * indicate this client does allow sleep. */
    appPowerClientRegister(TwsTopologyGetTask());
    appPowerClientAllowSleep(TwsTopologyGetTask());

    /* register with handset service as we need disconnect and connect notification */
    HandsetService_ClientRegister(TwsTopologyGetTask());

    appPhyStateRegisterClient(TwsTopologyGetTask());
    ConManagerRegisterConnectionsClient(TwsTopologyGetTask());

    twsTopology_RegisterBtParameters();
    twsTopology_SelectBtParameters();

    /* Register to use marshalled message channel with topology on peer Earbud. */
    appPeerSigMarshalledMsgChannelTaskRegister(TwsTopologyGetTask(),
                                               PEER_SIG_MSG_CHANNEL_TOPOLOGY,
                                               tws_topology_marshal_type_descriptors,
                                               NUMBER_OF_TWS_TOPOLOGY_MARSHAL_TYPES);
    /* Setup tasklist for multiple users (internal to tws topology) to
     * make use of the topology peer signalling channel */
    TaskList_InitialiseWithCapacity((task_list_flexible_t*)&tws_taskdata->peer_sig_msg_client_list, 2);

    TwsTopology_SetState(TWS_TOPOLOGY_STATE_SETTING_SDP);


    TaskList_InitialiseWithCapacity(TwsTopologyGetMessageClientTasks(), MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY);

    
    unsigned rc_registrations_array_dim;
    rc_registrations_array_dim = (unsigned)role_change_client_registrations_end -
                              (unsigned)role_change_client_registrations_begin;
    PanicFalse((rc_registrations_array_dim % sizeof(role_change_client_callback_t)) == 0);
    rc_registrations_array_dim /= sizeof(role_change_client_callback_t);

    TwsTopology_RoleChangeClientNotifierInit(role_change_client_registrations_begin, 
                                    rc_registrations_array_dim); 

    return TRUE;
}

void TwsTopology_Start(Task requesting_task)
{
    twsTopologyTaskData *twst = TwsTopologyGetTaskData();

    twst->start_cfm_needed = TRUE;

    if (!twst->started)
    {
        DEBUG_LOG("TwsTopology_Start (normal start)");
        twst->started = TRUE;

        twst->app_task = requesting_task;

        twst->pairing_notification_task.handler = twsTopology_HandlePairingActivityNotification;

        Pairing_ActivityClientRegister(&twst->pairing_notification_task);

        MessageSend(TwsTopologyGetTask(), TWSTOP_INTERNAL_START, NULL);
    }
    else if (   twst->role == tws_topology_role_primary
             || twst->role == tws_topology_role_secondary)
    {
        DEBUG_LOG("TwsTopology_Start, sending immediate success");
        TwsTopology_SendStartCfm(tws_topology_status_success, twst->role);
    }
    else
    {
        DEBUG_LOG("TwsTopology_Start called again. Response waiting for a role");
    }
}

void TwsTopology_RegisterMessageClient(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), client_task);
}

void TwsTopology_UnRegisterMessageClient(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(TwsTopologyGetMessageClientTasks()), client_task);
}

tws_topology_role TwsTopology_GetRole(void)
{
    return TwsTopologyGetTaskData()->role;
}


void TwsTopology_SwitchToDfuRole(void)
{
    DEBUG_LOG("TwsTopology_SwitchToDfuRole. Reset & set event TWSTOP_RULE_EVENT_DFU_ROLE");

    twsTopology_RulesResetEvent(TWSTOP_RULE_EVENT_DFU_ROLE);
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_DFU_ROLE);
}


void TwsTopology_EndDfuRole(void)
{
    DEBUG_LOG("TwsTopology_EndDfuRole. Setting event TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE");

    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE);

    TwsTopologyPrimaryRules_ResetEvent(TWSTOP_RULE_EVENT_DFU_ROLE);
}


void TwsTopology_SelectPrimaryAddress(bool primary)
{
    /*! \todo Consider whether should handle "now in DFU role" message
            and call this. That depends on <something> remembering/
            knowing that we are already in an upgrade */
    TwsTopologyDfuRules_SetEvent(primary ? TWSTOP_RULE_EVENT_ROLE_SELECTED_PRIMARY
                                         : TWSTOP_RULE_EVENT_ROLE_SELECTED_SECONDARY);
}

void TwsTopology_SetProfilesOnPeerConnect(void)
{
    DEBUG_LOG("TwsTopology_SetProfilesOnPeerConnect. Setting event TWSTOP_RULE_EVENT_DFU_PEER_CONNECTED");
    TwsTopologyDfuRules_SetEvent(TWSTOP_RULE_EVENT_DFU_PEER_CONNECTED);
}

bool TwsTopology_IsPrimary(void)
{
    if (TwsTopology_GetRole() == tws_topology_role_dfu)
    {
        DEBUG_LOG("TwsTopology_IsPrimary called in DFU mode");
    }

    return (TwsTopology_GetRole() == tws_topology_role_primary);
}

bool TwsTopology_IsFullPrimary(void)
{
    return (   (TwsTopology_GetRole() == tws_topology_role_primary)
            && !TwsTopologyGetTaskData()->acting_in_role);
}

bool TwsTopology_IsSecondary(void)
{
    return (TwsTopology_GetRole() == tws_topology_role_secondary);
}

bool TwsTopology_IsActingPrimary(void)
{
    return ((TwsTopology_GetRole() == tws_topology_role_primary)
            && (TwsTopologyGetTaskData()->acting_in_role));
}

void TwsTopology_ProhibitHandover(bool prohibit)
{
    TwsTopologyGetTaskData()->app_prohibit_handover = prohibit;

    if(!prohibit)
    {
        twsTopology_TriggerHandoverEvent();
    }
}

void TwsTopology_ProhibitHandsetConnection(bool prohibit)
{
    TwsTopologyGetTaskData()->prohibit_connect_to_handset = prohibit;

    if(prohibit)
    {
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_PROHIBIT_CONNECT_TO_HANDSET);
    }
}

bool twsTopology_IsRunning(void)
{
    twsTopologyTaskData *twst = TwsTopologyGetTaskData();

    return twst->app_task && twst->started;
}