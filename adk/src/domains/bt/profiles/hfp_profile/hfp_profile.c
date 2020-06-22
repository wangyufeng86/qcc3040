/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application domain HFP component.
*/

#include <panic.h>
#include <ps.h>

#ifdef INCLUDE_HFP

#include "hfp_profile.h"

#include "init.h"
#include "bt_device.h"
#include "device_properties.h"
#include "hfp_profile_config.h"
#include "link_policy.h"
#include "voice_sources.h"
#include "hfp_profile_audio.h"
#include "hfp_profile_telephony_control.h"
#include "hfp_profile_voice_source_device_mapping.h"
#include "hfp_profile_volume.h"
#include "hfp_profile_volume_observer.h"
#include "telephony_messages.h"
#include "volume_messages.h"
#include "volume_utils.h"
#include "kymera.h"

#include <profile_manager.h>

#include <connection_manager.h>
#include <device.h>
#include <device_list.h>
#include <logging.h>
#include <scofwd_profile.h>
#include <mirror_profile.h>
#include <stdio.h>
#include <stdlib.h>
#include <message.h>
#include "scofwd_profile_config.h"
#include "ui.h"

/*! \todo layer violation.
    This domain component should not be referencing a service
    layer component.
*/
#include <state_proxy.h>

/*! Macro for creating a message based on the message name */
#define MAKE_HFP_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);
/*! Macro for creating a variable length message based on the message name */
#define MAKE_HFP_MESSAGE_WITH_LEN(TYPE, LEN) \
    TYPE##_T *message = (TYPE##_T *)PanicUnlessMalloc(sizeof(TYPE##_T) + (LEN-1));

#ifndef HFP_SPEAKER_GAIN
#define HFP_SPEAKER_GAIN    (10)
#endif
#ifndef HFP_MICROPHONE_GAIN
#define HFP_MICROPHONE_GAIN (15)
#endif

#define PSKEY_LOCAL_SUPPORTED_FEATURES (0x00EF)
#define PSKEY_LOCAL_SUPPORTED_FEATURES_SIZE (4)
#define PSKEY_LOCAL_SUPPORTED_FEATURES_DEFAULTS { 0xFEEF, 0xFE8F, 0xFFDB, 0x875B }

/*! \brief Application HFP component main data structure. */
hfpTaskData appHfp;

/*! \brief Get application HFP component task */
/*#define appGetHfpTask()     (&(appGetHfp()->task))*/

/*! \brief Get current HFP state */
#define appHfpGetState() (appGetHfp()->state)

/*! \brief Call status state lookup table

    This table is used to convert the call setup and call indicators into
    the appropriate HFP state machine state
*/
const hfpState hfp_call_state_table[10] =
{
    /* hfp_call_state_idle              */ HFP_STATE_CONNECTED_IDLE,
    /* hfp_call_state_incoming          */ HFP_STATE_CONNECTED_INCOMING,
    /* hfp_call_state_incoming_held     */ HFP_STATE_CONNECTED_INCOMING,
    /* hfp_call_state_outgoing          */ HFP_STATE_CONNECTED_OUTGOING,
    /* hfp_call_state_active            */ HFP_STATE_CONNECTED_ACTIVE,
    /* hfp_call_state_twc_incoming      */ HFP_STATE_CONNECTED_IDLE, /* Not supported */
    /* hfp_call_state_twc_outgoing      */ HFP_STATE_CONNECTED_IDLE, /* Not supported */
    /* hfp_call_state_held_active       */ HFP_STATE_CONNECTED_IDLE, /* Not supported */
    /* hfp_call_state_held_remaining    */ HFP_STATE_CONNECTED_IDLE, /* Not supported */
    /* hfp_call_state_multiparty        */ HFP_STATE_CONNECTED_IDLE, /* Not supported */
};

/* Ui Inputs in which telephony module is interested*/
const message_group_t hfp_ui_inputs[] =
{
    UI_INPUTS_PEER_MESSAGE_GROUP,
    UI_INPUTS_VOLUME_MESSAGE_GROUP
};

/* Local Function Prototypes */
static void appHfpHandleMessage(Task task, MessageId id, Message message);

/*! \brief returns hfp task pointer to requesting component

    \return hfp task pointer.
*/
Task appGetHfpTask(void)
{
  return &appGetHfp()->task;
}

/*! \brief provides hfp (telephony) current context to ui module

    \param[in]  void

    \return     current context of hfp module.
*/
static unsigned hfpProfile_GetCurrentContext(void)
{
    hfp_provider_context_t context = BAD_CONTEXT;

    switch(appHfpGetState())
    {
    case HFP_STATE_NULL:
    case HFP_STATE_INITIALISING_HFP:
        break;
    case HFP_STATE_DISCONNECTED:
        context = context_hfp_disconnected;
        break;
    case HFP_STATE_CONNECTING_LOCAL:
    case HFP_STATE_CONNECTING_REMOTE:
        break;
    case HFP_STATE_CONNECTED_IDLE:
        context = context_hfp_connected;
        break;
    case HFP_STATE_CONNECTED_OUTGOING:
        context = context_hfp_voice_call_outgoing;
        break;
    case HFP_STATE_CONNECTED_INCOMING:
        context = context_hfp_voice_call_incoming;
        break;
    case HFP_STATE_CONNECTED_ACTIVE:
        if (appHfpIsScoActive())
        {
            context = context_hfp_voice_call_sco_active;
        }
        else
        {
            context = context_hfp_voice_call_sco_inactive;
        }
        break;
    case HFP_STATE_DISCONNECTING:
        break;
    default:
        break;
    }
    return (unsigned)context;
}

static int16 appHfpGetVolumeChange(ui_input_t ui_input)
{
    uint16 num_pending_msgs = 0;
    uint16 num_steps = 1;
    int16 hfp_change = 0;

    if(ui_input == ui_input_hfp_volume_up_start || ui_input == ui_input_sco_fwd_volume_up_start)
    {
        num_pending_msgs = MessageCancelAll(&appHfp.task, ui_input_hfp_volume_up_start);
        num_steps += num_pending_msgs;
        hfp_change = hfpConfigGetHfpVolumeStep() * num_steps;
    }
    else if(ui_input == ui_input_hfp_volume_down_start || ui_input == ui_input_sco_fwd_volume_down_start)
    {
        num_pending_msgs = MessageCancelAll(&appHfp.task, ui_input_hfp_volume_down_start);
        num_steps += num_pending_msgs;
        hfp_change = -(hfpConfigGetHfpVolumeStep() * num_steps);
    }
    return hfp_change;
}

/*! \brief handles hfp module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void appHfpHandleUiInput(MessageId  ui_input)
{
    switch (ui_input)
    {
        case ui_input_sco_forward_call_hang_up:
            ScoFwdCallHangup();
            break;

        case ui_input_sco_forward_call_accept:
            ScoFwdCallAccept();
            break;

        case ui_input_sco_voice_call_reject:
            ScoFwdCallReject();
            break;

        case ui_input_hfp_volume_stop:
            appHfpVolumeStop(0);
            break;

        case ui_input_hfp_mute_toggle:
            appHfpMuteToggle();
            break;

        case ui_input_hfp_volume_down_start:
        case ui_input_hfp_volume_up_start:
            appHfpVolumeStart(appHfpGetVolumeChange(ui_input));
            break;

        case ui_input_sco_fwd_volume_down_start:
        case ui_input_sco_fwd_volume_up_start:
            ScoFwdVolumeStart(appHfpGetVolumeChange(ui_input));
            break;

        case ui_input_sco_fwd_volume_stop:
            ScoFwdVolumeStop(-hfpConfigGetHfpVolumeStep());
            break;
            
        default:
            break;
    }
}

/*! \brief Entering `Initialising HFP` state

    This function is called when the HFP state machine enters
    the 'Initialising HFP' state, it calls the HfpInit() function
    to initialise the profile library for HFP.
*/
static void appHfpEnterInitialisingHfp(void)
{
    hfp_init_params hfp_params = {0};
    uint16 supp_features = (HFP_VOICE_RECOGNITION |
                            HFP_NREC_FUNCTION |
                            HFP_REMOTE_VOL_CONTROL |
                            HFP_CODEC_NEGOTIATION |
                            HFP_HF_INDICATORS |
                            HFP_ESCO_S4_SUPPORTED);

    /* Initialise an HFP profile instance */
    hfp_params.supported_profile = hfp_handsfree_107_profile;
    hfp_params.supported_features = supp_features;
    hfp_params.disable_nrec = TRUE;
    hfp_params.extended_errors = FALSE;
    hfp_params.optional_indicators.service = hfp_indicator_off;
    hfp_params.optional_indicators.signal_strength = hfp_indicator_off;
    hfp_params.optional_indicators.roaming_status = hfp_indicator_off;
    hfp_params.optional_indicators.battery_charge = hfp_indicator_off;
    hfp_params.multipoint = FALSE;
    hfp_params.supported_wbs_codecs = hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc;
    hfp_params.link_loss_time = 1;
    hfp_params.link_loss_interval = 5;
    if (appConfigHfpBatteryIndicatorEnabled())
        hfp_params.hf_indicators = hfp_battery_level_mask;
    else
        hfp_params.hf_indicators = hfp_indicator_mask_none;

#ifdef INCLUDE_SWB
    if (appConfigScoSwbEnabled())
        hfp_params.hf_codec_modes = CODEC_64_2_EV3;
    else
        hfp_params.hf_codec_modes = 0;
#endif

    HfpInit(appGetHfpTask(), &hfp_params, NULL);
}

/*! \brief Enter 'connecting local' state

    The HFP state machine has entered 'connecting local' state.  Set the
    'connect busy' flag and operation lock to serialise connect attempts,
    reset the page timeout to the default and attempt to connect SLC.
    Make sure AV streaming is suspended.
*/
static void appHfpEnterConnectingLocal(void)
{
    DEBUG_LOG("appHfpEnterConnectingLocal");

    /* Set operation lock */
    appHfpSetLock(TRUE);

    if (!appGetHfp()->bitfields.flags & HFP_CONNECT_NO_UI)
        TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), PAGING_START);

    /* Clear detach pending flag */
    appGetHfp()->bitfields.detach_pending = FALSE;

    /* Start HFP connection */
    if (appGetHfp()->profile == hfp_handsfree_107_profile)
    {
        DEBUG_LOG("Connecting HFP to AG (%x,%x,%lx)", appGetHfp()->ag_bd_addr.nap, appGetHfp()->ag_bd_addr.uap, appGetHfp()->ag_bd_addr.lap);

        /* Issue connect request for HFP */
        HfpSlcConnectRequest(&appGetHfp()->ag_bd_addr, hfp_handsfree_and_headset, hfp_handsfree_all);
    }
    else
        Panic();
}

/*! \brief Exit 'connecting local' state

    The HFP state machine has exited 'connecting local' state, the connection
    attempt was successful or it failed.  Clear the 'connect busy' flag and
    operation lock to allow pending connection attempts and any pending
    operations on this instance to proceed.  AV streaming can resume now.
*/
static void appHfpExitConnectingLocal(void)
{
    DEBUG_LOG("appHfpExitConnectingLocal");

    /* Clear operation lock */
    appHfpSetLock(FALSE);

    if (!appGetHfp()->bitfields.flags & HFP_CONNECT_NO_UI)
        TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), PAGING_STOP);

    /* We have finished (successfully or not) attempting to connect, so
     * we can relinquish our lock on the ACL.  Bluestack will then close
     * the ACL when there are no more L2CAP connections */
    ConManagerReleaseAcl(&appGetHfp()->ag_bd_addr);
}

