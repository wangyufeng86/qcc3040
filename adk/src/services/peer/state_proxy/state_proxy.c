/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy.c
\brief      Implementation of component providing local and remote state.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_client_msgs.h"
/* local include for state/events monitored by state proxy */
#include "state_proxy_phy_state.h"
#include "state_proxy_battery.h"
#include "state_proxy_flags.h"
#include "state_proxy_pairing.h"
#include "state_proxy_connection.h"
#include "state_proxy_link_quality.h"
#include "state_proxy_mic_quality.h"
#include "state_proxy_anc.h"
#include "state_proxy_leakthrough.h"

/* framework includes */
#include <panic.h>
#include <connection_manager.h>
#include <charger_monitor.h>
#include <battery_monitor.h>
#include "logical_input_switch.h"
#include <phy_state.h>
#include <peer_signalling.h>
#include <hfp_profile.h>
#include <av.h>
#include <pairing.h>
#include <mirror_profile.h>

/* library includes */
#include <logging.h>
#include <task_list.h>

/* system includes */
#include <message.h>
#include <panic.h>
#include <stdlib.h>

/*! Instance of the state proxy. */
state_proxy_task_data_t state_proxy;

/*****************************
 * Utils
 * \todo move to another file
 *****************************/
/*! \brief Calculate total memory size of a data set. */
static size_t stateProxy_GetStateProxyDataSize(state_proxy_source source)
{
    UNUSED(source);
    return sizeof(state_proxy_data_t);
}

/*! \brief Make updates to local and remote active handset addr on profile conn/discon events. */
static void stateProxy_UpdateActiveHandsetAddr(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    bdaddr active_handset_addr;
    bdaddr peer_addr;

    /* initialise active address to zero and override with actual address
     * if we have any active profiles to the handset */
    BdaddrSetZero(&active_handset_addr);
    if (appDeviceIsHandsetAnyProfileConnected())
    {
        appDeviceGetHandsetBdAddr(&active_handset_addr);
    }

    /* save active handset addr in local state, will clear to zero if no profiles
     * connected */
    proxy->local_state->handset_addr = active_handset_addr;

    /* if we're secondary send updated handset addr to peer */
    if (!stateProxy_Paused() &&
        /*stateProxy_IsSecondary() && */
        appDeviceGetPeerBdAddr(&peer_addr))
    {
        state_proxy_active_handset_addr_t* msg = PanicUnlessMalloc(sizeof(state_proxy_active_handset_addr_t));
        msg->active_handset_addr = active_handset_addr;

        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                msg, MARSHAL_TYPE_state_proxy_active_handset_addr_t);
    }
}

/*! \brief Collate the latest of all the various states monitored by state proxy. */
static void stateProxy_GetInitialState(void)
{
    DEBUG_LOG("stateProxy_GetInitialState");

    /* call each state proxy sub-module that implements a type of state. */
    stateProxy_GetInitialPhyState();
    stateProxy_GetInitialBatteryState();
    stateProxy_GetInitialFlags();
    stateProxy_GetInitialPairingState();
    stateProxy_GetInitialConnectionState();
    stateProxy_GetInitialAncData();
    stateProxy_GetInitialLeakthroughData();
}

/****************************
 * Handlers for local events
 ****************************/
/*! \brief Send our initial state to peer when peer signalling is connected. */
static void stateProxy_HandlePeerSigConnectionInd(const PEER_SIG_CONNECTION_IND_T* ind)
{
    DEBUG_LOG("stateProxy_HandlePeerSigConnectionInd status %u", ind->status);

    switch (ind->status)
    {
        case peerSigStatusConnected:
        {
            /* peer signalling is up, send our initial state to sync with
             * peer */
            StateProxy_SendInitialState();
        }
        break;

        case peerSigStatusDisconnected:
            /* fall-through */
        case peerSigStatusLinkLoss:
        {
            /* reset remote device flgas to initial state*/
            stateProxy_SetRemoteInitialFlags();
            
            /* reset initial state sent, so we'll send it again on reconnect
             * and not try and send any event updates until then */
            stateProxy_GetTaskData()->initial_state_sent = FALSE;
            stateProxy_GetTaskData()->initial_state_received = FALSE;
        }
        break;

        default:
        DEBUG_LOG("stateProxy_HandlePeerSigConnectionInd unhandled status %u", ind->status);
        break;
    }
}

