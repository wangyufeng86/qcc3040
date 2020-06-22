/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to mirror ACL & eSCO connections.
*/

#include <stdlib.h>

#include <vm.h>
#include <message.h>

#include "av.h"
#include "connection_manager.h"
#include "hfp_profile.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include "peer_signalling.h"
#include "volume_messages.h"
#include "telephony_messages.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_audio.h"
#include "audio_sync.h"
#include "voice_sources.h"
#include "qualcomm_connection_manager.h"


#ifdef INCLUDE_MIRRORING

#include "mirror_profile.h"
#include "mirror_profile_signalling.h"
#include "mirror_profile_typedef.h"
#include "mirror_profile_marshal_typedef.h"
#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "mirror_profile_sm.h"
#include "mirror_profile_audio_source.h"
#include "mirror_profile_voice_source.h"
#include "mirror_profile_volume_observer.h"
#include "mirror_profile_peer_audio_sync_l2cap.h"
#include "mirror_profile_peer_mode_sm.h"
#include "timestamp_event.h"

mirror_profile_task_data_t mirror_profile;

/*! \brief Reset mirror SCO connection state */
void MirrorProfile_ResetEscoConnectionState(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    sp->esco.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    sp->esco.codec_mode = hfp_codec_mode_none;
    sp->esco.wesco = 0;
    sp->esco.volume = appHfpGetVolume();
}

/* \brief Set the local SCO audio volume */
void MirrorProfile_SetScoVolume(uint8 volume)
{
    mirror_profile_esco_t *esco = MirrorProfile_GetScoState();

    MIRROR_LOG("mirrorProfile_SetLocalVolume vol %u old_vol %u", volume, esco->volume);

    assert(!MirrorProfile_IsPrimary());

    if (volume != esco->volume)
    {
        esco->volume = volume;
        Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_hfp_1, event_origin_peer, volume);
    }
}

/*\! brief Set the local SCO codec params */
void MirrorProfile_SetScoCodec(hfp_codec_mode_t codec_mode)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("MirrorProfile_SetScoCodec codec_mode 0x%x", codec_mode);

    /* \todo Store the params as hfp params? That may actually be the best way w.r.t.
             handover as well? */
    sp->esco.codec_mode = codec_mode;
}

/*
    External notification helpers
*/

void MirrorProfile_SendAclConnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_CONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_CONNECT_IND, ind);
}

void MirrorProfile_SendAclDisconnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_DISCONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    /* \todo propagate disconnect reason */
    ind->reason = hci_error_unspecified;
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_DISCONNECT_IND, ind);
}

void MirrorProfile_SendScoConnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_ESCO_CONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_ESCO_CONNECT_IND, ind);
}

void MirrorProfile_SendScoDisconnectInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    MESSAGE_MAKE(ind, MIRROR_PROFILE_ESCO_DISCONNECT_IND_T);
    BdaddrTpFromBredrBdaddr(&ind->tpaddr, &sp->acl.bd_addr);
    /* \todo propagate disconnect reason */
    ind->reason = hci_error_unspecified;
    TaskList_MessageSend(sp->client_tasks, MIRROR_PROFILE_ESCO_DISCONNECT_IND, ind);
}

void MirrorProfile_SendA2dpStreamActiveInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_MessageSendId(sp->client_tasks, MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND);
}

void MirrorProfile_SendA2dpStreamInactiveInd(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_MessageSendId(sp->client_tasks, MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND);
}

/*
    Message handling functions
*/

/*! \brief Inspect profile and internal state and decide the target state. */
void MirrorProfile_SetTargetStateFromProfileState(void)
{
    mirror_profile_state_t target = MIRROR_PROFILE_STATE_DISCONNECTED;

    if (MirrorProfile_IsPrimary())
    {
        if (appPeerSigIsConnected() &&
            MirrorProfile_IsAudioSyncL2capConnected() &&
            appDeviceIsHandsetAnyProfileConnected() &&
            MirrorProfile_IsQhsReady())
        {
            target = MIRROR_PROFILE_STATE_ACL_CONNECTED;

            /* SCO has higher priority than A2DP */
            if (appDeviceIsHandsetScoActive() && MirrorProfile_IsEscoMirroringEnabled()
                && MirrorProfile_IsVoiceSourceSupported(voice_source_hfp_1))
            {
                target = MIRROR_PROFILE_STATE_ESCO_CONNECTED;
            }
            else if (MirrorProfile_GetA2dpState()->state == AUDIO_SYNC_STATE_ACTIVE)
            {
                if (MirrorProfile_IsA2dpMirroringEnabled())
                {
                    target = MIRROR_PROFILE_STATE_A2DP_CONNECTED;
                }
            }
        }
        MirrorProfile_SetTargetState(target);
    }
}