/*! \brief Enter 'connecting remote' state

    The HFP state machine has entered 'connecting remote' state, this is due
    to receiving a incoming SLC indication. Set operation lock to block any
    pending operations.
*/
static void appHfpEnterConnectingRemote(void)
{
    DEBUG_LOG("appHfpEnterConnectingRemote");

    /* Set operation lock */
    appHfpSetLock(TRUE);

    /* Clear detach pending flag */
    appGetHfp()->bitfields.detach_pending = FALSE;
}

/*! \brief Exit 'connecting remote' state

    The HFP state machine has exited 'connecting remote' state.  Clear the
    operation lock to allow pending operations on this instance to proceed.
*/
static void appHfpExitConnectingRemote(void)
{
    DEBUG_LOG("appHfpExitConnectingRemote");

    /* Clear operation lock */
    appHfpSetLock(FALSE);
}

/*! \brief Set flag to indicate HFP is supported on a device

    \param bd_addr Pointer to read-only device BT address.
    \param profile HFP profile version support by the device
*/
static void hfpProfile_SetHfpIsSupported(const bdaddr *bd_addr, hfp_profile profile)
{
    device_t device = BtDevice_SetSupportedProfile(bd_addr, DEVICE_PROFILE_HFP);
    if (device)
        Device_SetPropertyU8(device, device_property_hfp_profile, (uint8)profile);
}

static bool hfpProfile_FindClientSendConnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;

    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, APP_HFP_CONNECT_CFM_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("hfpProfile_FindClientSendConnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, APP_HFP_CONNECT_CFM, msg);

        hfpTaskData * hfp = appGetHfp();
        TaskList_RemoveTask(TaskList_GetBaseTaskList(&hfp->connect_request_clients), task);
    }
    return !found_client_task;
}

static bool hfpProfile_FindClientSendDisconnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, APP_HFP_DISCONNECT_CFM_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("hfpProfile_FindClientSendDisconnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, APP_HFP_DISCONNECT_CFM, msg);

        hfpTaskData * hfp = appGetHfp();
        TaskList_RemoveTask(TaskList_GetBaseTaskList(&hfp->disconnect_request_clients), task);
    }
    return !found_client_task;
}

/*! \brief Enter 'connected' state

    The HFP state machine has entered 'connected' state, this means that
    there is a SLC active.  At this point we need to retreive the remote device's
    support features to determine which (e)SCO packets it supports.  Also if there's an
    incoming or active call then answer/transfer the call to headset.
*/
static void appHfpEnterConnected(void)
{
    DEBUG_LOG("appHfpEnterConnected");

    /* Update most recent connected device */
    appDeviceUpdateMruDevice(&appGetHfp()->ag_bd_addr);

    /* Mark this device as supporting HFP */
    hfpProfile_SetHfpIsSupported(&appGetHfp()->ag_bd_addr, appGetHfp()->profile);

    /* Read the remote supported features of the AG */
    ConnectionReadRemoteSuppFeatures(appGetHfpTask(), appGetHfp()->slc_sink);

    /* Check if connected as HFP 1.5 */
    if (appGetHfp()->profile == hfp_handsfree_107_profile)
    {
        /* Inform AG of the current gain settings */
        /* hfp_primary_link indicates the link that was connected first */
        /* TODO : Handle multipoint support */
        HfpVolumeSyncSpeakerGainRequest((hfp_link_priority)hfp_primary_link, &appGetHfp()->volume);
        HfpVolumeSyncMicrophoneGainRequest(hfp_primary_link, &appGetHfp()->mic_volume);
    }

    /* Set link supervision timeout to 5 seconds */
    //ConnectionSetLinkSupervisionTimeout(appGetHfp()->slc_sink, 0x1F80);
    appLinkPolicyUpdateRoleFromSink(appGetHfp()->slc_sink);

    /* If this is completing a connect request, send confirmation for this device */
    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->connect_request_clients),
                                            &appGetHfp()->ag_bd_addr, profile_manager_success, hfpProfile_FindClientSendConnectCfm);

    Telephony_NotifyConnected(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

    /* Clear silent flags */
    appGetHfp()->bitfields.flags &= ~(HFP_CONNECT_NO_UI | HFP_CONNECT_NO_ERROR_UI | HFP_DISCONNECT_NO_UI);

    /* Tell clients we have connected */
    MAKE_HFP_MESSAGE(APP_HFP_CONNECTED_IND);
    message->bd_addr = appGetHfp()->ag_bd_addr;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_CONNECTED_IND, message);

#if defined(HFP_CONNECT_AUTO_ANSWER) || defined(HFP_CONNECT_AUTO_TRANSFER)
    if (appGetHfp()->profile != hfp_headset_profile)
    {
#if defined(HFP_CONNECT_AUTO_ANSWER)
        /* Check if incoming call */
        if (appHfpGetState() == HFP_STATE_CONNECTED_INCOMING)
        {
            /* Accept the incoming call */
            appHfpCallAccept();
        }
#endif
#if defined(HFP_CONNECT_AUTO_TRANSFER)
        /* Check if incoming call */
        if (appHfpGetState() == HFP_STATE_CONNECTED_ACTIVE)
        {
            /* Check SCO is not active */
            if (appGetHfp()->sco_sink == 0)
            {
                /* Attempt to transfer audio */
                HfpAudioTransferConnection(appGetHfp()->hfp, hfp_audio_to_hfp, appGetHfp()->sco_supported_packets ^ sync_all_edr_esco, 0);
            }
        }
#endif
    }
#endif
}

/*! \brief Exit 'connected' state

    The HFP state machine has exited 'connected' state, this means that
    the SLC has closed.  Make sure any SCO link is disconnected.
*/
static void appHfpExitConnected(void)
{
    DEBUG_LOG("appHfpExitConnected");

    /* Unregister for battery updates */
    appBatteryUnregister(appGetHfpTask());

    /* Reset hf_indicator_assigned_num */
     appGetHfp()->bitfields.hf_indicator_assigned_num = hf_indicators_invalid;

    /* Check if SCO is still up */
    if (appGetHfp()->sco_sink)
    {
        /* Disconnect SCO */
        /* TODO: Support multipoint */
        HfpAudioDisconnectRequest(hfp_primary_link);
    }

    /* Handle any pending config write */
    if (MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_CONFIG_WRITE_REQ) > 0)
    {
        appHfpHandleInternalConfigWriteRequest();
    }
}

/*! \brief Enter 'connected idle' state

    The HFP state machine has entered 'connected idle' state, this means that
    there is a SLC active but no active call in process.  When entering this
    state we clear the ACL lock that was when entering the 'connecting local'
    state, this allows any other pending connections to proceed.  Any suspended
    AV streaming can now resume.
*/
static void appHfpEnterConnectedIdle(void)
{
    DEBUG_LOG("appHfpEnterConnectedIdle");

#ifdef INCLUDE_AV
    /* Resume AV streaming */
    appAvStreamingResume(AV_SUSPEND_REASON_HFP);
#endif
}

/*! \brief Exit 'connected idle' state

    The HFP state machine has exited 'connected idle' state.
*/
static void appHfpExitConnectedIdle(void)
{
    DEBUG_LOG("appHfpExitConnectedIdle");
}

/*! \brief Enter 'connected outgoing' state

    The HFP state machine has entered 'connected outgoing' state, this means
    that we are in the process of making an outgoing call, just make sure
    that we have suspended AV streaming. Update UI to indicate active call.
*/
static void appHfpEnterConnectedOutgoing(void)
{
    DEBUG_LOG("appHfpEnterConnectedOutgoing");

    Telephony_NotifyCallActive(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

#ifdef INCLUDE_AV
    /* We should suspend AV streaming */
    appAvStreamingSuspend(AV_SUSPEND_REASON_HFP);
#endif
}

/*! \brief Exit 'connected outgoing' state

    The HFP state machine has exited 'connected outgoing' state.
*/
static void appHfpExitConnectedOutgoing(void)
{
    DEBUG_LOG("appHfpExitConnectedOutgoing");
}

/*! \brief Enter 'connected incoming' state

    The HFP state machine has entered 'connected incoming' state, this means
    that there's an incoming call in progress.  Update UI to indicate incoming
    call.
*/
static void appHfpEnterConnectedIncoming(void)
{
    DEBUG_LOG("appHfpEnterConnectedIncoming");

    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_SCO_INCOMING_RING_IND);

    Telephony_NotifyCallIncoming(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
}

/*! \brief Exit 'connected incoming' state

    The HFP state machine has exited 'connected incoming' state, this means
    that the incoming call has either been accepted or rejected.  Make sure
    any ring tone is cancelled.
*/
static void appHfpExitConnectedIncoming(void)
{
    DEBUG_LOG("appHfpExitConnectedIncoming");

    /* Clear call accepted flag */
    appGetHfp()->bitfields.call_accepted = FALSE;

    /* TODO: Cancel any ring-tones */
    /* AudioStopTone();*/

    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_SCO_INCOMING_ENDED_IND);
    Telephony_NotifyCallIncomingEnded(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

    /* Cancel HSP incoming call timeout */
    MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_HSP_INCOMING_TIMEOUT);
}

/*! \brief Enter 'connected active' state

    The HFP state machine has entered 'connected active' state, this means
    that a call is in progress, just make sure that we have suspended AV
    streaming.
*/
static void appHfpEnterConnectedActive(void)
{
    DEBUG_LOG("appHfpEnterConnectedActive");

#ifdef INCLUDE_AV
    /* We should suspend AV streaming */
    appAvStreamingSuspend(AV_SUSPEND_REASON_HFP);
#endif
}

