/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_flags.c
\brief      Handle boolean state changes that are monitored by state proxy.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_flags.h"

/* framework includes */
#include <bt_device.h>
#include <hfp_profile.h>
#include <peer_signalling.h>
#include <connection_manager.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>
#include <marshal.h>

/* forward declarations */
static void stateProxy_UpdateConnectedSupportedProfiles(void);

/*! \brief Get flags state for initial state message. */
void stateProxy_GetInitialFlags(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    proxy->local_state->flags.a2dp_connected = appDeviceIsHandsetA2dpConnected();
    proxy->local_state->flags.avrcp_connected = appDeviceIsHandsetAvrcpConnected();
    proxy->local_state->flags.hfp_connected = appDeviceIsHandsetHfpConnected();
    /* activity */
    /*! \todo candidate to become MediaPlayer_IsActive() or similar */
    proxy->local_state->flags.a2dp_streaming = appDeviceIsHandsetA2dpStreaming();
    /*! \todo why is this direct to HFP unlike others? Will probably become
     * VoiceService_IsActive() or similar */
    proxy->local_state->flags.sco_active = appHfpIsScoActive();
    proxy->local_state->flags.dfu_in_progress = FALSE;
    proxy->local_state->flags.advertising = ConManagerIsConnectionAllowed(cm_transport_ble);
    proxy->local_state->flags.ble_connected = ConManagerAnyTpLinkConnected(cm_transport_ble);
}

/*! \brief Set remote device flags to initial state. */
void stateProxy_SetRemoteInitialFlags(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    /* Resetting all the flags */
    memset(&proxy->remote_state->flags, 0, sizeof(state_proxy_data_flags_t));

    /* Setting InCase, as expectation that earbud in InCase at startup*/
    proxy->remote_state->flags.in_case = TRUE;
}

/*! \brief Handle local flags event. 
    
    Update the local state for the flag
    Notify local specific event clients
    If Secondary send to Primary
    
    \param[in] marshal_type Marshalling type definition for the flags.
    \param[in] setting Boolean value to set the flag to.
    \param[in] ind Pointer to the message containing event.
    \param[in] ind_size Size of the event message in bytes.

 */
void stateProxy_FlagIndicationHandler(marshal_type_t marshal_type, bool setting,
                                             const void* ind, size_t ind_size)
{
    state_proxy_event_type event_type;

    DEBUG_LOG("stateProxy_FlagIndicationHandler marshal_type %u set %u", marshal_type, setting);

    event_type = stateProxy_UpdateFlag(marshal_type, setting, state_proxy_source_local);

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(state_proxy_source_local,
                                         event_type,
                                         ind);

    /* currently have to send flags from Primary to Secondary as well as vice-versa,
     * this is so that profile connectivity device attributes are recorded for the
     * secondary to use if it becomes primary */
    if (!stateProxy_Paused() &&
        appPeerSigIsConnected())
    {
        void* msg = NULL;
        if (ind)
        {
            msg = PanicUnlessMalloc(ind_size);
            memcpy(msg, ind, ind_size);
        }
        else
        {
            msg = PanicUnlessMalloc(sizeof(state_proxy_msg_empty_payload_t));
            ((state_proxy_msg_empty_payload_t*)msg)->type = event_type;
            marshal_type = MARSHAL_TYPE_state_proxy_msg_empty_payload_t;
        }
        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                         msg, marshal_type);
    }
}

/*! \brief Handle remote flags event. 
    \param[in] marshal_type Marshalling type definition for the flags.
    \param[in] setting Boolean value to set the flag to.
    \param[in] ind Pointer to the message containing event.
 */
void stateProxy_RemoteFlagIndicationHandler(marshal_type_t marshal_type, bool setting,
                                                   const void* ind)
{
    state_proxy_event_type event_type;
    
    DEBUG_LOG("stateProxy_RemoteFlagIndicationHandler marshal_type %u set %u", marshal_type, setting);

    event_type = stateProxy_UpdateFlag(marshal_type, setting, state_proxy_source_remote);

    /* must be called after stateProxy_UpdateFlag() */
//    stateProxy_UpdateConnectedSupportedProfiles();

    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                         event_type,
                                         ind);
}

/*! \brief Handle remote flag events generated by messages with no payload.
    \param[in] msg Pointer to message containing ID of remote flag event.
*/
void stateProxy_HandleMsgEmptyPayload(const state_proxy_msg_empty_payload_t* msg)
{
    DEBUG_LOG("stateProxy_HandleMsgEmptyPayload type %u", msg->type);

    /* generate corresponding original event that has no payload. */
    switch (msg->type)
    {
        case state_proxy_event_type_a2dp_streaming:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_ACTIVE_IND_T, TRUE, NULL);
            break;
        case state_proxy_event_type_a2dp_not_streaming:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_AV_STREAMING_INACTIVE_IND_T, FALSE, NULL);
            break;
        case state_proxy_event_type_sco_active:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_APP_HFP_SCO_CONNECTED_IND_T, TRUE, NULL);
            break;
        case state_proxy_event_type_sco_inactive:
            stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_APP_HFP_SCO_DISCONNECTED_IND_T, FALSE, NULL);
            break;
        default:
            break;
    }
}