/*!  \brief Handle an APP_HFP_CONNECTED_IND.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpConnectedInd(const APP_HFP_CONNECTED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpConnectedInd state 0x%x handset %u",
                MirrorProfile_GetState(), appDeviceIsHandset(&ind->bd_addr));

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle APP_HFP_DISCONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpDisconnectedInd(const APP_HFP_DISCONNECTED_IND_T *ind)
{
    UNUSED(ind);
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_A2DP_CONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAvA2dpConnectedInd(const AV_A2DP_CONNECTED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleAvA2dpConnectedInd state 0x%x", MirrorProfile_GetState());

	if(MirrorProfile_IsPrimary())
	{
	    appA2dpSyncRegister(ind->av_instance, MirrorProfile_GetSyncIf());
	}

    /* Target state is updated on AUDIO_SYNC_STATE_IND */
}

/*! \brief Handle APP_HFP_VOLUME_IND

    Only Primary should receive this, because the Handset HFP must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpVolumeInd(const APP_HFP_VOLUME_IND_T *ind)
{
    if (MirrorProfile_IsPrimary())
    {
        MirrorProfile_GetScoState()->volume = ind->volume;

        MIRROR_LOG("mirrorProfile_HandleAppHfpVolumeInd volume %u", ind->volume);

        MirrorProfile_SendHfpVolumeToSecondary(ind->volume);
    }
}

/*! \brief Handle TELEPHONY_INCOMING_CALL

    Happens when a call is incoming, but before the SCO channel has been
    created.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleTelephonyIncomingCall(void)
{
    /* Save time later by starting DSP now */
    appKymeraProspectiveDspPowerOn();
}

/*! \brief Handle TELEPHONY_CALL_ONGOING

    Happens when a call is outgoing, but before the SCO channel has been
    created.

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleTelephonyOutgoingCall(void)
{
    /* Prepare to mirror the SCO by exiting sniff on the peer link.
       This speeds up connecting the SCO mirror. The link is put back to sniff
       once the SCO mirror is connected */
    mirrorProfilePeerMode_SetTargetState(MIRROR_PROFILE_PEER_MODE_STATE_ACTIVE);
}

/*! \brief Handle TELEPHONY_CALL_ENDED
*/
static void mirrorProfile_HandleTelephonyCallEnded(void)
{
}

/*! \brief Handle APP_HFP_SCO_CONNECTING_IND */
static void mirrorProfile_HandleAppHfpScoConnectingInd(void)
{
}