/*! \brief Exit 'connected active' state

    The HFP state machine has exited 'connected active' state, this means
    that a call has just finished.  Make sure mute is cancelled.
*/
static void appHfpExitConnectedActive(void)
{
    DEBUG_LOG("appHfpExitConnectedActive");

    Telephony_NotifyMicrophoneUnmuted(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
    appGetHfp()->bitfields.mute_active = FALSE;

    Telephony_NotifyCallEnded(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
}

/*! \brief Enter 'disconnecting' state

    The HFP state machine has entered 'disconnecting' state, this means
    that the SLC should be disconnected.  Set the operation lock to block
    any pending operations.
*/
static void appHfpEnterDisconnecting(void)
{
    DEBUG_LOG("appHfpEnterDisconnecting");

    /* Set operation lock */
    appHfpSetLock(TRUE);

    /* Disconnect SLC */
    /* TODO: Support Multipoint */
    HfpSlcDisconnectRequest(hfp_primary_link);
}

/*! \brief Exit 'disconnecting' state

    The HFP state machine has exited 'disconnecting' state, this means
    that the SLC is now disconnected.  Clear the operation lock to allow
    any pending operations.
*/
static void appHfpExitDisconnecting(void)
{
    DEBUG_LOG("appHfpExitDisconnecting");

    /* Clear operation lock */
    appHfpSetLock(FALSE);
}

/*! \brief Enter 'disconnected' state

    The HFP state machine has entered 'disconnected' state, this means
    that there is now active SLC.  Reset all flags, clear the ACL lock to
    allow pending connections to proceed.  Also make sure AV streaming is
    resumed if previously suspended.
*/
static void appHfpEnterDisconnected(void)
{
    DEBUG_LOG("appHfpEnterDisconnected");

    if (TaskList_Size(TaskList_GetBaseTaskList(&appGetHfp()->connect_request_clients)) != 0)
    {
        if (appGetHfp()->bitfields.disconnect_reason == APP_HFP_CONNECT_FAILED)
        {
            /* If this is due to an unsuccessful connect request, send confirmation for this device */
            ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->connect_request_clients),
                                                    &appGetHfp()->ag_bd_addr, profile_manager_failed, hfpProfile_FindClientSendConnectCfm);
        }
    }
    if (TaskList_Size(TaskList_GetBaseTaskList(&appGetHfp()->disconnect_request_clients)) != 0)
    {
        if (appGetHfp()->bitfields.disconnect_reason == APP_HFP_DISCONNECT_NORMAL)
        {
            ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->disconnect_request_clients),
                                                    &appGetHfp()->ag_bd_addr,
                                                    profile_manager_success,
                                                    hfpProfile_FindClientSendDisconnectCfm);
        }
    }

    /* Tell clients we have disconnected */
    MAKE_HFP_MESSAGE(APP_HFP_DISCONNECTED_IND);
    message->bd_addr = appGetHfp()->ag_bd_addr;
    message->reason =  appGetHfp()->bitfields.disconnect_reason;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_DISCONNECTED_IND, message);

#ifdef INCLUDE_AV
    /* Resume AV streaming if HFP disconnects for any reason */
    appAvStreamingResume(AV_SUSPEND_REASON_HFP);
#endif

    /* Clear status flags */
    appGetHfp()->bitfields.caller_id_active = FALSE;
    appGetHfp()->bitfields.voice_recognition_active = FALSE;
    appGetHfp()->bitfields.voice_recognition_request = FALSE;
    appGetHfp()->bitfields.mute_active = FALSE;
    appGetHfp()->bitfields.in_band_ring = FALSE;
    appGetHfp()->bitfields.call_accepted = FALSE;

    /* Clear call state indication */
    appGetHfp()->bitfields.call_state = 0;
}

static void appHfpExitDisconnected(void)
{
    /* Reset disconnect reason */
    appGetHfp()->bitfields.disconnect_reason = APP_HFP_CONNECT_FAILED;
}

/*! \brief Set HFP state

    Called to change state.  Handles calling the state entry and exit functions.
    Note: The entry and exit functions will be called regardless of whether or not
    the state actually changes value.
*/
static void appHfpSetState(hfpState state)
{
    hfpState old_state;

    DEBUG_LOG("appHfpSetState(%d)", state);

    /* Handle state exit functions */
    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
            appHfpExitConnectingLocal();
            break;
        case HFP_STATE_CONNECTING_REMOTE:
            appHfpExitConnectingRemote();
            break;
        case HFP_STATE_CONNECTED_IDLE:
            appHfpExitConnectedIdle();
            if (state < HFP_STATE_CONNECTED_IDLE || state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpExitConnected();
            break;
        case HFP_STATE_CONNECTED_ACTIVE:
            appHfpExitConnectedActive();
            if (state < HFP_STATE_CONNECTED_IDLE || state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpExitConnected();
            break;
        case HFP_STATE_CONNECTED_INCOMING:
            appHfpExitConnectedIncoming();
            if (state < HFP_STATE_CONNECTED_IDLE || state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpExitConnected();
            break;
        case HFP_STATE_CONNECTED_OUTGOING:
            appHfpExitConnectedOutgoing();
            if (state < HFP_STATE_CONNECTED_IDLE || state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpExitConnected();
            break;
        case HFP_STATE_DISCONNECTING:
            appHfpExitDisconnecting();
            break;
        case HFP_STATE_DISCONNECTED:
            appHfpExitDisconnected();
            break;
        default:
            break;
    }

    /* Set new state, copy old state */
    old_state = appHfpGetState();
    appGetHfp()->state = state;

    /* Handle state entry functions */
    switch (state)
    {
        case HFP_STATE_INITIALISING_HFP:
            appHfpEnterInitialisingHfp();
            break;
        case HFP_STATE_CONNECTING_LOCAL:
            appHfpEnterConnectingLocal();
            break;
        case HFP_STATE_CONNECTING_REMOTE:
            appHfpEnterConnectingRemote();
            break;
        case HFP_STATE_CONNECTED_IDLE:
            if (old_state < HFP_STATE_CONNECTED_IDLE || old_state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpEnterConnected();
            appHfpEnterConnectedIdle();
            break;
        case HFP_STATE_CONNECTED_ACTIVE:
            if (old_state < HFP_STATE_CONNECTED_IDLE || old_state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpEnterConnected();
            appHfpEnterConnectedActive();
            break;
        case HFP_STATE_CONNECTED_INCOMING:
            if (old_state < HFP_STATE_CONNECTED_IDLE || old_state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpEnterConnected();
            appHfpEnterConnectedIncoming();
            break;
        case HFP_STATE_CONNECTED_OUTGOING:
            if (old_state < HFP_STATE_CONNECTED_IDLE || old_state > HFP_STATE_CONNECTED_ACTIVE)
                appHfpEnterConnected();
            appHfpEnterConnectedOutgoing();
            break;
        case HFP_STATE_DISCONNECTING:
            appHfpEnterDisconnecting();
            break;
        case HFP_STATE_DISCONNECTED:
            appHfpEnterDisconnected();
            break;
        default:
            break;
    }

    Ui_InformContextChange(ui_provider_hfp, hfpProfile_GetCurrentContext());

    /* Update link policy following change in state */
    appLinkPolicyUpdatePowerTable(appHfpGetAgBdAddr());
}

/*! \brief Handle HFP error

    Some error occurred in the HFP state machine, to avoid the state machine
    getting stuck, drop connection and move to 'disconnected' state.
*/
void appHfpError(MessageId id, Message message)
{
    UNUSED(message); UNUSED(id);

    DEBUG_LOG("appHfpError, state=%u, id=%x", appGetHfp()->state, id);

    /* Check if we are connected */
    if (appHfpIsConnected())
    {
        /* Move to 'disconnecting' state */
        appHfpSetState(HFP_STATE_DISCONNECTING);
    }
}

/*! \brief Check SCO encryption

    This functions is called to check if SCO is encrypted or not.  If there is
    a SCO link active, a call is in progress and the link becomes unencrypted, start
    a indication tone to the user.
*/
static void appHfpCheckEncryptedSco(void)
{
    DEBUG_LOG("appHfpCheckEncryptedSco(%d, %x)", appGetHfp()->bitfields.encrypted, appGetHfp()->sco_sink);

    /* Check SCO is active */
    if (appHfpIsScoActive() && appHfpIsCall())
    {
        /* Check if link is encrypted */
        if (appHfpIsEncrypted())
        {
            /* SCO is encrypted */
            MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_SCO_UNENCRYPTED_IND);
        }
        else
        {
            Telephony_NotifyCallBecameUnencrypted(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

            /* \todo Mute the MIC to prevent eavesdropping */
        }
    }
}

/*! \brief Handle HFP Library initialisation confirmation
*/
static void appHfpHandleHfpInitConfirm(const HFP_INIT_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpInitConfirm");

    switch (appHfpGetState())
    {
        case HFP_STATE_INITIALISING_HFP:
        {
            /* Check HFP initialisation was successful */
            if (cfm->status == hfp_init_success)
            {
                /* Move to disconnected state */
                appHfpSetState(HFP_STATE_DISCONNECTED);

                /* Tell main application task we have initialised */
                MessageSend(Init_GetInitTask(), APP_HFP_INIT_CFM, 0);
            }
            else
                Panic();
        }
        return;

        default:
            appHfpError(HFP_INIT_CFM, cfm);
            return;
    }
}

/*! \brief Handle SLC connect indication
*/
static void appHfpHandleHfpSlcConnectIndication(const HFP_SLC_CONNECT_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpSlcConnectIndication state %d", appHfpGetState());

    switch (appHfpGetState())
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Check which profile we have connecting */
            /* TODO: Probably need to accept the RFCOMM connection first */
            if (ind->accepted)
            {
                /* Store address of AG */
                appGetHfp()->ag_bd_addr = ind->addr;
            }

            /* TODO: Probably need to accept the RFCOMM connection first */
            appHfpSetState(HFP_STATE_CONNECTING_REMOTE);
        }
        return;

        default:
        /* TODO: What should we do here? Go back to DISCONNECTED ?*/
        return;
    }
}

/*! \brief Send SLC status indication to all clients on the list.
 */
static void appHfpSendSlcStatus(bool connected, hfp_link_priority priority,const bdaddr* bd_addr)
{
    Task next_client = 0;

    while (TaskList_Iterate(TaskList_GetFlexibleBaseTaskList(appHfpGetSlcStatusNotifyList()), &next_client))
    {
        MAKE_HFP_MESSAGE(APP_HFP_SLC_STATUS_IND);
        message->slc_connected = connected;
        message->priority = priority;
        message->bd_addr = *bd_addr;
        MessageSend(next_client, APP_HFP_SLC_STATUS_IND, message);
    }
}

static deviceLinkMode hfpProfile_GetLinkMode(device_t device)
{
    deviceLinkMode link_mode = DEVICE_LINK_MODE_UNKNOWN;
    void *value = NULL;
    size_t size = sizeof(deviceLinkMode);
    if (Device_GetProperty(device, device_property_link_mode, &value, &size))
    {
        link_mode = *(deviceType *)value;
    }
    return link_mode;
}

/*! \brief Determine if a device supports secure connections

    \param bd_addr Pointer to read-only device BT address.
    \return bool TRUE if secure connects supported, FALSE otherwise.
*/
static bool hfpProfile_IsSecureConnection(const bdaddr *bd_addr)
{
    bool is_secure_connection = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        is_secure_connection = (hfpProfile_GetLinkMode(device) == DEVICE_LINK_MODE_SECURE_CONNECTION);
    }
    return is_secure_connection;
}

/*! \brief Handle SLC connect confirmation
*/
static void appHfpHandleHfpSlcConnectConfirm(const HFP_SLC_CONNECT_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpSlcConnectConfirm(%d)", cfm->status);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        {
            /* Check if SLC connection was successful */
            if (cfm->status == hfp_connect_success)
            {
                /* Inform the hfp library if the link is secure */
                if (hfpProfile_IsSecureConnection(&cfm->bd_addr))
                    HfpLinkSetLinkMode(cfm->priority, TRUE);

                /* Store SLC sink */
                appGetHfp()->slc_sink = cfm->sink;

                /* Update profile used */
                appGetHfp()->profile = cfm->profile;

                /* Turn off link-loss management */
                HfpManageLinkLoss(cfm->priority, FALSE);

                /* Ensure the underlying ACL is encrypted*/
                ConnectionSmEncrypt(appGetHfpTask(), appGetHfp()->slc_sink, TRUE);

                /* Move to new connected state */
                appHfpSetState(hfp_call_state_table[appGetHfp()->bitfields.call_state]);

                /* inform clients */
                appHfpSendSlcStatus(TRUE, cfm->priority, &cfm->bd_addr);

                return;
            }

            Telephony_NotifyCallConnectFailure(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(cfm->priority));

            /* Set disconnect reason */
            appGetHfp()->bitfields.disconnect_reason = APP_HFP_CONNECT_FAILED;

            /* Move to disconnected state */
            appHfpSetState(HFP_STATE_DISCONNECTED);
        }
        return;

        default:
            appHfpError(HFP_SLC_CONNECT_CFM, cfm);
            return;
    }
}

/*! \brief Handle SLC disconnect indication
*/
static void appHfpHandleHfpSlcDisconnectIndication(const HFP_SLC_DISCONNECT_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpSlcDisconnectIndication(%d)", ind->status);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            /* Check if SCO is still up */
            if (appGetHfp()->sco_sink)
            {
                /* Disconnect SCO */
                HfpAudioDisconnectRequest(hfp_primary_link);
            }

            /* Reconnect on link loss */
            if (ind->status == hfp_disconnect_link_loss && !appGetHfp()->bitfields.detach_pending)
            {
                Telephony_NotifyDisconnectedDueToLinkloss(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

                 /* Set disconnect reason */
                 appGetHfp()->bitfields.disconnect_reason = APP_HFP_DISCONNECT_LINKLOSS;
            }
            else
            {
                Telephony_NotifyDisconnected(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

                /* Set disconnect reason */
                appGetHfp()->bitfields.disconnect_reason = APP_HFP_DISCONNECT_NORMAL;
            }

            /* inform clients */
            appHfpSendSlcStatus(FALSE, hfp_primary_link, appHfpGetAgBdAddr());

            /* Move to disconnected state */
            appHfpSetState(HFP_STATE_DISCONNECTED);
        }
        break;

        case HFP_STATE_DISCONNECTING:
        case HFP_STATE_DISCONNECTED:
        {
            /* Ignore disconnect indication with status link-transferred */
            if(ind->status != hfp_disconnect_transferred)
            {
                Telephony_NotifyDisconnected(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));

                /* Set disconnect reason */
                appGetHfp()->bitfields.disconnect_reason = APP_HFP_DISCONNECT_NORMAL;

                /* Move to disconnected state */
                appHfpSetState(HFP_STATE_DISCONNECTED);
            }
        }
        break;

        default:
            appHfpError(HFP_SLC_DISCONNECT_IND, ind);
            return;
    }
}