static void stateProxy_FlagChangesInState(state_proxy_data_t * new_state, state_proxy_data_t * previous_state)
{
    if(new_state->flags.in_ear != previous_state->flags.in_ear)
    {
        stateProxy_SendRemotePhyStateChangedInd(PHY_STATE_IN_EAR, phy_state_event_in_ear);
    }
}

/********************************************************
 * Handlers for state proxy internal marshalled messages
 ********************************************************/
/*! \brief Handle marshalled message containing initial state of peer.
*/
static void stateProxy_HandleInitialState(const state_proxy_initial_state_t* initial_state)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    DEBUG_LOG("stateProxy_HandleInitialState");

    state_proxy_data_t previous_state = *proxy->remote_state;

    *proxy->remote_state = initial_state->state;

    stateProxy_FlagChangesInState(proxy->remote_state, &previous_state);
    stateProxy_HandleInitialPeerAncData(proxy->remote_state);
    stateProxy_HandleInitialPeerLeakthroughData(proxy->remote_state);

    /* notify clients that initial state has been received from peer, so
     * state proxy is synchronised with peer state */
    proxy->initial_state_received = TRUE;
    stateProxy_MsgStateProxyEventInitialStateReceived();
}

/*! \brief Handle version message transmitted by state proxy on peer.

    \todo Generate hash of the marshal type descriptors and send in the version
          Compare received hash with ours
          Handle version clash failure

    \param
*/
static void stateProxy_HandleRemoteVersion(const state_proxy_version_t* version)
{
    DEBUG_LOG("stateProxy_HandleRemoteVersion state_proxy_version_t version %u", version->version);
}

/*******************************************
 * Transmit state proxy marshalled messages
 *******************************************/

/***********************************
 * Marshalled Message TX CFM and RX
 **********************************/

/*! \brief Handle confirmation of transmission of a marshalled message. */
static void stateProxy_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    SP_LOG_VERBOSE("stateProxy_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);
}

/*! \brief Handle incoming marshalled messages.
*/
static void stateProxy_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        /* State Proxy internal messages */
        case MARSHAL_TYPE_state_proxy_initial_state_t:
            stateProxy_HandleInitialState((const state_proxy_initial_state_t*)ind->msg);
            break;

        case MARSHAL_TYPE_state_proxy_version_t:
            stateProxy_HandleRemoteVersion((const state_proxy_version_t*)ind->msg);
            break;

        /* State Proxy event messages */

            /* flags events */
        case MARSHAL_TYPE_PHY_STATE_CHANGED_IND_T:
            stateProxy_HandleRemotePhyStateChangedInd((const PHY_STATE_CHANGED_IND_T*)ind->msg);
            break;
        case MARSHAL_TYPE_AV_A2DP_CONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_A2DP_CONNECTED_IND_T, TRUE, ind->msg);
            break;
        case MARSHAL_TYPE_AV_A2DP_DISCONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_A2DP_DISCONNECTED_IND_T, FALSE, ind->msg);
            break;
        case MARSHAL_TYPE_AV_AVRCP_CONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_AVRCP_CONNECTED_IND_T, TRUE, ind->msg);
            break;
        case MARSHAL_TYPE_AV_AVRCP_DISCONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_AVRCP_DISCONNECTED_IND_T, FALSE, ind->msg);
            break;
        case MARSHAL_TYPE_APP_HFP_CONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_APP_HFP_CONNECTED_IND_T, TRUE, ind->msg);
            break;
        case MARSHAL_TYPE_APP_HFP_DISCONNECTED_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_APP_HFP_DISCONNECTED_IND_T, FALSE, ind->msg);
            break;
        case MARSHAL_TYPE_AV_STREAMING_ACTIVE_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_ACTIVE_IND_T, TRUE, ind->msg);
            break;
        case MARSHAL_TYPE_AV_STREAMING_INACTIVE_IND_T:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_INACTIVE_IND_T, FALSE, ind->msg);
            break;
        case MARSHAL_TYPE_CON_MANAGER_TP_CONNECT_IND_T:
            stateProxy_HandleRemoteConManagerConnectInd((CON_MANAGER_TP_CONNECT_IND_T*)ind->msg);
            break;
        case MARSHAL_TYPE_CON_MANAGER_TP_DISCONNECT_IND_T:
            stateProxy_HandleRemoteConManagerDisconnectInd((CON_MANAGER_TP_DISCONNECT_IND_T*)ind->msg);
            break;
        case MARSHAL_TYPE_state_proxy_msg_empty_payload_t:
            stateProxy_HandleMsgEmptyPayload((const state_proxy_msg_empty_payload_t*)ind->msg);
            break;

            /* handset addr and TWS version events */
        case MARSHAL_TYPE_state_proxy_active_handset_addr_t:
            stateProxy_handleActiveHandsetAddr((const state_proxy_active_handset_addr_t*)ind->msg);
            break;

            /* pairing events */
        case MARSHAL_TYPE_PAIRING_ACTIVITY_T:
            stateProxy_HandleRemotePairingHandsetActivity((const PAIRING_ACTIVITY_T*)ind->msg);
            break;

            /* battery events */
        case MARSHAL_TYPE_MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T:
            stateProxy_HandleRemoteBatteryLevelVoltage((const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T*)ind->msg);
            break;
        case MARSHAL_TYPE_MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T:
            stateProxy_HandleRemoteBatteryLevelState((const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T*)ind->msg);
            break;

        case MARSHAL_TYPE_STATE_PROXY_LINK_QUALITY_T:
            stateProxy_HandleRemoteLinkQuality((const STATE_PROXY_LINK_QUALITY_T*)ind->msg);
            break;
        case MARSHAL_TYPE_STATE_PROXY_MIC_QUALITY_T:
            stateProxy_HandleRemoteMicQuality((const STATE_PROXY_MIC_QUALITY_T *)ind->msg);
            break;

        case MARSHAL_TYPE_STATE_PROXY_ANC_DATA_T:
            stateProxy_HandleRemoteAncUpdate((const ANC_UPDATE_IND_T *)ind->msg);
            break;

        case MARSHAL_TYPE_STATE_PROXY_LEAKTHROUGH_DATA_T:
            stateProxy_HandleRemoteLeakthroughUpdate((const LEAKTHROUGH_UPDATE_IND_T *)ind->msg);
            break;

            /* connection events TBD */
        default:
            break;
    }
    /* message is an unmarshalled piece of dynamic memory */
    free(ind->msg);
}