/*! \brief Handle APP_HFP_SCO_CONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpScoConnectedInd(void)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpScoConnectedInd");

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle APP_HFP_SCO_DISCONNECTED_IND

    Only Primary should receive this, because the Handset must always
    be connected to the Primary.
*/
static void mirrorProfile_HandleAppHfpScoDisconnectedInd(void)
{
    MIRROR_LOG("mirrorProfile_HandleAppHfpScoDisconnectedInd");

    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle PEER_SIG_CONNECTION_IND

    Both Primary and Secondary will receive this when the peer signalling
    channel is connected and disconnected.
*/
static void mirrorProfile_HandlePeerSignallingConnectionInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    UNUSED(ind);
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_A2DP_DISCONNECTED_IND_T
    \param ind The message.
*/
static void mirrorProfile_HandleAvA2dpDisconnectedInd(const AV_A2DP_DISCONNECTED_IND_T *ind)
{
    appA2dpSyncUnregister(ind->av_instance, MirrorProfile_GetSyncIf());
    MirrorProfile_GetA2dpState()->state = AUDIO_SYNC_STATE_DISCONNECTED;
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_AVRCP_CONNECTED_IND */
static void mirrorProfile_HandleAvAvrcpConnectedInd(void)
{
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AV_AVRCP_DISCONNECTED_IND */
static void mirrorProfile_HandleAvAvrcpDisconnectedInd(void)
{
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AUDIO_SYNC_CONNECT_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncConnectInd(const AUDIO_SYNC_CONNECT_IND_T *ind)
{
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncConnectInd");

        MirrorProfile_GetA2dpState()->state = AUDIO_SYNC_STATE_CONNECTED;
        MirrorProfile_SendA2dpStreamContextToSecondary();

        MESSAGE_MAKE(rsp, AUDIO_SYNC_CONNECT_RES_T);
        rsp->sync_id = ind->sync_id;
        MessageSend(ind->task, AUDIO_SYNC_CONNECT_RES, rsp);

        MirrorProfile_SetTargetStateFromProfileState();
    }
    else
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncConnectInd invalid audio source parameters");
    }
}

/*! \brief Handle AUDIO_SYNC_ACTIVATE_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncActivateInd(const AUDIO_SYNC_ACTIVATE_IND_T *ind)
{
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncActivateInd");
        /* The context is sent to the secondary with the state set to
           AUDIO_SYNC_STATE_CONNECTED, not AUDIO_SYNC_STATE_ACTIVE.
           This ensures that the secondary reports the correct
           correct mirror_profile_a2dp_start_mode_t. */
        MirrorProfile_GetA2dpState()->state = AUDIO_SYNC_STATE_CONNECTED;
        MirrorProfile_SendA2dpStreamContextToSecondary();
        MirrorProfile_GetA2dpState()->state = AUDIO_SYNC_STATE_ACTIVE;
        MirrorProfile_SetTargetStateFromProfileState();
        {
            MESSAGE_MAKE(rsp, AUDIO_SYNC_ACTIVATE_RES_T);
            rsp->sync_id = ind->sync_id;
            if (MirrorProfile_IsAclConnected())
            {
                MirrorProfile_SetA2dpMirrorStartLock();
            }
            MessageSendConditionally(ind->task, AUDIO_SYNC_ACTIVATE_RES, rsp,
                                     MirrorProfile_GetA2dpStartLockAddr());
        }
    }
    else
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncActivateInd invalid audio source parameters");
    }
}

/*! \brief Handle AUDIO_SYNC_STATE_IND_T
    \param ind The message.

    The only state of interest here is disconnected, since other states are
    indicated in other sync messages.
*/
static void MirrorProfile_HandleAudioSyncStateInd(const AUDIO_SYNC_STATE_IND_T *ind)
{
    MIRROR_LOG("MirrorProfile_HandleAudioSyncStateInd state:%u", ind->state);

    switch (ind->state)
    {
        case AUDIO_SYNC_STATE_DISCONNECTED:
        break;
        case AUDIO_SYNC_STATE_CONNECTED:
            MirrorProfile_StoreAudioSourceParameters(ind->source_id);
        break;
        case AUDIO_SYNC_STATE_ACTIVE:
        break;
    }

    MirrorProfile_GetA2dpState()->state = ind->state;
    MirrorProfile_SendA2dpStreamContextToSecondary();
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Handle AUDIO_SYNC_CODEC_RECONFIGURED_IND_T
    \param ind The message.
*/
static void MirrorProfile_HandleAudioSyncReconfiguredInd(const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *ind)
{
    if(MirrorProfile_StoreAudioSourceParameters(ind->source_id))
    {
        MirrorProfile_SendA2dpStreamContextToSecondary();
    }
    else
    {
        MIRROR_LOG("MirrorProfile_HandleAudioSyncReconfiguredInd invalid audio source parameters");
    }
}

/*! \brief Handle QHS link establishing between buds or QHS start timeout */
static void MirrorProfile_HandleQhsReadyOrFailed(void)
{
    MirrorProfile_SetQhsReady();
    MirrorProfile_SetTargetStateFromProfileState();
    MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT);
}

static void MirrorProfile_HandleQhsConnectedInd(const QCOM_CON_MANAGER_QHS_CONNECTED_T * message)
{
    if(appDeviceIsPeer(&message->bd_addr))
    {
        MirrorProfile_HandleQhsReadyOrFailed();
    }
}