static bool (*AcceptCallCallback)(void) = NULL;

void appHfpRegisterAcceptCallCallback(bool (*accept_call_callback)(void))
{
    AcceptCallCallback = accept_call_callback;
}

/*! \brief Handle SCO Audio connect indication
*/
static void appHfpHandleHfpAudioConnectIndication(const HFP_AUDIO_CONNECT_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpAudioConnectIndication state %d", appHfpGetState());
    const hfp_audio_params *hfp_sync_config_params = NULL;

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        {
            bool accept = TRUE;

            if(AcceptCallCallback)
            {
                accept = AcceptCallCallback();
            }

            if(accept)
            {
                /* Inform client tasks SCO is connecting */
                TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_SCO_CONNECTING_IND);

                /* Accept SCO connection */
                HfpAudioConnectResponse(ind->priority, TRUE, appGetHfp()->sco_supported_packets ^ sync_all_edr_esco, hfp_sync_config_params, FALSE);

                appGetHfp()->bitfields.esco_connecting = TRUE;


#ifdef INCLUDE_AV
                /* Suspend AV streaming */
                appAvStreamingSuspend(AV_SUSPEND_REASON_SCO);
#endif
                return;
            }
        }
        /* Fall through */

        default:
        {
            /* Reject SCO connection */
            HfpAudioConnectResponse(ind->priority, FALSE, 0, NULL, FALSE);
        }
        return;
    }
}

/*! \brief Handle SCO Audio connect confirmation
*/
static void appHfpHandleHfpAudioConnectConfirmation(const HFP_AUDIO_CONNECT_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpAudioConnectConfirmation(%d) state %d", cfm->status, appHfpGetState());

    appGetHfp()->bitfields.esco_connecting = FALSE;

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        {
            /* Check if audio connection was successful. */
            if (cfm->status == hfp_audio_connect_success)
            {
                voice_source_t source = HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(cfm->priority);

                /* Inform client tasks SCO is active */
                TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_SCO_CONNECTED_IND);

                /* Store sink associated with SCO */
                appGetHfp()->sco_sink = cfm->audio_sink;

                /* Check if SCO is now encrypted (or not) */
                appHfpCheckEncryptedSco();

                /* Update link policy now SCO is active */
                appLinkPolicyUpdatePowerTable(appHfpGetAgBdAddr());

#ifdef INCLUDE_AV
                /* Check if AG only supports HV1 SCO packets, if so then disconnect A2DP & AVRCP */
                if (appAvHasAConnection() && (appGetHfp()->sco_supported_packets == sync_hv1))
                {
                    /* Disconnect AV */
                    appAvDisconnectHandset();

                    /* Set flag for AV re-connect */
                    appGetHfp()->bitfields.sco_av_reconnect = TRUE;
                }
                else
                {
                    /* Clear flag for AV re-connect */
                    appGetHfp()->bitfields.sco_av_reconnect = FALSE;
                }
#endif
                HfpProfile_StoreConnectParams(cfm);

#if defined(INCLUDE_SCOFWD)
                ScoFwdHandleHfpAudioConnectConfirmation(source);
#elif defined(INCLUDE_MIRRORING)
                MirrorProfile_HandleHfpAudioConnectConfirmation(source);
#else
                Telephony_NotifyCallAudioConnected(source);
#endif

                /* Check if in HSP mode, use audio connection as indication of active call */
                if (appGetHfp()->profile == hfp_headset_profile)
                {
                    /* Move to active call state */
                    appHfpSetState(HFP_STATE_CONNECTED_ACTIVE);
                }

                /* Play SCO connected tone, only play if state is ConnectedIncoming,
                   ConnectedOutgoing or ConnectedActive and not voice recognition */
                if (appHfpIsCall() && !appHfpIsVoiceRecognitionActive())
                {
                    /* Set flag indicating we need UI tone when SCO disconnects */
                    appGetHfp()->bitfields.sco_ui_indication = TRUE;

                    Telephony_NotifyCallAudioRenderedLocal(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(cfm->priority));
                }
                else
                {
                    /* Clear flag indicating we don't need UI tone when SCO disconnects */
                    appGetHfp()->bitfields.sco_ui_indication = FALSE;
                }
            }
            else if (cfm->status == hfp_audio_connect_in_progress)
            {
                /* This can happen if we have asked to transfer the audio to this device
                   multiple times before the first HFP_AUDIO_CONNECT_CFM was received.

                   Do nothing here because eventually we should get the CFM for the
                   first request with another success or failure status. */
            }
            else
            {
#ifdef INCLUDE_AV
                /* Resume AV streaming */
                appAvStreamingResume(AV_SUSPEND_REASON_SCO);
#endif
            }
        }
        return;

        default:
            appHfpError(HFP_AUDIO_CONNECT_CFM, cfm);
            return;
    }
}

/*! \brief Handle SCO Audio disconnect indication
*/
static void appHfpHandleHfpAudioDisconnectIndication(const HFP_AUDIO_DISCONNECT_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpAudioDisconnectIndication(%d)", ind->status);

    appGetHfp()->bitfields.esco_connecting = FALSE;

    /* The SCO has been transferred to the secondary earbud. Ignore this message.
       The SLC disconnection will clean up the hfp state */
    if (ind->status == hfp_audio_disconnect_transferred)
    {
        return;
    }

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        case HFP_STATE_DISCONNECTED:
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        {
            /* Inform client tasks SCO is inactive */
            TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_SCO_DISCONNECTED_IND);

            /* Check if have SCO link */
            if (appGetHfp()->sco_sink)
            {
#ifdef INCLUDE_AV
                /* Check if we need to reconnect AV */
                if (appGetHfp()->bitfields.sco_av_reconnect)
                    appAvConnectHandset(FALSE);
#endif
                Telephony_NotifyCallAudioRenderedLocal(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(ind->priority));

                ScoFwdHandleHfpAudioDisconnectIndication(ind);

                /* Check if in HSP mode, if so then end the call */
                if (appGetHfp()->profile == hfp_headset_profile && appHfpIsConnected())
                {
                    /* Move to connected state */
                    appHfpSetState(HFP_STATE_CONNECTED_IDLE);
                }

                /* Clear SCO sink */
                appGetHfp()->sco_sink = 0;

                /* Clear any SCO unencrypted reminders */
                appHfpCheckEncryptedSco();

                /* Update link policy now SCO is inactive */
                appLinkPolicyUpdatePowerTable(appHfpGetAgBdAddr());

#ifdef INCLUDE_AV
                /* Resume AV streaming */
                appAvStreamingResume(AV_SUSPEND_REASON_SCO);
#endif
            }
        }
        return;

        default:
            appHfpError(HFP_AUDIO_DISCONNECT_IND, ind);
            return;
    }
}

/* TODO: Support for HFP encryption change ? */

/*! \brief Handle Ring indication
*/
static void appHfpHandleHfpRingIndication(const HFP_RING_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpRingIndication");

    /* TODO: Support multipoint */
    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        {
            /* Check if in HSP mode, use rings as indication of incoming call */
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Move to incoming call establishment */
                appHfpSetState(HFP_STATE_CONNECTED_INCOMING);

                /* Start HSP incoming call timeout */
                MessageSendLater(appGetHfpTask(), HFP_INTERNAL_HSP_INCOMING_TIMEOUT, 0, D_SEC(5));
            }

            /* Play ring tone if AG doesn't support in band ringing */
            if (!ind->in_band && !appGetHfp()->bitfields.call_accepted)
            {
                Telephony_NotifyCallIncomingOutOfBandRingtone(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(ind->priority));
            }
        }
        return;

        case HFP_STATE_CONNECTED_INCOMING:
        {
            /* Check if in HSP mode, use rings as indication of incoming call */
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Reset incoming call timeout */
                MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_HSP_INCOMING_TIMEOUT);
                MessageSendLater(appGetHfpTask(), HFP_INTERNAL_HSP_INCOMING_TIMEOUT, 0, D_SEC(5));
            }
        }
        /* Fallthrough */

        case HFP_STATE_CONNECTED_ACTIVE:
        {
            /* Play ring tone if AG doesn't support in band ringing */
            if (!ind->in_band && !appGetHfp()->bitfields.call_accepted)
            {
                Telephony_NotifyCallIncomingOutOfBandRingtone(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(ind->priority));
            }
        }
        return;

        case HFP_STATE_DISCONNECTING:
            return;

        default:
            appHfpError(HFP_RING_IND, ind);
            return;
    }
}

/*! \brief Handle service indication
*/
static void appHfpHandleHfpServiceIndication(const HFP_SERVICE_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpServiceIndication(%d)", ind->service);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            /* TODO: Handle service/no service */
        }
        return;

        default:
            appHfpError(HFP_SERVICE_IND, ind);
            return;
    }
}

/*! \brief Handle call state indication
*/
static void appHfpHandleHfpCallStateIndication(const HFP_CALL_STATE_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpCallStateIndication(%d)", ind->call_state);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_DISCONNECTING:
        {
            /* Store call setup indication */
            appGetHfp()->bitfields.call_state = ind->call_state;
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            hfpState state;

            /* Store call setup indication */
            appGetHfp()->bitfields.call_state = ind->call_state;

            /* Move to new state, depending on call state */
            state = hfp_call_state_table[appGetHfp()->bitfields.call_state];
            if (appHfpGetState() != state)
                appHfpSetState(state);
        }
        return;

        default:
            appHfpError(HFP_CALL_STATE_IND, ind);
            return;
    }
}