/*! \brief Helper function to update a flag state for a local or remote data set.
    \param[in] marshal_type Marshalling type definition for the flags.
    \param[in] setting Boolean value to set the flag to.
    \param[in] source Local or Remote state proxy data set.
*/
state_proxy_event_type stateProxy_UpdateFlag(marshal_type_t marshal_type, bool setting,
                                             state_proxy_source source)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    state_proxy_event_type event_type = state_proxy_event_type_a2dp_conn;
    state_proxy_data_t* state = source == state_proxy_source_local ? proxy->local_state :
                                                                     proxy->remote_state;
    switch (marshal_type)
    {
        case MARSHAL_TYPE_AV_A2DP_CONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer A2DP connected");
            event_type = state_proxy_event_type_a2dp_conn;
            state->flags.a2dp_connected = setting;
            break;
        case MARSHAL_TYPE_AV_A2DP_DISCONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer A2DP disconnected");
            event_type = state_proxy_event_type_a2dp_discon;
            state->flags.a2dp_connected = setting;
            break;
        case MARSHAL_TYPE_AV_AVRCP_CONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer AVRCP connected");
            event_type = state_proxy_event_type_avrcp_conn;
            state->flags.avrcp_connected = setting;
            break;
        case MARSHAL_TYPE_AV_AVRCP_DISCONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer AVRCP disconnected");
            event_type = state_proxy_event_type_avrcp_discon;
            state->flags.avrcp_connected = setting;
            break;
        case MARSHAL_TYPE_AV_STREAMING_ACTIVE_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer AV streaming");
            event_type = state_proxy_event_type_a2dp_streaming;
            state->flags.a2dp_streaming = setting;
            break;
        case MARSHAL_TYPE_AV_STREAMING_INACTIVE_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer AV not streaming");
            event_type = state_proxy_event_type_a2dp_not_streaming;
            state->flags.a2dp_streaming = setting;
            break;
        case MARSHAL_TYPE_APP_HFP_CONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer HFP connected");
            event_type = state_proxy_event_type_hfp_conn;
            state->flags.hfp_connected = setting;
            break;
        case MARSHAL_TYPE_APP_HFP_DISCONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer HFP disconnected");
            event_type = state_proxy_event_type_hfp_discon;
            state->flags.hfp_connected = setting;
            break;
        case MARSHAL_TYPE_APP_HFP_SCO_CONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer SCO active");
            event_type = state_proxy_event_type_sco_active;
            state->flags.sco_active = setting;
            break;
        case MARSHAL_TYPE_APP_HFP_SCO_DISCONNECTED_IND_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer SCO inactive");
            event_type = state_proxy_event_type_sco_inactive;
            state->flags.sco_active = setting;
            break;
        case MARSHAL_TYPE_PAIRING_ACTIVITY_T:
            DEBUG_LOG("stateProxy_UpdateFlag peer handset pairing progress");
            event_type = state_proxy_event_type_is_pairing;
            state->flags.is_pairing = setting;
            break;
        default:
            Panic();
    }
    stateProxy_UpdateConnectedSupportedProfiles();
    return event_type;
}

static void stateProxy_UpdateConnectedSupportedProfiles(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();

    uint8 was_supported;

    was_supported = BtDevice_SetProfileConnectedAndSupportedFlagsFromPeer(
                        &proxy->remote_state->handset_addr,
                        proxy->remote_state->flags.a2dp_connected,
                        proxy->remote_state->flags.avrcp_connected,
                        proxy->remote_state->flags.hfp_connected,
                        hfp_handsfree_107_profile); // TODO: Get HFP profile version from peer

    if (!(was_supported & DEVICE_PROFILE_A2DP) & proxy->remote_state->flags.a2dp_connected)
    {
        stateProxy_MsgStateProxyEventClients(state_proxy_source_remote, state_proxy_event_type_a2dp_supported, NULL);
    }
    if (!(was_supported & DEVICE_PROFILE_AVRCP) & proxy->remote_state->flags.avrcp_connected)
    {
        stateProxy_MsgStateProxyEventClients(state_proxy_source_remote, state_proxy_event_type_avrcp_supported, NULL);
    }
    if (!(was_supported & DEVICE_PROFILE_HFP) & proxy->remote_state->flags.hfp_connected)
    {
        stateProxy_MsgStateProxyEventClients(state_proxy_source_remote, state_proxy_event_type_hfp_supported, NULL);
    }
}