static void mirrorProfile_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    /* Notifications from other bt domain modules */
    case CON_MANAGER_TP_DISCONNECT_IND:
        mirrorProfile_HandleTpConManagerDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
        break;

    case CON_MANAGER_TP_CONNECT_IND:
        mirrorProfile_HandleTpConManagerConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *)message);
        break;

    case APP_HFP_CONNECTED_IND:
        mirrorProfile_HandleAppHfpConnectedInd((const APP_HFP_CONNECTED_IND_T *)message);
        break;

    case APP_HFP_DISCONNECTED_IND:
        mirrorProfile_HandleAppHfpDisconnectedInd((const APP_HFP_DISCONNECTED_IND_T *)message);
        break;

    case APP_HFP_SCO_INCOMING_RING_IND:
        /* \todo Use this as a trigger to send a ring command to Secondary */
        break;

    case APP_HFP_SCO_INCOMING_ENDED_IND:
        /* \todo Use this as a trigger to send a stop ring command to Secondary */
        break;

    case APP_HFP_VOLUME_IND:
        mirrorProfile_HandleAppHfpVolumeInd((const APP_HFP_VOLUME_IND_T *)message);
        break;

    case APP_HFP_SCO_CONNECTING_IND:
        mirrorProfile_HandleAppHfpScoConnectingInd();
        break;

    case APP_HFP_SCO_CONNECTED_IND:
        mirrorProfile_HandleAppHfpScoConnectedInd();
        break;

    case APP_HFP_SCO_DISCONNECTED_IND:
        mirrorProfile_HandleAppHfpScoDisconnectedInd();
        break;

    case AV_A2DP_CONNECTED_IND:
        mirrorProfile_HandleAvA2dpConnectedInd((const AV_A2DP_CONNECTED_IND_T *)message);
        break;

    case AV_A2DP_DISCONNECTED_IND:
        mirrorProfile_HandleAvA2dpDisconnectedInd((const AV_A2DP_DISCONNECTED_IND_T *)message);
        break;

    case AV_AVRCP_CONNECTED_IND:
        mirrorProfile_HandleAvAvrcpConnectedInd();
        break;

    case AV_AVRCP_DISCONNECTED_IND:
        mirrorProfile_HandleAvAvrcpDisconnectedInd();
        break;

    case TELEPHONY_INCOMING_CALL:
        mirrorProfile_HandleTelephonyIncomingCall();
        break;

    case TELEPHONY_CALL_ONGOING:
        mirrorProfile_HandleTelephonyOutgoingCall();
        break;

    case TELEPHONY_CALL_ENDED:
        mirrorProfile_HandleTelephonyCallEnded();
        break;

    /* Internal mirror_profile messages */
    case MIRROR_INTERNAL_DELAYED_KICK:
        MirrorProfile_SmKick();
        break;

    case MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT:
        MirrorProfile_PeerLinkPolicyHandleIdleTimeout();
        break;

    /* MDM prims from firmware */
    case MESSAGE_BLUESTACK_MDM_PRIM:
        MirrorProfile_HandleMessageBluestackMdmPrim((const MDM_UPRIM_T *)message);
        break;

    /* Peer Signalling messages */
    case PEER_SIG_CONNECTION_IND:
        mirrorProfile_HandlePeerSignallingConnectionInd((const PEER_SIG_CONNECTION_IND_T *)message);
        break;

    case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
        MirrorProfile_HandlePeerSignallingMessage((const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *)message);
        break;

    case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
        MirrorProfile_HandlePeerSignallingMessageTxConfirm((const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *)message);
        break;

    /* Audio sync messages */
    case AUDIO_SYNC_CONNECT_IND:
        MirrorProfile_HandleAudioSyncConnectInd((const AUDIO_SYNC_CONNECT_IND_T *)message);
        break;

    case AUDIO_SYNC_ACTIVATE_IND:
        MirrorProfile_HandleAudioSyncActivateInd((const AUDIO_SYNC_ACTIVATE_IND_T *)message);
        break;

    case AUDIO_SYNC_STATE_IND:
        MirrorProfile_HandleAudioSyncStateInd((const AUDIO_SYNC_STATE_IND_T *)message);
        break;

    case AUDIO_SYNC_CODEC_RECONFIGURED_IND:
        MirrorProfile_HandleAudioSyncReconfiguredInd((const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *)message);
        break;
    
    /* Connection library messages */
    case CL_L2CAP_REGISTER_CFM:
        MirrorProfile_HandleClL2capRegisterCfm((const CL_L2CAP_REGISTER_CFM_T *)message);
        break; 
    
    case CL_SDP_REGISTER_CFM:
        MirrorProfile_HandleClSdpRegisterCfm((const CL_SDP_REGISTER_CFM_T *)message);
        break;
    
    case CL_L2CAP_CONNECT_IND:
        MirrorProfile_HandleL2capConnectInd((const CL_L2CAP_CONNECT_IND_T *)message);
        break;
    
    case CL_L2CAP_CONNECT_CFM:
        MirrorProfile_HandleL2capConnectCfm((const CL_L2CAP_CONNECT_CFM_T *)message);
        break;
    
    case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
        MirrorProfile_HandleClSdpServiceSearchAttributeCfm((const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
        break;
    
    case CL_L2CAP_DISCONNECT_IND:
        MirrorProfile_HandleL2capDisconnectInd((const CL_L2CAP_DISCONNECT_IND_T *)message);
        break;

    case CL_L2CAP_DISCONNECT_CFM:
        MirrorProfile_HandleL2capDisconnectCfm((const CL_L2CAP_DISCONNECT_CFM_T *)message);
        break;

    case QCOM_CON_MANAGER_QHS_CONNECTED:
        MirrorProfile_HandleQhsConnectedInd((const QCOM_CON_MANAGER_QHS_CONNECTED_T *) message);
        break;

    case MIRROR_INTERNAL_QHS_START_TIMEOUT:
         /* QHS link didn't establish */
        MirrorProfile_HandleQhsReadyOrFailed();
        break;

    case MIRROR_INTERNAL_IDLE_PEER_ENTER_SNIFF:
        mirrorProfile_HandleIdlePeerEnterSniff();
        break;

    default:
        MIRROR_LOG("mirrorProfile_MessageHandler: Unhandled id 0x%x", id);
        break;
    }
}