/*! \brief Handle voice recognition indication
*/
static void appHfpHandleHfpVoiceRecognitionIndication(const HFP_VOICE_RECOGNITION_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpVoiceRecognitionIndication(%d)", ind->enable);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            appGetHfp()->bitfields.voice_recognition_active = ind->enable;

#if defined(INCLUDE_AV) && defined(SUSPEND_ON_VOICE_RECOGNITION)
            if (appHfpIsVoiceRecognitionActive())
                appAvStreamingSuspend(AV_SUSPEND_REASON_HFP);
            else
                appAvStreamingResume(AV_SUSPEND_REASON_HFP);
#endif
        }
        return;

        default:
            appHfpError(HFP_VOICE_RECOGNITION_IND, ind);
            return;
    }
}

/*! \brief Handle voice recognition enable confirmation
*/
static void appHfpHandleHfpVoiceRecognitionEnableConfirmation(const HFP_VOICE_RECOGNITION_ENABLE_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpVoiceRecognitionEnableConfirmation(%d)", cfm->status);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            if (cfm->status == hfp_success)
                appGetHfp()->bitfields.voice_recognition_active = appGetHfp()->bitfields.voice_recognition_request;
            else
                appGetHfp()->bitfields.voice_recognition_request = appGetHfp()->bitfields.voice_recognition_active;

#if defined(INCLUDE_AV) && defined(SUSPEND_ON_VOICE_RECOGNITION)
            if (appHfpIsVoiceRecognitionActive())
                appAvStreamingSuspend(AV_SUSPEND_REASON_HFP);
            else
                appAvStreamingResume(AV_SUSPEND_REASON_HFP);
#endif
        }
        return;

        default:
            appHfpError(HFP_VOICE_RECOGNITION_ENABLE_CFM, cfm);
            return;
    }
}

/*! \brief Handle caller ID indication
*/
static void appHfpHandleHfpCallerIdIndication(const HFP_CALLER_ID_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpCallerIdIndication");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_INCOMING:
        {
            /* Check we haven't already accepted the call */
            if (!appGetHfp()->bitfields.call_accepted)
            {
                /* Queue prompt & number
                 * This was a todo on playing voice prompts to announce the caller id from text to speech */
            }
        }
        return;

        case HFP_STATE_DISCONNECTING:
            return;

        default:
            appHfpError(HFP_CALLER_ID_IND, ind);
            return;
    }
}

/*! \brief Handle caller ID enable confirmation
*/
static void appHfpHandleHfpCallerIdEnableConfirmation(const HFP_CALLER_ID_ENABLE_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpCallerIdEnableConfirmation(%d)", cfm->status);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            if (cfm->status == hfp_success)
                appGetHfp()->bitfields.caller_id_active = TRUE;
        }
        return;

        default:
            appHfpError(HFP_CALLER_ID_ENABLE_CFM, cfm);
            return;
    }
}

/*! \brief Notify all registered clients of new HFP volume. */
void appHfpVolumeNotifyClients(uint8 new_volume)
{
    MAKE_HFP_MESSAGE(APP_HFP_VOLUME_IND);
    message->volume = new_volume;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), APP_HFP_VOLUME_IND, message);
}

/*! \brief Handle volume indication
*/
static void appHfpHandleHfpVolumeSyncSpeakerGainIndication(const HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpVolumeSyncSpeakerGainIndication(%d)", ind->volume_gain);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            Volume_SendVoiceSourceVolumeUpdateRequest(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(ind->priority), event_origin_external, ind->volume_gain);
        }
        return;

        default:
            appHfpError(HFP_VOLUME_SYNC_SPEAKER_GAIN_IND, ind);
            return;
    }
}

/*! \brief Handle microphone volume indication
*/
static void appHfpHandleHfpVolumeSyncMicrophoneGainIndication(const HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpVolumeSyncMicrophoneGainIndication(%d)", ind->mic_gain);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            /* Set input gain */
            appGetHfp()->mic_volume = ind->mic_gain;

            /* Store new configuration */
            appHfpConfigStore();
        }
        return;

        default:
            appHfpError(HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND, ind);
            return;
    }
}

/*! \brief Handle answer call confirmation
*/
static void appHfpHandleHfpCallAnswerConfirmation(const HFP_CALL_ANSWER_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpCallAnswerConfirmation");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_INCOMING:
        {
            if (cfm->status == hfp_success)
            {
#ifdef INCLUDE_AV
                /* Tell main task we should suspend AV streaming */
                appAvStreamingSuspend(AV_SUSPEND_REASON_HFP);
#endif
                /* Flag call as accepted, so we ignore any ring indications or caller ID */
                appGetHfp()->bitfields.call_accepted = TRUE;

                /* TODO: Cancel any ring-tones */
                /* AudioStopTone();*/
            }
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
            return;

        default:
            appHfpError(HFP_CALL_ANSWER_CFM, cfm);
            return;
    }
}

/*! \brief Handle terminate call confirmation
*/
static void appHfpHandleHfpCallTerminateConfirmation(const HFP_CALL_TERMINATE_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleHfpCallTerminateConfirmation");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
            return;

        default:
            appHfpError(HFP_CALL_TERMINATE_CFM, cfm);
            return;
    }
}

/*! \brief Handle unrecognised AT commands as TWS+ custom commands.
 */
static void appHfpHandleHfpUnrecognisedAtCmdInd(HFP_UNRECOGNISED_AT_CMD_IND_T* ind)
{
    hfpTaskData* hfp = appGetHfp();

    DEBUG_LOG("appHfpHandleHfpUnrecognisedAtCmdInd");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            /* copy the message and send to register AT client */
            MAKE_HFP_MESSAGE_WITH_LEN(APP_HFP_AT_CMD_IND, ind->size_data);
            message->priority = ind->priority;
            message->size_data = ind->size_data;
            memcpy(message->data, ind->data, ind->size_data);
            MessageSend(hfp->at_cmd_task, APP_HFP_AT_CMD_IND, message);
        }
            return;

        default:
        {
            uint16 i;
            for(i = 0; i < ind->size_data; ++i)
            {
                DEBUG_LOG("0x%x %c", ind->data[i], ind->data[i]);
            }
            appHfpError(HFP_UNRECOGNISED_AT_CMD_IND, ind);
            return;
        }
    }

}


static void appHfpHandleHfpHfIndicatorsReportInd(const HFP_HF_INDICATORS_REPORT_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpHfIndicatorsReportInd, num %u, mask %04x", ind->num_hf_indicators, ind->hf_indicators_mask);
}

static void appHfpHandleHfpHfIndicatorsInd(const HFP_HF_INDICATORS_IND_T *ind)
{
    DEBUG_LOG("appHfpHandleHfpHfIndicatorsInd, num %u, status %u", ind->hf_indicator_assigned_num, ind->hf_indicator_status);
    if (ind->hf_indicator_assigned_num == hf_battery_level)
    {
        if (ind->hf_indicator_status)
        {
            hfpTaskData *hfp = appGetHfp();

            hfp->bitfields.hf_indicator_assigned_num = hf_battery_level;
            appBatteryRegister(&hfp->battery_form);
            HfpBievIndStatusRequest(hfp_primary_link, hf_battery_level, appBatteryGetPercent());
        }
        else
        {
            appBatteryUnregister(appGetHfpTask());
        }
    }
}


static void appHfpHandleBatteryLevelUpdatePercent(MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T *msg)
{
    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
            HfpBievIndStatusRequest(hfp_primary_link, hf_battery_level, msg->percent);
            break;

        default:
            break;
    }
}


/*! \brief Send handset signalling AT command to handset.

    \param  priority    Which HFP link to send the command over
    \param  cmd         The NULL terminated AT command to send
*/
void appHfpSendAtCmdReq(hfp_link_priority priority, char* cmd)
{
    HfpAtCmdRequest(priority, cmd);
}

/*! \brief Handle confirmation result of attempt to send AT command to handset. */
static void appHfpHandleHfpAtCmdConfirm(HFP_AT_CMD_CFM_T *cfm)
{
    hfpTaskData* hfp = appGetHfp();
    DEBUG_LOG("appHfpHandleHfpAtCmdConfirm %d", cfm->status);
    MAKE_HFP_MESSAGE(APP_HFP_AT_CMD_CFM);
    message->status = cfm->status == hfp_success ? TRUE : FALSE;
    MessageSend(hfp->at_cmd_task, APP_HFP_AT_CMD_CFM, message);
}

/*! \brief Handle remote support features confirmation
*/
static void appHfpHandleClDmRemoteFeaturesConfirm(const CL_DM_REMOTE_FEATURES_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleClDmRemoteFeaturesConfirm");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        case HFP_STATE_DISCONNECTED:
        {
            if (cfm->status == hci_success)
            {
                uint16 features[PSKEY_LOCAL_SUPPORTED_FEATURES_SIZE] = PSKEY_LOCAL_SUPPORTED_FEATURES_DEFAULTS;
                uint16 packets;
                uint16 index;

                /* Read local supported features to determine SCO packet types */
                PsFullRetrieve(PSKEY_LOCAL_SUPPORTED_FEATURES, &features, PSKEY_LOCAL_SUPPORTED_FEATURES_SIZE);

                /* Get supported features that both HS & AG support */
                for (index = 0; index < PSKEY_LOCAL_SUPPORTED_FEATURES_SIZE; index++)
                {
                    printf("%04x ", features[index]);
                    features[index] &= cfm->features[index];
                }
                printf("");

                /* Calculate SCO packets we should use */
                packets = sync_hv1;
                if (features[0] & 0x2000)
                    packets |= sync_hv3;
                if (features[0] & 0x1000)
                    packets |= sync_hv2;

                /* Only use eSCO for HFP 1.5 */
                if (appGetHfp()->profile == hfp_handsfree_107_profile)
                {
                    if (features[1] & 0x8000)
                        packets |= sync_ev3;
                    if (features[2] & 0x0001)
                        packets |= sync_ev4;
                    if (features[2] & 0x0002)
                        packets |= sync_ev5;
                    if (features[2] & 0x2000)
                    {
                        packets |= sync_2ev3;
                        if (features[2] & 0x8000)
                            packets |= sync_2ev5;
                    }
                    if (features[2] & 0x4000)
                    {
                        packets |= sync_3ev3;
                        if (features[2] & 0x8000)
                            packets |= sync_3ev5;
                    }
                }

                /* Update supported SCO packet types */
                appGetHfp()->sco_supported_packets = packets;

                DEBUG_LOG("appHfpHandleClDmRemoteFeaturesConfirm, SCO packets %x", packets);
            }
        }
        return;

        default:
            appHfpError(CL_DM_REMOTE_FEATURES_CFM, cfm);
            return;
    }
}