/*! State Proxy Message Handler. */
static void stateProxy_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
            /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            stateProxy_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            stateProxy_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

            /* connection change indications */
        case MIRROR_PROFILE_CONNECT_IND:
            if (stateProxy_IsPrimary())
            {
                break;
            }
        /* Fallthrough */
        case CON_MANAGER_TP_CONNECT_IND:
            stateProxy_HandleConManagerConnectInd((const CON_MANAGER_TP_CONNECT_IND_T*)message);
            break;

        /* disconnection change indications */
        case MIRROR_PROFILE_DISCONNECT_IND:
            if (stateProxy_IsPrimary())
            {
                break;
            }
        /* Fallthrough */
        case CON_MANAGER_TP_DISCONNECT_IND:
            stateProxy_HandleConManagerDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T*)message);
            break;

            /* phy state change indications */
        case PHY_STATE_CHANGED_IND:
            stateProxy_HandlePhyStateChangedInd((const PHY_STATE_CHANGED_IND_T*)message);
            break;

            /* battery change indications */
        case MESSAGE_BATTERY_LEVEL_UPDATE_STATE:
            stateProxy_HandleBatteryLevelUpdateState((const MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T*)message);
            break;
        case MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE:
            stateProxy_HandleBatteryLevelUpdateVoltage((const MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T*)message);
            break;

            /* AV status change indications */
            /*! \todo MUST UPDATE concept of profiles connected and supported like peer sync did!!! */
        case AV_A2DP_CONNECTED_IND:
            if (appDeviceIsHandset(&((AV_A2DP_CONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_A2DP_CONNECTED_IND_T, TRUE,
                                                 message, sizeof(AV_A2DP_CONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case AV_A2DP_DISCONNECTED_IND:
            if (appDeviceIsHandset(&((AV_A2DP_DISCONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_A2DP_DISCONNECTED_IND_T, FALSE,
                                                 message, sizeof(AV_A2DP_DISCONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case AV_AVRCP_CONNECTED_IND:
            if (appDeviceIsHandset(&((AV_AVRCP_CONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_AVRCP_CONNECTED_IND_T, TRUE,
                                                 message, sizeof(AV_AVRCP_CONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case AV_AVRCP_DISCONNECTED_IND:
            if (appDeviceIsHandset(&((AV_AVRCP_DISCONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_AVRCP_DISCONNECTED_IND_T, FALSE,
                                                 message, sizeof(AV_AVRCP_DISCONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case AV_STREAMING_ACTIVE_IND:
            stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_ACTIVE_IND_T, TRUE,
                                             message, sizeof(AV_STREAMING_ACTIVE_IND_T));
            break;
        case AV_STREAMING_INACTIVE_IND:
            stateProxy_FlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_INACTIVE_IND_T, FALSE,
                                             message, sizeof(AV_STREAMING_INACTIVE_IND_T));
            break;
            /* HFP status change indications */
        case APP_HFP_CONNECTED_IND:
            if (appDeviceIsHandset(&((APP_HFP_CONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_APP_HFP_CONNECTED_IND_T, TRUE,
                                                 message, sizeof(APP_HFP_CONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case APP_HFP_DISCONNECTED_IND:
            if (appDeviceIsHandset(&((APP_HFP_DISCONNECTED_IND_T*)message)->bd_addr))
            {
                stateProxy_FlagIndicationHandler(MARSHAL_TYPE_APP_HFP_DISCONNECTED_IND_T, FALSE,
                                                 message, sizeof(APP_HFP_DISCONNECTED_IND_T));
                stateProxy_UpdateActiveHandsetAddr();
            }
            break;
        case APP_HFP_SCO_CONNECTED_IND:
        case MIRROR_PROFILE_ESCO_CONNECT_IND:
            /* Treat mirror/actual eSCO connect identically, without message content */
            stateProxy_FlagIndicationHandler(MARSHAL_TYPE_APP_HFP_SCO_CONNECTED_IND_T, TRUE,
                                             NULL, 0);
            stateProxy_MicQualityKick();
            break;
        case APP_HFP_SCO_DISCONNECTED_IND:
        case MIRROR_PROFILE_ESCO_DISCONNECT_IND:
            /* Treat mirror/actual eSCO disconnect identically, without message content */
            stateProxy_FlagIndicationHandler(MARSHAL_TYPE_APP_HFP_SCO_DISCONNECTED_IND_T, FALSE,
                                             NULL, 0);
            stateProxy_MicQualityKick();
            break;

            /* Peer signalling notifications */
        case PEER_SIG_CONNECTION_IND:
            stateProxy_HandlePeerSigConnectionInd((const PEER_SIG_CONNECTION_IND_T*)message);
            break;

            /* Pairing notifications */
        case PAIRING_ACTIVITY:
            stateProxy_HandlePairingHandsetActivity((const PAIRING_ACTIVITY_T*)message);
            break;

        case CL_DM_LINK_QUALITY_BDADDR_CFM:
            stateProxy_HandleClDmLinkQualityBdaddrCfm((const CL_DM_LINK_QUALITY_BDADDR_CFM_T *)message);
            break;

        case STATE_PROXY_INTERNAL_TIMER_LINK_QUALITY:
            stateProxy_HandleIntervalTimerLinkQuality();
            break;

        case STATE_PROXY_INTERNAL_TIMER_MIC_QUALITY:
            stateProxy_HandleIntervalTimerMicQuality();
            break;

            /* ANC update indication */
        case ANC_UPDATE_IND:
            stateProxy_HandleAncUpdateInd((const ANC_UPDATE_IND_T*)message);
            break;

        /* Leakthrough Update Indication */
        case LEAKTHROUGH_UPDATE_IND:
            stateProxy_HandleLeakthroughUpdateInd((const LEAKTHROUGH_UPDATE_IND_T*)message);
            break;

        default:
            DEBUG_LOG("stateProxyHandleMessage unhandled message id %u", id);
            break;
    }
}

static state_proxy_data_t *stateProxy_createData(void)
{
    state_proxy_connection_t *conn = NULL;
    state_proxy_data_t *data = PanicUnlessMalloc(sizeof(state_proxy_data_t));
    memset(data, 0, sizeof(*data));

    ARRAY_FOREACH(conn, data->connection)
    {
        BdaddrTpSetEmpty(&conn->device);
    }
    data->mic_quality = MIC_QUALITY_UNAVAILABLE;

    return data;
}

/*******************
 * Public Functions
 *******************/

/*! \brief Initialise the State Proxy component.
    \param[in] Initialise component task.
    \return bool
*/
bool StateProxy_Init(Task init_task)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    UNUSED(init_task);

    DEBUG_LOG("StateProxy_Init");

    /* Initialise component task data */
    memset(proxy, 0, sizeof(*proxy));
    proxy->task.handler = stateProxy_HandleMessage;

    /* Initialise local and remote state stores */
    proxy->local_state = stateProxy_createData();
    proxy->remote_state = stateProxy_createData();

    /* Initialise TaskLists */
    proxy->event_tasks = TaskList_WithDataCreate();
    TaskList_InitialiseWithCapacity(stateProxy_GetEvents(), STATE_PROXY_EVENTS_TASK_LIST_INIT_CAPACITY);

    /* start in paused state */
    proxy->paused = TRUE;

    /* Register with peer signalling to use the State Proxy msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(stateProxy_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                               state_proxy_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);

    /* get notification of peer signalling availability to send initial state to peer */
    appPeerSigClientRegister(stateProxy_GetTask());

    /* register for notifications this component is interested in */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, stateProxy_GetTask());
    appPhyStateRegisterClient(stateProxy_GetTask());
    stateProxy_RegisterBatteryClient();
    appHfpStatusClientRegister(stateProxy_GetTask());
    appAvStatusClientRegister(stateProxy_GetTask());
    Pairing_ActivityClientRegister(stateProxy_GetTask());
    MirrorProfile_ClientRegister(stateProxy_GetTask());
    AncStateManager_ClientRegister(stateProxy_GetTask());
    AecLeakthrough_ClientRegister(stateProxy_GetTask());
    /* \todo also need to register for audio mic quality notifications, but they
     * dont' exist yet */

    /* get our initial state */
    stateProxy_GetInitialState();

    return TRUE;
}

/*! \brief Register a task for event(s) updates. */
void StateProxy_EventRegisterClient(Task client_task, state_proxy_event_type event_mask)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    if (!TaskList_IsTaskOnList(proxy->event_tasks, client_task))
    {
        task_list_data_t data = {0};

        /* first time registration for this task, add to list */
        data.u32 = event_mask;
        TaskList_AddTaskWithData(proxy->event_tasks, client_task, &data);
    }
    else
    {
        /* client already on the list, add events */
        task_list_data_t* data = NULL;
        if (TaskList_GetDataForTaskRaw(proxy->event_tasks, client_task, &data))
        {
            data->u32 |= event_mask;
        }
    }
    stateProxy_LinkQualityKick();
    stateProxy_MicQualityKick();
}

/*! \brief Unregister event(s) updates for the specified task. */
void StateProxy_EventUnregisterClient(Task client_task, state_proxy_event_type event_mask)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    task_list_data_t* data = NULL;

    /* clear the events in the event_mask for the client_task
     * if no events are left registered then remove the task from the list. */
    if (TaskList_GetDataForTaskRaw(proxy->event_tasks, client_task, &data))
    {
        data->u32 &= ~event_mask;
        if (!data->u32)
        {
            TaskList_RemoveTask(proxy->event_tasks, client_task);
        }
    }
    stateProxy_LinkQualityKick();
    stateProxy_MicQualityKick();
}

/*! \brief Register for events concerning state proxy itself. */
void StateProxy_StateProxyEventRegisterClient(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(stateProxy_GetEvents()), client_task);
}

/*! \brief Send current device state to peer to initialise event baseline, and enable event forwarding. */
/*! \todo should this post internal version of itself before handling? */
/*! \todo consider renaming to StateProxy_StartAndSync() or similar */
void StateProxy_SendInitialState(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    bdaddr peer_addr;
    size_t local_data_size = stateProxy_GetStateProxyDataSize(state_proxy_source_local);

    DEBUG_LOG("StateProxy_SendInitialState");

    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        state_proxy_initial_state_t* msg = PanicUnlessMalloc(local_data_size);
        memset(msg, 0, local_data_size);
        memcpy(&msg->state, proxy->local_state, local_data_size);

        /* Bring up peer sig connection */
        if (!appPeerSigIsConnected())
        {
            DEBUG_LOG("StateProxy_SendInitialState - force connecting peer_signalling");
            appPeerSigConnect(stateProxy_GetTask(), &peer_addr);
        }

        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                msg, MARSHAL_TYPE_state_proxy_initial_state_t);
        /* let state_proxy clients know initial state was sent */
        proxy->initial_state_sent = TRUE;
        /* unpause */
        proxy->paused = FALSE;
        stateProxy_MsgStateProxyEventInitialStateSent();
    }
    else
    {
        /* Note: The system doesn't work if it fails to send the initial state to the other earbud */
        DEBUG_LOG("StateProxy_SendInitialState no peer to send to");
    }
}

/*! \brief Inform state proxy of current device Primary/Secondary role.

    Currently used to determine if state proxy needs to forward events
    to the Primary.

    TEMPORARY eventually the state proxy will be able to listen to role
    events from the role selection peer service.
*/
void StateProxy_SetRole(bool primary)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->is_primary = primary;
}

/*! \brief Prevent State Proxy forwarding any events */
void StateProxy_Stop(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    DEBUG_LOG("StateProxy_Stop");
    proxy->paused = TRUE;
}

/*! \brief Has initial state been received from peer.
    \return TRUE if initial state received, otherwise FALSE.
*/
bool StateProxy_InitialStateReceived(void)
{
    return stateProxy_GetTaskData()->initial_state_received;
}

/*************************
 * State Access Functions
 *************************/

/*! \brief A table defining each state/flag accessor function.
    \param X The table is expanded using the macro X.  */
#define FOR_EACH_FLAG(X) \
    X(Is, InCase, , in_case) \
    X(Is, OutOfCase, !, in_case) \
    X(Is, InEar, , in_ear) \
    X(Is, OutOfEar, !, in_ear) \
    X(Is, HandsetA2dpConnected, , a2dp_connected) \
    X(Is, HandsetHfpConnected, , hfp_connected) \
    X(Is, HandsetAvrcpConnected, , avrcp_connected) \
    X(Is, HandsetA2dpStreaming, , a2dp_streaming) \
    X(Is, ScoActive, , sco_active) \
    X(Is, Pairing, , is_pairing) \
    X(Has, HandsetPairing, , has_handset_pairing) \
    X(Is, Advertising, , advertising) \
    X(Is, BleConnected, , ble_connected) \
    X(Is, DfuInProgress, , dfu_in_progress) \

/*! \brief X-Macro generator, creating a remote flag accessor function */
#define GENERATE_PEER_ACCESSOR(verb, func, negate, flag) \
bool StateProxy_##verb##Peer##func##(void) \
{\
    return !!##negate##stateProxy_GetRemoteFlag(##flag##); \
}

/*! Use the table and X-Macro to generate a list of peer flag accessor functions */
FOR_EACH_FLAG(GENERATE_PEER_ACCESSOR)

/*! \brief X-Macro generator, creating a remote flag accessor function */
#define GENERATE_SELF_ACCESSOR(verb, func, negate, flag) \
bool StateProxy_##verb##func##(void) \
{\
    return !!##negate##stateProxy_GetLocalFlag(##flag##); \
}

/*! Use the table and X-Macro to generate a list of 'self' flag accessor functions */
FOR_EACH_FLAG(GENERATE_SELF_ACCESSOR)

void StateProxy_GetLocalAndRemoteBatteryLevels(uint16 *battery_level, uint16 *peer_battery_level)
{
    /*! \todo gets levels not just state */
    *battery_level = appBatteryGetVoltage();
    *peer_battery_level = stateProxy_GetRemoteData()->battery_voltage;
}

void StateProxy_GetLocalAndRemoteBatteryStates(battery_level_state *battery_state,
                                               battery_level_state *peer_battery_state)
{
    *battery_state = appBatteryGetState();
    *peer_battery_state = stateProxy_GetRemoteData()->battery;
}

void StateProxy_GetPeerHandsetAddr(bdaddr *peer_handset_addr)
{
    *peer_handset_addr = stateProxy_GetRemoteData()->handset_addr;
}

bool StateProxy_IsPrimary(void)
{
    return state_proxy.is_primary;
}

bool StateProxy_GetPeerAncState(void)
{
    return stateProxy_GetRemoteData()->flags.anc_state;
}

uint8 StateProxy_GetPeerAncMode(void)
{
    return stateProxy_GetRemoteData()->anc_mode;
}

bool StateProxy_GetPeerLeakthroughState(void)
{
    return stateProxy_GetRemoteData()->flags.leakthrough_state;
}

uint8 StateProxy_GetPeerLeakthroughMode(void)
{
    return stateProxy_GetRemoteData()->leakthrough_mode;
}

state_proxy_data_t* stateProxy_GetData(state_proxy_source source)
{
    return (source == state_proxy_source_local) ?
                stateProxy_GetLocalData() :
                stateProxy_GetRemoteData();
}