/*! \brief Send an audio_sync_msg_t internally.

    The audio_sync_msg_t messages must only be handled in a known stable
    state, so they are sent conditionally on the lock.
*/
static void mirrorProfile_SyncSendAudioSyncMessage(audio_sync_t *sync_inst, MessageId id, Message message)
{
    PanicFalse(MessageCancelAll(sync_inst->task, id) <= 1);
    MessageSendConditionally(sync_inst->task, id, message, &MirrorProfile_GetLock());
}

/*! \brief Registers mirror profile as an observer to Audio Source Observer interface for an audio source.

    \param source The audio source
 */
static void mirrorProfile_RegisterForAudioSourceVolume(audio_source_t source)
{
    AudioSources_RegisterObserver(source, MirrorProfile_GetObserverInterface());
}

bool MirrorProfile_Init(Task task)
{
    memset(&mirror_profile, 0, sizeof(mirror_profile));
    mirror_profile.task_data.handler = mirrorProfile_MessageHandler;
    mirror_profile.state = MIRROR_PROFILE_STATE_DISCONNECTED;
    mirror_profile.target_state = MIRROR_PROFILE_STATE_DISCONNECTED;
    mirror_profile.acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    mirror_profile.esco.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
    mirror_profile.esco.volume = appHfpGetVolume();
    mirror_profile.init_task = task;
    mirror_profile.client_tasks = TaskList_Create();
    mirror_profile.sync_if.task = &mirror_profile.task_data;
    mirror_profile.sync_if.sync_if.SendSyncMessage = mirrorProfile_SyncSendAudioSyncMessage;
    mirror_profile.audio_sync.local_psm = 0;
    mirror_profile.audio_sync.remote_psm = 0;
    mirror_profile.audio_sync.sdp_search_attempts = 0;
    mirror_profile.audio_sync.l2cap_state = MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE;
    mirror_profile.enable_esco_mirroring = TRUE;
    mirror_profile.enable_a2dp_mirroring = TRUE;

    /* Register a Protocol/Service Multiplexor (PSM) that will be
       used for this application. The same PSM is used at both
       ends. */
    ConnectionL2capRegisterRequest(MirrorProfile_GetTask(), L2CA_PSM_INVALID, 0);

    /* Register for notifications when devices and/or profiles connect
       or disconnect. */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, MirrorProfile_GetTask());
    appHfpStatusClientRegister(MirrorProfile_GetTask());
    appAvStatusClientRegister(MirrorProfile_GetTask());
    Telephony_RegisterForMessages(MirrorProfile_GetTask());
    QcomConManagerRegisterClient(MirrorProfile_GetTask());

    /* Register to Audio Source Observer to receive Volume */
    mirrorProfile_RegisterForAudioSourceVolume(audio_source_a2dp_1);

    /* Register a channel for peer signalling */
    appPeerSigMarshalledMsgChannelTaskRegister(MirrorProfile_GetTask(),
                                            PEER_SIG_MSG_CHANNEL_MIRROR_PROFILE,
                                            mirror_profile_marshal_type_descriptors,
                                            NUMBER_OF_MIRROR_PROFILE_MARSHAL_TYPES);

    /* Register for peer signaling notifications */
    appPeerSigClientRegister(MirrorProfile_GetTask());

    /* Now wait for MDM_REGISTER_CFM */
    return TRUE;
}