/*! \brief Handle encrypt confirmation
*/
static void appHfpHandleClDmEncryptConfirmation(const CL_SM_ENCRYPT_CFM_T *cfm)
{
    DEBUG_LOG("appHfpHandleClDmEncryptConfirmation(%d)", cfm->encrypted);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTING:
        {
            /* Store encrypted status */
            appGetHfp()->bitfields.encrypted = cfm->encrypted;

            /* Check if SCO is now encrypted (or not) */
            appHfpCheckEncryptedSco();
        }
        return;

        default:
            appHfpError(CL_SM_ENCRYPT_CFM, cfm);
            return;
    }
}

/*! \brief Handle connect HFP SLC request
*/
static void appHfpHandleInternalHfpConnectRequest(const HFP_INTERNAL_HFP_CONNECT_REQ_T *req)
{
    DEBUG_LOG("appHfpHandleInternalHfpConnectRequest, %x,%x,%lx",
               req->addr.nap, req->addr.uap, req->addr.lap);

    switch (appHfpGetState())
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Check ACL is connected */
            if (ConManagerIsConnected(&req->addr))
            {
                /* Store connection flags */
                appGetHfp()->bitfields.flags = req->flags;

                /* Store AG Bluetooth Address and profile type */
                appGetHfp()->ag_bd_addr = req->addr;
                appGetHfp()->profile = req->profile;

                /* Move to connecting local state */
                appHfpSetState(HFP_STATE_CONNECTING_LOCAL);
            }
            else
            {
                DEBUG_LOG("appHfpHandleInternalHfpConnectRequest, no ACL %x,%x,%lx",
                           req->addr.nap, req->addr.uap, req->addr.lap);

                /* Set disconnect reason */
                appGetHfp()->bitfields.disconnect_reason = APP_HFP_CONNECT_FAILED;

                /* Move to 'disconnected' state */
                appHfpSetState(HFP_STATE_DISCONNECTED);
            }
        }
        return;

        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTING_LOCAL:
            return;

        case HFP_STATE_DISCONNECTING:
        {
            MAKE_HFP_MESSAGE(HFP_INTERNAL_HFP_CONNECT_REQ);

            /* repost the connect message pending final disconnection of the profile
             * via the lock */
            message->addr = req->addr;
            message->profile = req->profile;
            message->flags = req->flags;
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_CONNECT_REQ, message,
                                     &appHfpGetLock());
        }
        return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CONNECT_REQ, req);
            return;
    }
}

/*! \brief Handle disconnect HFP SLC request
*/
static void appHfpHandleInternalHfpDisconnectRequest(const HFP_INTERNAL_HFP_DISCONNECT_REQ_T *req)
{
    DEBUG_LOG("appHfpHandleInternalHfpDisconnectRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            /* Move to disconnecting state */
            appHfpSetState(HFP_STATE_DISCONNECTING);
        }
        return;

        case HFP_STATE_DISCONNECTED:
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_DISCONNECT_REQ, req);
            return;
    }
}

/*! \brief Handle last number redial request
*/
static void appHfpHandleInternalHfpLastNumberRedialRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpLastNumberRedialRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send button press */
                /* TODO: Support Multilink */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                /* Request last number redial */
                /* TODO: Support Mulitilink */
                HfpDialLastNumberRequest(hfp_primary_link);
            }
        }
        return;

        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore last number redial request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_LAST_NUMBER_REDIAL_REQ, NULL);
            return;
    }
}

/*! \brief Handle voice dial request
*/
static void appHfpHandleInternalHfpVoiceDialRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpVoiceDialRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send button press */
                /* TODO: Support Multilink */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                /* Send the CMD to the AG */
                /* TODO: Support Multipoint */
                HfpVoiceRecognitionEnableRequest(hfp_primary_link, appGetHfp()->bitfields.voice_recognition_request = TRUE);
            }
        }
        return;

        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore voice dial request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_VOICE_DIAL_REQ, NULL);
            return;
    }
}

/*! \brief Handle voice dial disable request
*/
static void appHfpHandleInternalHfpVoiceDialDisableRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpVoiceDialDisableRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send button press */
                /* TODO: Support Multilink */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                /* Send the CMD to the AG */
                /* TODO: Support Multipoint */
                HfpVoiceRecognitionEnableRequest(hfp_primary_link, appGetHfp()->bitfields.voice_recognition_request = FALSE);
            }
        }
        return;

        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore voice dial request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_VOICE_DIAL_DISABLE_REQ, NULL);
            return;
    }
}

static void appHfpHandleInternalNumberDialRequest(HFP_INTERNAL_NUMBER_DIAL_REQ_T * message)
{
    DEBUG_LOG("appHfpHandleInternalNumberDialRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_IDLE:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send button press */
                /* TODO: Support Multilink */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                HfpDialNumberRequest(hfp_primary_link, message->length, message->number);
            }
        }
        return;

        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore number dial request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_NUMBER_DIAL_REQ, NULL);
            return;
    }
}

/*! \brief Handle accept call request
*/
static void appHfpHandleInternalHfpCallAcceptRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpCallAcceptRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_INCOMING:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send button press */
                /* TODO: Support Multilink */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                /* Answer the incoming call */
                /* TODO: Support Multipoint */
                HfpCallAnswerRequest(hfp_primary_link, TRUE);
            }
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore call accept request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CALL_ACCEPT_REQ, NULL);
            return;
    }
}

/*! \brief Handle reject call request
*/
static void appHfpHandleInternalHfpCallRejectRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpCallRejectRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_INCOMING:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                Telephony_NotifyError(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
            }
            else
            {
                /* Reject the incoming call */
                /* TODO: Support Multipoint */
                HfpCallAnswerRequest(hfp_primary_link, FALSE);
            }
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore call accept request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CALL_REJECT_REQ, NULL);
            return;
    }
}

/*! \brief Handle hangup call request
*/
static void appHfpHandleInternalHfpCallHangupRequest(void)
{
    DEBUG_LOG("appHfpHandleInternalHfpCallHangupRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_CONNECTED_OUTGOING:
        {
            if (appGetHfp()->profile == hfp_headset_profile)
            {
                /* Send an HSP button press */
                /* TODO: Support Multipoint */
                HfpHsButtonPressRequest(hfp_primary_link);
            }
            else
            {
                /* Terminate the call */
                /* TODO: Support Multiponit */
                HfpCallTerminateRequest(hfp_primary_link);
            }
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_DISCONNECTED:
            /* Ignore call accept request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CALL_HANGUP_REQ, NULL);
            return;
    }
}

/*! \brief Handle mute/unmute request
*/
static void appHfpHandleInternalHfpMuteRequest(const HFP_INTERNAL_HFP_MUTE_REQ_T *req)
{
    DEBUG_LOG("appHfpHandleInternalHfpMuteRequest");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            if (req->mute)
            {
                Telephony_NotifyMicrophoneMuted(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
            }
            else
            {
                Telephony_NotifyMicrophoneUnmuted(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
            }

            /* Set mute flag */
            appGetHfp()->bitfields.mute_active = req->mute;

            /* Re-configure audio chain */
            appKymeraScoMicMute(req->mute);
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        case HFP_STATE_DISCONNECTED:
            /* Ignore call accept request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CALL_HANGUP_REQ, NULL);
            return;
    }
}

/*! \brief Handle audio transfer request
*/
static void appHfpHandleInternalHfpTransferRequest(const HFP_INTERNAL_HFP_TRANSFER_REQ_T *req)
{
    DEBUG_LOG("appHfpHandleInternalHfpTransferRequest state %u transfer_to_ag %d", appHfpGetState(), req->transfer_to_ag);

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_ACTIVE:
        case HFP_STATE_CONNECTED_INCOMING:
        case HFP_STATE_CONNECTED_OUTGOING:
        {
            /* Attempt to transfer audio */
            /* TODO: Support Multipoint */
            HfpAudioTransferRequest(hfp_primary_link,
                                    req->transfer_to_ag ? hfp_audio_to_ag : hfp_audio_to_hfp,
                                    appGetHfp()->sco_supported_packets  ^ sync_all_edr_esco,
                                    NULL);
        }
        return;

        case HFP_STATE_CONNECTED_IDLE:
        case HFP_STATE_DISCONNECTED:
            /* Ignore call accept request as it doesn't make sense in these states */
            return;

        default:
            appHfpError(HFP_INTERNAL_HFP_CALL_HANGUP_REQ, NULL);
            return;
    }
}

/*! \brief Handle config write request

    This functions is called to write the current HFP configuration
    to persistent store.
*/
void appHfpHandleInternalConfigWriteRequest(void)
{
    hfpPsConfigData ps_config;

    DEBUG_LOG("appHfpHandleInternalConfigWriteRequest");

    /* Initialise PS config structure */
    ps_config.volume = appGetHfp()->volume;
    ps_config.mic_volume = appGetHfp()->mic_volume;

    /* Write to persistent store */
    PsStore(PS_HFP_CONFIG, &ps_config, sizeof(ps_config));
}

/*! \brief Handle HSP incoming call timeout

    We have had a ring indication for a while, so move back to 'connected
    idle' state.
*/
static void appHfpHandleHfpHspIncomingTimeout(void)
{
    DEBUG_LOG("appHfpHandleHfpHspIncomingTimeout");

    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_INCOMING:
        {
            /* Move back to connected idle state */
            appHfpSetState(HFP_STATE_CONNECTED_IDLE);
        }
        return;

        default:
            appHfpError(HFP_INTERNAL_HSP_INCOMING_TIMEOUT, NULL);
            return;
    }
}

/*! \brief Handle indication of change in a connection status.

    Some phones will disconnect the ACL without closing any L2CAP/RFCOMM
    connections, so we check the ACL close reason code to determine if this
    has happened.

    If the close reason code was not link-loss and we have an HFP profile
    on that link, mark it as detach pending, so that we can gracefully handle
    the L2CAP or RFCOMM disconnection that will follow shortly.
 */
static void appHfpHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *ind)
{
    /* if disconnection and not an connection timeout, see if we need to mark
     * the HFP profile at having a pending detach */
    if (!ind->connected && ind->reason != hci_error_conn_timeout)
    {
        if (!appHfpIsDisconnected() && BdaddrIsSame(&ind->bd_addr, &appGetHfp()->ag_bd_addr) && !ind->ble)
        {
            DEBUG_LOG("appHfpHandleConManagerConnectionInd, detach pending");
            appGetHfp()->bitfields.detach_pending = TRUE;
        }
    }
}

static void initHfpVolume(voice_source_t source, uint8 init_volume)
{
    volume_t volume;
    VoiceSources_RegisterVolume(source, HfpProfile_GetVoiceSourceVolumeInterface());
    VoiceSources_RegisterObserver(source, HfpProfile_GetVoiceSourceObserverInterface());
    volume = VoiceSources_GetVolume(source);
    volume.value = init_volume;
    VoiceSources_SetVolume(source, volume);
}

/*! \brief Initialise HFP instance

    This function initialises the HFP instance, all state variables are
    set to defaults and volume defaults read from persistent store.
*/
bool appHfpInit(Task init_task)
{
    hfpPsConfigData ps_config;

    /* Set up task handler */
    appGetHfp()->task.handler = appHfpHandleMessage;

    /* Set defaults */
    ps_config.volume = HFP_SPEAKER_GAIN;
    ps_config.mic_volume = HFP_MICROPHONE_GAIN;

    /* Read config from persistent store */
    PsRetrieve(PS_HFP_CONFIG, &ps_config, sizeof(ps_config));

    VoiceSources_RegisterAudioInterface(voice_source_hfp_1, HfpProfile_GetAudioInterface());
    initHfpVolume(voice_source_hfp_1, ps_config.volume);
    VoiceSources_RegisterTelephonyControlInterface(voice_source_hfp_1, HfpProfile_GetTelephonyControlInterface());

    /* Update configuration */
    appGetHfp()->mic_volume = ps_config.mic_volume;

    /* Store default SCO packet types */
    appGetHfp()->sco_supported_packets = sync_all_sco;

    /* create list for SLC notification clients */
    TaskList_InitialiseWithCapacity(appHfpGetSlcStatusNotifyList(), HFP_SLC_STATUS_NOTIFY_LIST_INIT_CAPACITY);

    /* create list for general status notification clients */
    TaskList_InitialiseWithCapacity(appHfpGetStatusNotifyList(), HFP_STATUS_NOTIFY_LIST_INIT_CAPACITY);

    /* create lists for connection/disconnection requests */
    TaskList_WithDataInitialise(&appGetHfp()->connect_request_clients);
    TaskList_WithDataInitialise(&appGetHfp()->disconnect_request_clients);

    /* Initialise state */
    appGetHfp()->state = HFP_STATE_NULL;
    appGetHfp()->sco_sink = 0;
    appGetHfp()->hfp_lock = 0;
    appGetHfp()->bitfields.disconnect_reason = APP_HFP_CONNECT_FAILED;
    appGetHfp()->bitfields.hf_indicator_assigned_num = hf_indicators_invalid;
    appGetHfp()->battery_form.task = appGetHfpTask();
    appGetHfp()->battery_form.hysteresis = 1;
    appGetHfp()->battery_form.representation = battery_level_repres_percent;
    appGetHfp()->codec = hfp_wbs_codec_mask_none;
    appGetHfp()->wesco = 0;
    appGetHfp()->tesco = 0;
    appGetHfp()->qce_codec_mode_id = CODEC_MODE_ID_UNSUPPORTED;

    appHfpSetState(HFP_STATE_INITIALISING_HFP);

    /* Register to receive notifications of (dis)connections */
    ConManagerRegisterConnectionsClient(appGetHfpTask());

    Init_SetInitTask(init_task);

    /* Register hfp  task call back as ui provider*/
    Ui_RegisterUiProvider(ui_provider_hfp, hfpProfile_GetCurrentContext);

    Ui_RegisterUiInputConsumer(appGetHfpTask(), hfp_ui_inputs, ARRAY_DIM(hfp_ui_inputs));

    ProfileManager_RegisterProfile(profile_manager_hfp_profile, hfpProfile_Connect, hfpProfile_Disconnect);

    return TRUE;
}

/*! \brief Initiate HFP connection to default

    Attempt to connect to the previously connected HFP AG.

    \return TRUE if a connection was requested. FALSE is returned
        in the case of an error such as HFP not being supported by
        the handset or there already being an HFP connection. The
        error will apply even if the existing HFP connection is
        to the requested handset.
*/
bool appHfpConnectHandset(void)
{
    bdaddr bd_addr;

    /* Get handset device address */
    if (appDeviceGetHandsetBdAddr(&bd_addr) && BtDevice_IsProfileSupported(&bd_addr, DEVICE_PROFILE_HFP))
    {
        device_t device = BtDevice_GetDeviceForBdAddr(&bd_addr);
        if (device)
        {
            uint8 our_hfp_profile = 0;
            Device_GetPropertyU8(device, device_property_hfp_profile, &our_hfp_profile);
            return appHfpConnectWithBdAddr(&bd_addr, our_hfp_profile);
        }
    }

    return FALSE;
}

void hfpProfile_Connect(const Task client_task, bdaddr *bd_addr)
{
    PanicNull((bdaddr *)bd_addr);
    if (BtDevice_IsProfileSupported(bd_addr, DEVICE_PROFILE_HFP))
    {
        device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
        if (device)
        {
            MessageCancelAll(&appGetHfp()->task, HFP_INTERNAL_HFP_DISCONNECT_REQ);

            uint8 our_hfp_profile = 0;
            Device_GetPropertyU8(device, device_property_hfp_profile, &our_hfp_profile);

            ProfileManager_AddRequestToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->connect_request_clients), device, client_task);
            if (!appHfpConnectWithBdAddr(bd_addr, our_hfp_profile))
            {
                /* If already connected, send an immediate confirmation */
                ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->connect_request_clients),
                                                        bd_addr, profile_manager_success, hfpProfile_FindClientSendConnectCfm);
            }
        }
    }
}