/* \brief Inform mirror profile of current device Primary/Secondary role.

    todo : A Primary <-> Secondary role switch should only be allowed
    when the state machine is in a stable state. This will be more important
    when the handover logic is implemented.
*/
void MirrorProfile_SetRole(bool primary)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    if (!primary)
    {
        /* Take ownership of the A2DP source (mirror) when becoming secondary */
        AudioSources_RegisterAudioInterface(audio_source_a2dp_1, MirrorProfile_GetAudioInterface());

        /* Register voice source interface */
        VoiceSources_RegisterAudioInterface(voice_source_hfp_1, MirrorProfile_GetVoiceInterface());

        /* Clear delayed kicks when becoming secondary. This avoids the state
           machine being kicked in the secondary role resulting in panic */
        MessageCancelAll(MirrorProfile_GetTask(), MIRROR_INTERNAL_DELAYED_KICK);
    }

    sp->is_primary = primary;
    MIRROR_LOG("MirrorProfile_SetRole primary %u", sp->is_primary);
}
/* \brief Get the SCO sink associated with the mirror eSCO link. */
Sink MirrorProfile_GetScoSink(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return StreamScoSink(sp->esco.conn_handle);
}

void MirrorProfile_Connect(Task task, const bdaddr *peer_addr)
{
    if(peer_addr)
    {
        DEBUG_LOG("MirrorProfile_Connect - startup");

        mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
        mirror_inst->is_primary = TRUE;
        MirrorProfile_CreateAudioSyncL2capChannel(task, peer_addr);
    }
    else
    {
        DEBUG_LOG("MirrorProfile_Connect - Peer address is NULL");
        Panic();
    }
}

void MirrorProfile_Disconnect(Task task)
{
    DEBUG_LOG("MirrorProfile_Disconnect");

    MirrorProfile_CloseAudioSyncL2capChannel(task);
}

void MirrorProfile_ClientRegister(Task client_task)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_AddTask(sp->client_tasks, client_task);
}

void MirrorProfile_ClientUnregister(Task client_task)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_RemoveTask(sp->client_tasks, client_task);
}

bool MirrorProfile_IsConnected(void)
{
    return (MirrorProfile_IsAclConnected() || MirrorProfile_IsEscoConnected());
}

bool MirrorProfile_IsEscoActive(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return (SinkIsValid(StreamScoSink(sp->esco.conn_handle)));
}

bool MirrorProfile_IsA2dpActive(void)
{
    return MirrorProfile_IsA2dpConnected();
}

uint16 MirrorProfile_GetMirrorAclHandle(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    return sp->acl.conn_handle;
}

/*
    Test only functions
*/
void MirrorProfile_Destroy(void)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();
    TaskList_Destroy(sp->client_tasks);
}

mirror_profile_a2dp_start_mode_t MirrorProfile_GetA2dpStartMode(void)
{
    mirror_profile_a2dp_start_mode_t mode = MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED;

    if (MirrorProfile_IsQ2Q())
    {
        mode = MIRROR_PROFILE_A2DP_START_Q2Q_MODE;
    }
    else
    {
        bool sync_start;

        switch (MirrorProfile_GetState())
        {
            case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
            case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
                sync_start = TRUE;
                break;
            default:
                sync_start = FALSE;
                break;
        }
        if (MirrorProfile_IsPrimary())
        {
            mode = sync_start ? MIRROR_PROFILE_A2DP_START_PRIMARY_SYNCHRONISED :
                                MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED;
        }
        else
        {
            switch (MirrorProfile_GetA2dpState()->state)
            {
                case AUDIO_SYNC_STATE_CONNECTED:
                    mode = sync_start ? MIRROR_PROFILE_A2DP_START_SECONDARY_SYNCHRONISED :
                                        MIRROR_PROFILE_A2DP_START_SECONDARY_JOINS_SYNCHRONISED;
                break;

                case AUDIO_SYNC_STATE_ACTIVE:
                    mode = MIRROR_PROFILE_A2DP_START_SECONDARY_JOINS_SYNCHRONISED;
                break;

                default:
                    DEBUG_LOG_WARN("MirrorProfile_GetA2dpStartMode Unexpected a2dp state:%d", 
                                   MirrorProfile_GetA2dpState()->state);
                break;
            }
        }
    }

    return mode;
}