void hfpProfile_Disconnect(const Task client_task, bdaddr *bd_addr)
{
    PanicNull((bdaddr *)bd_addr);
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        ProfileManager_AddRequestToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->disconnect_request_clients), device, client_task);
        if (!appHfpDisconnectInternal())
        {
            /* If already disconnected, send an immediate confirmation */
            ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&appGetHfp()->disconnect_request_clients),
                                                    bd_addr, profile_manager_success, hfpProfile_FindClientSendDisconnectCfm);
        }
    }
}

/*! \brief Initiate HFP connection to device

    Attempt to connect to the specified HFP AG.

    \param  bd_addr Bluetooth address of the HFP AG to connect to
    \param  profile The version of hfp profile to use when connectinhg

    \return TRUE if a connection was requested. FALSE is returned
        if there is already an HFP connection. The error will apply
        even if the existing HFP connection is to the requested handset.

*/
bool appHfpConnectWithBdAddr(const bdaddr *bd_addr, hfp_profile profile)
{
    DEBUG_LOG("appHfpConnectWithBdAddr");

    /* Check if not already connected */
    if (!appHfpIsConnected())
    {
        /* Store address of AG */
        appGetHfp()->ag_bd_addr = *bd_addr;
        
        MAKE_HFP_MESSAGE(HFP_INTERNAL_HFP_CONNECT_REQ);

        /* Send message to HFP task */
        message->addr = *bd_addr;
        message->profile = profile;
        message->flags = 0;
        MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_CONNECT_REQ, message,
                                 ConManagerCreateAcl(bd_addr));

        /* Connect will now be handled by HFP task */
        return TRUE;
    }

    /* Error occured */
    return FALSE;
}

/*! \brief Initiate HFP disconnect
*/
bool appHfpDisconnectInternal(void)
{
    DEBUG_LOG("appHfpDisconnect");
    bool disconnect_request_sent = FALSE;
    if (!appHfpIsDisconnected())
    {
        MAKE_HFP_MESSAGE(HFP_INTERNAL_HFP_DISCONNECT_REQ);

        /* Send message to HFP task */
        message->silent = FALSE;
        MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_DISCONNECT_REQ,
                                 message, &appHfpGetLock());
        disconnect_request_sent = TRUE;
    }
    return disconnect_request_sent;
}

/*! \brief Attempt last number redial

    Initiate last number redial, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallLastDialed(void)
{
    DEBUG_LOG("appHfpCallLastDialed");

    switch (appHfpGetState())
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                Telephony_NotifyError(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
                break;
            }
        }
        /* Fall through */

        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        {
            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_LAST_NUMBER_REDIAL_REQ,
                                     NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

/*! \brief Attempt voice dial

    Initiate voice dial, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallVoice(void)
{
    DEBUG_LOG("appHfpCallVoice");
    VoiceSources_InitiateVoiceDial(voice_source_hfp_1);
}

/*! \brief Disable voice dial

    Disable voice dial, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallVoiceDisable(void)
{
    DEBUG_LOG("appHfpCallVoiceDisable");

    switch (appHfpGetState())
    {
        case HFP_STATE_DISCONNECTED:
        {
            /* Connect SLC */
            if (!appHfpConnectHandset())
            {
                /* Play error tone to indicate we don't have a valid address */
                Telephony_NotifyError(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_primary_link));
                break;
            }
        }
        /* Fall through */

        case HFP_STATE_CONNECTING_LOCAL:
        case HFP_STATE_CONNECTING_REMOTE:
        case HFP_STATE_CONNECTED_IDLE:
        {
            /* Send message into HFP state machine */
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_VOICE_DIAL_DISABLE_REQ,
                                     NULL, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

/*! \brief Accept call

    Accept incoming call, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallAccept(void)
{
    DEBUG_LOG("appHfpCallAccept");

    VoiceSources_AcceptIncomingCall(voice_source_hfp_1);
}

/*! \brief Reject call

    Reject incoming call, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallReject(void)
{
    DEBUG_LOG("appHfpCallReject");

    VoiceSources_RejectIncomingCall(voice_source_hfp_1);
}

/*! \brief Hangup call

    Hangup active call, attempt to connect SLC first if not currently
    connected.
*/
void appHfpCallHangup(void)
{
    DEBUG_LOG("appHfpCallHangup");

    VoiceSources_TerminateOngoingCall(voice_source_hfp_1);
}

/*! \brief Store AV configuration

    This function is called to store the current HFP configuration.

    The configuration isn't store immediately, instead a timer is started, any
    currently running timer is cancel.  On timer expiration the configuration
    is written to Persistent Store, (see \ref appHfpHandleInternalConfigWriteRequest).
    This is to avoid multiple writes when the user adjusts the playback volume.
*/
void appHfpConfigStore(void)
{
    /* Cancel any pending messages */
    MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_CONFIG_WRITE_REQ);

    /* Store configuration after a 5 seconds */
    MessageSendLater(appGetHfpTask(), HFP_INTERNAL_CONFIG_WRITE_REQ, 0, D_SEC(5));
}

/*! \brief Make volume change
*/
static bool appHfpVolumeChange(int16 step)
{
    int new_volume = VolumeUtils_LimitVolumeToRange((VoiceSources_GetVolume(voice_source_hfp_1).value + step),
                                                            VoiceSources_GetVolume(voice_source_hfp_1).config.range);
    DEBUG_LOG("appHfpVolumeChange");
    Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_hfp_1, event_origin_local, new_volume);

    /* Return indicating volume changed successfully */
    return TRUE;
}

/*! \brief Continue volume change
*/
static bool appHfpVolumeRepeat(int16 step)
{
    DEBUG_LOG("appHfpVolumeRepeat(%d)", step);

    if (appHfpIsConnected())
    {
        /* Handle volume change locally */
        if (appHfpVolumeChange(step))
        {
            /* Send repeat message later */
            MessageSendLater(appGetHfpTask(), step > 0 ? HFP_INTERNAL_VOLUME_UP : HFP_INTERNAL_VOLUME_DOWN, NULL, 300);
            appGetHfp()->bitfields.volume_repeat = 1;

            /* Return indicating volume changed */
            return TRUE;
        }
    }

    /* Return indicating volume not changed */
    return FALSE;
}

/*! \brief Start volume change

    Start a repeating volume change

    \param  step    change to be applied to volume, +ve or -ve
*/
void appHfpVolumeStart(int16 step)
{
    DEBUG_LOG("appHfpVolumeStart(%d)", step);

    if (appHfpVolumeRepeat(step))
    {
        appGetHfp()->bitfields.volume_repeat = 0;
    }
}

/*! \brief Stop volume change

    Cancel a repeating volume change.

    \todo We don't need to supply the step for our HFP implementation of
    repeating volume. Remove the parameter ?

    \param  step    The volume step that was being used
*/
void appHfpVolumeStop(int16 step)
{
    UNUSED(step);
    DEBUG_LOG("appHfpVolumeStop(%d)", step);

    /* Cancel any pending volume messages, play tone if message actually cancelled */
    MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_VOLUME_UP);
    MessageCancelFirst(appGetHfpTask(), HFP_INTERNAL_VOLUME_DOWN);
}

/*! \brief Toggle mute
*/
void appHfpMuteToggle(void)
{
    DEBUG_LOG("appHfpMuteToggle");
    switch (appHfpGetState())
    {
        case HFP_STATE_CONNECTED_ACTIVE:
        {
            MAKE_HFP_MESSAGE(HFP_INTERNAL_HFP_MUTE_REQ);

            /* Send message into HFP state machine */
            message->mute = !appHfpIsMuted();
            MessageSendConditionally(appGetHfpTask(), HFP_INTERNAL_HFP_MUTE_REQ,
                                     message, &appHfpGetLock());
        }
        break;

        default:
            break;
    }
}

/*! \brief Transfer call to headset

    Transfer call to headset, attempt to connect SLC first if not currently
    connected.
*/
void appHfpTransferToHeadset(void)
{
    DEBUG_LOG("appHfpTransferToHeadset");

    VoiceSources_TransferOngoingCallAudioToSelf(voice_source_hfp_1);
}

/*! \brief Transfer call to AG

    Transfer call to AG, attempt to connect SLC first if not currently
    connected.
*/
void appHfpTransferToAg(void)
{
    DEBUG_LOG("appHfpTransferToAg");

    VoiceSources_TransferOngoingCallAudioToAg(voice_source_hfp_1);
}

/*! \brief Register with HFP to receive notifications of SLC connect/disconnect.

    \param  task    The task being registered to receive notifications.
 */
void appHfpClientRegister(Task task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(appHfpGetSlcStatusNotifyList()), task);
}

/*! \brief Register with HFP to receive notifications of state changes.

    \param  task    The task being registered to receive notifications.
 */
void appHfpStatusClientRegister(Task task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(appHfpGetStatusNotifyList()), task);
}

/*! \brief Register task to handle custom AT commands.

    \param  task    The task being registered to receive the commands
 */
void appHfpRegisterAtCmdTask(Task task)
{
    hfpTaskData* hfp = appGetHfp();
    hfp->at_cmd_task = task;
}

uint8 appHfpGetVolume(void)
{
    return appGetHfp()->volume;
}

/*! \brief Message Handler

    This function is the main message handler for the HFP instance, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
static void appHfpHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("appHfpHandleMessage id 0x%x, is message UI input %d", id, isMessageUiInput(id));

    if (isMessageUiInput(id))
    {
        appHfpHandleUiInput(id);
        return;
    }

    /* Handle internal messages */
    switch (id)
    {
        case HFP_INTERNAL_CONFIG_WRITE_REQ:
            appHfpHandleInternalConfigWriteRequest();
            return;

        case HFP_INTERNAL_SCO_UNENCRYPTED_IND:
            appHfpCheckEncryptedSco();
            return;

        case HFP_INTERNAL_HSP_INCOMING_TIMEOUT:
            appHfpHandleHfpHspIncomingTimeout();
            return;

        case HFP_INTERNAL_HFP_CONNECT_REQ:
            appHfpHandleInternalHfpConnectRequest((HFP_INTERNAL_HFP_CONNECT_REQ_T *)message);
            return;

        case HFP_INTERNAL_HFP_DISCONNECT_REQ:
            appHfpHandleInternalHfpDisconnectRequest((HFP_INTERNAL_HFP_DISCONNECT_REQ_T *)message);
            return;

        case HFP_INTERNAL_HFP_LAST_NUMBER_REDIAL_REQ:
            appHfpHandleInternalHfpLastNumberRedialRequest();
            return;

        case HFP_INTERNAL_HFP_VOICE_DIAL_REQ:
            appHfpHandleInternalHfpVoiceDialRequest();
            return;

        case HFP_INTERNAL_HFP_VOICE_DIAL_DISABLE_REQ:
            appHfpHandleInternalHfpVoiceDialDisableRequest();
            return;

        case HFP_INTERNAL_HFP_CALL_ACCEPT_REQ:
            appHfpHandleInternalHfpCallAcceptRequest();
            return;

        case HFP_INTERNAL_HFP_CALL_REJECT_REQ:
            appHfpHandleInternalHfpCallRejectRequest();
            return;

        case HFP_INTERNAL_HFP_CALL_HANGUP_REQ:
            appHfpHandleInternalHfpCallHangupRequest();
            return;

        case HFP_INTERNAL_HFP_MUTE_REQ:
            appHfpHandleInternalHfpMuteRequest((HFP_INTERNAL_HFP_MUTE_REQ_T *)message);
            return;

        case HFP_INTERNAL_HFP_TRANSFER_REQ:
            appHfpHandleInternalHfpTransferRequest((HFP_INTERNAL_HFP_TRANSFER_REQ_T *)message);
            return;

        case HFP_INTERNAL_VOLUME_UP:
            appHfpVolumeRepeat(1);
            return;

        case HFP_INTERNAL_VOLUME_DOWN:
            appHfpVolumeRepeat(-1);
            return;

        case HFP_INTERNAL_NUMBER_DIAL_REQ:
            appHfpHandleInternalNumberDialRequest((HFP_INTERNAL_NUMBER_DIAL_REQ_T *)message);
            return;
    }

    /* HFP profile library messages */
    switch (id)
    {
        case HFP_INIT_CFM:
            appHfpHandleHfpInitConfirm((HFP_INIT_CFM_T *)message);
            return;

        case HFP_SLC_CONNECT_IND:
            appHfpHandleHfpSlcConnectIndication((HFP_SLC_CONNECT_IND_T *)message);
            return;

        case HFP_SLC_CONNECT_CFM:
            appHfpHandleHfpSlcConnectConfirm((HFP_SLC_CONNECT_CFM_T *)message);
            return;

        case HFP_SLC_DISCONNECT_IND:
            appHfpHandleHfpSlcDisconnectIndication((HFP_SLC_DISCONNECT_IND_T *)message);
            return;

        case HFP_AUDIO_CONNECT_IND:
             appHfpHandleHfpAudioConnectIndication((HFP_AUDIO_CONNECT_IND_T *)message);
             return;

        case HFP_AUDIO_CONNECT_CFM:
             appHfpHandleHfpAudioConnectConfirmation((HFP_AUDIO_CONNECT_CFM_T *)message);
             return;

        case HFP_AUDIO_DISCONNECT_IND:
             appHfpHandleHfpAudioDisconnectIndication((HFP_AUDIO_DISCONNECT_IND_T *)message);
             return;

        case HFP_RING_IND:
             appHfpHandleHfpRingIndication((HFP_RING_IND_T *)message);
             return;

        case HFP_SERVICE_IND:
             appHfpHandleHfpServiceIndication((HFP_SERVICE_IND_T *)message);
             return;

        case HFP_CALL_STATE_IND:
             appHfpHandleHfpCallStateIndication((HFP_CALL_STATE_IND_T *)message);
             return;

        case HFP_VOICE_RECOGNITION_IND:
             appHfpHandleHfpVoiceRecognitionIndication((HFP_VOICE_RECOGNITION_IND_T *)message);
             return;

        case HFP_VOICE_RECOGNITION_ENABLE_CFM:
             appHfpHandleHfpVoiceRecognitionEnableConfirmation((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message);
             return;

        case HFP_CALLER_ID_IND:
             appHfpHandleHfpCallerIdIndication((HFP_CALLER_ID_IND_T *)message);
             return;

        case HFP_CALLER_ID_ENABLE_CFM:
             appHfpHandleHfpCallerIdEnableConfirmation((HFP_CALLER_ID_ENABLE_CFM_T *)message);
             return;

        case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
             appHfpHandleHfpVolumeSyncSpeakerGainIndication((HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *)message);
             return;

        case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
             appHfpHandleHfpVolumeSyncMicrophoneGainIndication((HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *)message);
             return;

        case HFP_CALL_ANSWER_CFM:
             appHfpHandleHfpCallAnswerConfirmation((HFP_CALL_ANSWER_CFM_T *)message);
             return;

        case HFP_CALL_TERMINATE_CFM:
             appHfpHandleHfpCallTerminateConfirmation((HFP_CALL_TERMINATE_CFM_T *)message);
             return;

        case HFP_AT_CMD_CFM:
             appHfpHandleHfpAtCmdConfirm((HFP_AT_CMD_CFM_T*)message);
             return;

        case HFP_UNRECOGNISED_AT_CMD_IND:
             appHfpHandleHfpUnrecognisedAtCmdInd((HFP_UNRECOGNISED_AT_CMD_IND_T*)message);
             return;

        /* Handle additional messages */
        case HFP_HS_BUTTON_PRESS_CFM:
        case HFP_DIAL_LAST_NUMBER_CFM:
        case HFP_SIGNAL_IND:
        case HFP_ROAM_IND:
        case HFP_BATTCHG_IND:
        case HFP_CALL_WAITING_IND:
        case HFP_EXTRA_INDICATOR_INDEX_IND:
        case HFP_EXTRA_INDICATOR_UPDATE_IND:
        case HFP_NETWORK_OPERATOR_IND:
        case HFP_CURRENT_CALLS_CFM:
        case HFP_DIAL_NUMBER_CFM:
            return;

        case HFP_HF_INDICATORS_REPORT_IND:
            appHfpHandleHfpHfIndicatorsReportInd((HFP_HF_INDICATORS_REPORT_IND_T *)message);
            return;

        case HFP_HF_INDICATORS_IND:
            appHfpHandleHfpHfIndicatorsInd((HFP_HF_INDICATORS_IND_T *)message);
            return;
    }

    /* Handle connection library messages */
    switch (id)
    {
        case CL_DM_REMOTE_FEATURES_CFM:
            appHfpHandleClDmRemoteFeaturesConfirm((CL_DM_REMOTE_FEATURES_CFM_T *)message);
            return;

        case CL_SM_ENCRYPT_CFM:
            appHfpHandleClDmEncryptConfirmation((CL_SM_ENCRYPT_CFM_T *)message);
            return;
   }

    /* Handle other messages */
    switch (id)
    {
        case CON_MANAGER_CONNECTION_IND:
            appHfpHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *)message);
            return;

        case MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT:
            appHfpHandleBatteryLevelUpdatePercent((MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T *)message);
            return;
    }

   /* Unhandled message */
   appHfpError(id, message);
}

void hfpProfile_RegisterHfpMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == APP_HFP_MESSAGE_GROUP);
    appHfpStatusClientRegister(task);
}

void hfpProfile_RegisterSystemMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == SYSTEM_MESSAGE_GROUP);
    appHfpStatusClientRegister(task);
}

/* \brief Inform hfp profile of current device Primary/Secondary role.
 */
void HfpProfile_SetRole(bool primary)
{
    if (primary)
    {
        /* Register voice source interface for hfp profile */
        VoiceSources_RegisterAudioInterface(voice_source_hfp_1, HfpProfile_GetAudioInterface());
    }

}

bool HfpProfile_IsScoConnecting(void)
{
    return (appGetHfp()->bitfields.esco_connecting != FALSE);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(APP_HFP, hfpProfile_RegisterHfpMessageGroup, NULL);
MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(SYSTEM, hfpProfile_RegisterSystemMessageGroup, NULL);

#endif