Sink MirrorProfile_GetA2dpAudioSyncTransportSink(void)
{
    return MirrorProfile_GetAudioSyncL2capState()->link_sink;
}

Source MirrorProfile_GetA2dpAudioSyncTransportSource(void)
{
    return MirrorProfile_GetAudioSyncL2capState()->link_source;
}

/*! \brief Request mirror_profile to Enable Mirror Esco.

    This should only be called from the Primary device.
*/
void MirrorProfile_EnableMirrorEsco(void)
{
    DEBUG_LOG("MirrorProfile_EnableMirrorEsco, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_esco_mirroring = TRUE;
    MirrorProfile_SetTargetStateFromProfileState();
}

/*! \brief Request mirror_profile to Disable Mirror Esco.

    This should only be called from the Primary device.
*/
void MirrorProfile_DisableMirrorEsco(void)
{
    DEBUG_LOG("MirrorProfile_DisableMirrorEsco, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_esco_mirroring = FALSE;
    MirrorProfile_SetTargetStateFromProfileState();
}

void MirrorProfile_EnableMirrorA2dp(void)
{
    DEBUG_LOG("MirrorProfile_EnableMirrorA2dp, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_a2dp_mirroring = TRUE;
    MirrorProfile_SetTargetStateFromProfileState();
}

void MirrorProfile_DisableMirrorA2dp(void)
{
    DEBUG_LOG("MirrorProfile_DisableMirrorA2dp, State(0x%x)", MirrorProfile_GetState());
    mirror_profile.enable_a2dp_mirroring = FALSE;
    MirrorProfile_SetTargetStateFromProfileState();
}

uint16 MirrorProfile_GetMirrorState(void)
{
    return MirrorProfile_GetState();
}

uint32 MirrorProfile_GetExpectedPeerLinkTransmissionTime(void)
{
    return MirrorProfilePeerLinkPolicy_GetExpectedTransmissionTime();
}

void MirrorProfile_HandleHfpAudioConnectConfirmation(voice_source_t source)
{
    DEBUG_LOG("MirrorProfile_HandleHfpAudioConnectConfirmation");

    if (MirrorProfile_IsConnected() && MirrorProfile_IsEscoMirroringEnabled()
        && MirrorProfile_IsVoiceSourceSupported(voice_source_hfp_1))
    {
        /* Defer starting eSCO audio until eSCO mirroring is connected.
           Since eSCO mirroring starts at the same time on both buds, the eSCO
           audio will then start approximately in sync. If eSCO mirroring is
           disabled, we don't defer, since that means eSCO mirroring will not
           start. */
        MirrorProfile_Get()->hfp_voice_source_routed = 0;
    }
    else
    {
        /* If mirror profile is not connected or eSCO mirroring is disabled
           then start eSCO audio immediately by notifying telephony service. */
        Telephony_NotifyCallAudioConnected(source);
        MirrorProfile_Get()->hfp_voice_source_routed = 1;
    }
}

bool MirrorProfile_IsVoiceSourceSupported(voice_source_t source)
{
    source_defined_params_t source_params = {0};
    bool mirroring_supported = TRUE;

    /* The local HFP SCO should already have been connected up to the point
       where we know the type (SCO/eSCO) and eSCO connection paramters. */
    if (VoiceSources_GetConnectParameters(source, &source_params))
    {
        voice_connect_parameters_t *voice_params = (voice_connect_parameters_t *)source_params.data;
        assert(source_params.data_length == sizeof(voice_connect_parameters_t));

        /* Mirroring is not supported for:
            SCO links (tesco == 0)
            eSCO links using HV3 packets (tesco == 6) */
        if (voice_params->tesco <= 6)
        {
            mirroring_supported = FALSE;
        }

        VoiceSources_ReleaseConnectParameters(source, &source_params);
    }

    DEBUG_LOG("MirrorProfile_IsVoiceSourceSupported supported %d", mirroring_supported);
    return mirroring_supported;
}

#endif /* INCLUDE_MIRRORING */
