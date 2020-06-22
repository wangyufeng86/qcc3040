/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      AV State Machines (A2DP & AVRCP)

Main AV task.
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include <a2dp.h>
#include <avrcp.h>
#include <panic.h>
#include <connection.h>
#include <ps.h>
#include <file.h>
#include <transform.h>
#include <feature.h>
#include <string.h>
#include <stdlib.h>

#include "app_task.h"
#include "audio_sources.h"
#include "av.h"
#include "av_config.h"
#include "a2dp_profile.h"
#include "a2dp_profile_audio.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_volume.h"
#include "avrcp_profile.h"
#include "avrcp_profile_config.h"
#include "avrcp_profile_volume_observer.h"
#include <device.h>
#include <device_list.h>
#include <device_properties.h>
#include "init.h"
#include "link_policy.h"
#include <profile_manager.h>
#include "adk_log.h"
#include "ui.h"
#include "volume_messages.h"
#include "volume_utils.h"
#include "unexpected_message.h"

#ifdef INCLUDE_APTX_ADAPTIVE
#include "a2dp_profile_caps_aptx_adaptive.h"
#endif

// this shouldn't be here
#include "state_proxy.h"

#include <connection_manager.h>
/* Remove these once feature_if.h has been updated */
#define APTX_ADAPTIVE (40)
#define APTX_ADAPTIVE_MONO (41)

/*! Macro for creating messages */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)

/*!< AV data structure */
avTaskData  app_av;

/* Ui Inputs in which media player module is interested*/
const message_group_t av_ui_inputs[] =
{
    UI_INPUTS_VOLUME_MESSAGE_GROUP
};

static void appAvHandleMessage(Task task, MessageId id, Message message);

/*! \brief Handle AV error

    Some error occurred in the Av state machine, to avoid the state machine
    getting stuck, drop connection and move to 'disconnected' state.
*/
static void appAvError(avTaskData *theAv, MessageId id, Message message)
{
    UNUSED(message); UNUSED(theAv);UNUSED(id);

#ifdef AV_DEBUG
    DEBUG_LOG("appAvError %p, state=%u, id=%x", (void *)theAv, theAv->bitfields.state, id);
#else
    Panic();
#endif
}

/*! \brief returns av task pointer to requesting component

    \return av task pointer.
*/
Task appGetAvPlayerTask(void)
{
  return &app_av.task;
}

/*! \brief Check if at least one A2DP or AVRCP link is connected

    \return TRUE if either an A2DP or an AVRCP link is connected. FALSE otherwise.
*/
bool appAvHasAConnection(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst)
            if (appA2dpIsConnected(theInst) || appAvrcpIsConnected(theInst))
                return TRUE;
    }

    /* No AV connections */
    return FALSE;
}

/*! \brief Check all A2DP links are disconnected

    \return TRUE if there are no connected links. FALSE if any AV link
            has either an A2DP or an AVRCP connection.
*/
bool appAvIsDisconnected(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst)
        {
            if (!appA2dpIsDisconnected(theInst) || !appAvrcpIsDisconnected(theInst))
                return FALSE;
        }
    }

    /* No AV connections */
    return TRUE;
}

/*! \brief Check if A2DP is streaming

    \return TRUE if there is an AV that is streaming
*/
bool appAvIsStreaming(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find entry that is streaming as sink */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsSinkCodec(theInst) && appA2dpIsStreaming(theInst))
            return TRUE;
    }

    /* No AV connections */
    return FALSE;
}

/*! \brief Check if A2DP is connected

    \return TRUE if there is an AV that is connected as a sink
*/
bool Av_IsA2dpConnected(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find entry that is connected as a sink */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsSinkCodec(theInst))
            return TRUE;
    }

    /* No AV connections */
    return FALSE;
}

/*! \brief Check if AV is connected as an A2DP slave

    \return TRUE If any connected AV has A2DP signalling and is
        a TWS Sink.
*/
bool appAvIsConnectedSlave(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find entry with matchin Bluetooth address */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsStateConnectedSignalling(theInst->a2dp.state))
        {
            if (appA2dpIsSinkTwsCodec(theInst))
                return TRUE;
        }
    }

    /* No AV connections */
    return FALSE;
}

/*! \brief Check if A2DP is streaming as a master

    \return TRUE If any connected AV has A2DP signalling and is
                 a TWS Source.
*/
bool appAvIsConnectedMaster(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find entry with matching Bluetooth address */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsStateConnectedSignalling(theInst->a2dp.state))
        {
            if (appA2dpIsSourceCodec(theInst))
                return TRUE;
        }
    }

    /* No AV connections */
    return FALSE;
}

/*! \brief Make volume change

    \note A2DP/AVRCP volume range is 0-127.
    \note Increasing the volume by 100 from 100 will return TRUE as the
    volume changed but the level set will be the maximum - 127

    \param  step    Relative amount to adjust the volume by.
                    This can be negative.

    \return TRUE if the volume was changed by at least 1 level,
            FALSE in all other cases
*/
bool appAvVolumeChange(int16 step)
{
    int step_size = VolumeUtils_GetStepSize(AudioSources_GetVolume(audio_source_a2dp_1).config);
    if(step == step_size)
    {
        Volume_SendAudioSourceVolumeIncrementRequest(audio_source_a2dp_1, event_origin_local);
    }
    else if(step == -step_size)
    {
        Volume_SendAudioSourceVolumeDecrementRequest(audio_source_a2dp_1, event_origin_local);
    }
    else
    {
        int new_volume = VolumeUtils_LimitVolumeToRange((AudioSources_GetVolume(audio_source_a2dp_1).value + step),
                                                                AudioSources_GetVolume(audio_source_a2dp_1).config.range);
        Volume_SendAudioSourceVolumeUpdateRequest(audio_source_a2dp_1, event_origin_local, new_volume);
    }
    return TRUE;
}

static event_origin_t getOrigin(avInstanceTaskData *theInst)
{
    event_origin_t origin = event_origin_local;
    if(theInst)
    {
        origin = (appDeviceIsPeer(&theInst->bd_addr) ? event_origin_peer : event_origin_external);
    }
    return origin;
}


/*! \brief Set new volume

    Set a new volume, and synchronise across other AV links if
    required.

    This function only has any effect if we are the sink for an
    audio stream.

    \param      volume      The new volume
    \param      theOtherInst The instance requesting the volume to be set.
*/
void appAvVolumeSet(uint8 volume, avInstanceTaskData *theOtherInst)
{
    DEBUG_LOG("appAvVolumeSet, volume %u", volume);
    Volume_SendAudioSourceVolumeUpdateRequest(audio_source_a2dp_1, getOrigin(theOtherInst), volume);
}

/*! \brief Set device volume

    \param bd_addr Pointer to read-only device BT address.
    \param volume The device volume
    \note The volume should only be set for handsets.
*/
static bool av_SetVolume(const bdaddr *bd_addr, uint8 volume)
{
    bool volume_set = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        uint8 a2dp_volume = volume;
        volume_set = Device_GetPropertyU8(device, device_property_a2dp_volume, &a2dp_volume);
        if (!volume_set || (volume != a2dp_volume))
        {
            Device_SetPropertyU8(device, device_property_a2dp_volume, volume);
            DEBUG_LOG("av_SetVolume %x %d", bd_addr->lap, volume);
        }
    }
    return volume_set;
}

/*! \brief Get device volume

    \param bd_addr Pointer to read-only device BT address.
    \param[out] volume The device's volume
    \note The volume should only be requested for handsets.
*/
static bool av_GetVolume(const bdaddr *bd_addr, uint8 *volume)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return (device) ? Device_GetPropertyU8(device, device_property_a2dp_volume, volume) : FALSE;
}

void appAvVolumeAttributeStore(avTaskData *theAv)
{
    bdaddr bd_addr = {0};
    bool status;
    if (appDeviceIsHandsetConnected())
    {
        PanicFalse(appDeviceGetHandsetBdAddr(&bd_addr));
    }
    else
    {
        StateProxy_GetPeerHandsetAddr(&bd_addr);
    }
    status = av_SetVolume(&bd_addr, theAv->volume);
    DEBUG_LOG("appAvVolumeAttributeStore addr=%x,%x,%x vol=%u status=%d",
                bd_addr.nap, bd_addr.uap, bd_addr.lap, theAv->volume, status);
}

static void appAvVolumeLoadDeviceVolumeAndSet(avInstanceTaskData *theInst)
{
    if (!appAvVolumeIsSet(AvGetTaskData()->volume))
    {
        uint8 volume;
        PanicFalse(av_GetVolume(&theInst->bd_addr, &volume));

        DEBUG_LOG("appAvVolumeLoadDeviceVolumeAndSet(%p), volume=%u", theInst, volume);

        /* Forward volume to other instance if AV connected */
        Volume_SendAudioSourceVolumeUpdateRequest(audio_source_a2dp_1, getOrigin(theInst), volume);

        AvGetTaskData()->volume = volume;
    }
}

/*! \brief Volume handling on A2DP connect.
    \param theInst The connecting instance.

    If A2DP connection is made to the handset before AVRCP, set the system volume
    based on the stored handset device volume.

    If the new A2DP connection is to the peer, AVRCP will also connect and
    appAvVolumeHandleAvrcpConnect will handle volume setup.
*/
static void appAvVolumeHandleA2dpConnect(avInstanceTaskData *theInst)
{
    if (appDeviceIsHandset(&theInst->bd_addr))
    {
        appAvVolumeLoadDeviceVolumeAndSet(theInst);
    }
}

/*! \brief Volume handling on AVRCP Connect
    \param theInst The connecting instance.

    If the handset is the new connector, set handset and peer volume based on the connecting handset address.
    If the peer is the new connector, just set the peer volume.
*/
static void appAvVolumeHandleAvrcpConnect(avInstanceTaskData *theInst)
{
    if (appDeviceIsHandset(&theInst->bd_addr))
    {
        appAvVolumeLoadDeviceVolumeAndSet(theInst);
    }
    else if (appDeviceIsPeer(&theInst->bd_addr))
    {
        /* Check if we have A2DP connection to non TWS+ handset, if so send volume to peer */
        if (appDeviceIsHandsetA2dpConnected())
        {
            bdaddr handset_bd_addr;
            if  (appDeviceGetHandsetBdAddr(&handset_bd_addr) && !appDeviceIsTwsPlusHandset(&handset_bd_addr))
            {
                /* Get volume and forward to peer, don't send to handset */
                avInstanceTaskData *handsetInst = appAvInstanceFindFromBdAddr(&handset_bd_addr);
                if (handsetInst)
                {
                    uint8 volume = AudioSources_GetVolume(audio_source_a2dp_1).value;
                    DEBUG_LOG("appAvVolumeHandleAvrcpConnect(%p), peer connected, handset already connected, volume=%u", theInst, volume);

                    /* Pass in handset instance so we don't attempt to send volume to handset */
                    appAvVolumeSet(volume, handsetInst);
                }
            }
        }
    }
}

/*! \brief Volume handling on AVRCP/A2DP disconnect
    \param theInst The disconnecting instance.
*/
static void appAvVolumeHandleAvDisconnect(avInstanceTaskData *theInst)
{
    avTaskData *theAv = AvGetTaskData();

    if (appDeviceIsHandset(&theInst->bd_addr))
    {
        if (MessageCancelFirst(&theAv->task, AV_INTERNAL_VOLUME_STORE_REQ))
        {
            appAvVolumeAttributeStore(theAv);
        }
    }
}

/*! \brief Handle incoming AVRCP connection

    AVRCP task has get an incoming AVRCP connection, we receive this message
    so that we can indicate if we'd like it for AV or not.  If A2DP is not connected
    then we say we don't want AVRCP.
*/
static void appAvHandleAvAvrcpConnectIndication(avTaskData *theAv, const AV_AVRCP_CONNECT_IND_T *ind)
{
    avInstanceTaskData *theInst;
    DEBUG_LOG("appAvHandleAvAvrcpConnectIndication");
    UNUSED(theAv);

    theInst = appAvInstanceFindFromBdAddr(&ind->bd_addr);
    if (theInst)
    {
        /* Reject connection if A2DP is disconnected, accept if A2DP connected or connecting */
        if (appA2dpIsDisconnected(theInst))
        {
            DEBUG_LOG("appAvHandleAvAvrcpConnectIndication, rejecting");

            /* Reject incoming connection */
            appAvAvrcpConnectResponse(&theAv->task, &theInst->av_task, &ind->bd_addr, ind->connection_id,
                                      ind->signal_id,  AV_AVRCP_REJECT);
        }
        else
        {
            DEBUG_LOG("appAvHandleAvAvrcpConnectIndication, accepting");

            /* Accept incoming connection */
            appAvAvrcpConnectResponse(&theAv->task, &theInst->av_task, &ind->bd_addr, ind->connection_id,
                                      ind->signal_id,  AV_AVRCP_ACCEPT);
        }
    }
}

/*! \brief Confirmation of AVRCP connection

*/
static void appAvInstanceHandleAvAvrcpConnectCfm(avInstanceTaskData *theInst, AV_AVRCP_CONNECT_CFM_T *cfm)
{
    assert(theInst == cfm->av_instance);
    DEBUG_LOG("appAvInstanceHandleAvAvrcpConnectCfm(%p), status %d", theInst, cfm->status);

    if (cfm->status == avrcp_success)
    {
        /* Get notified of change in playback status */
        uint16 events = 1 << avrcp_event_playback_status_changed;

        /* Get notified of change in volume if TWS peer device */
        if (appDeviceIsPeer(&theInst->bd_addr))
            events |= 1 << avrcp_event_volume_changed;

        /* Register for notifications to be sent to AV task */
        appAvrcpNotificationsRegister(theInst, events);

        appAvVolumeHandleAvrcpConnect(theInst);

        /* Cancel outstanding connect later request since we are now connected */
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_LATER_REQ);
    }
}

/*! \brief Handle confirmation of AVRCP disconnection */
static void appAvInstanceHandleAvAvrcpDisconnectInd(avInstanceTaskData *theInst, AV_AVRCP_DISCONNECT_IND_T *ind)
{
    DEBUG_LOG("appAvInstanceHandleAvAvrcpDisconnectInd(%p), status %d", theInst, ind->status);
    appAvVolumeHandleAvDisconnect(theInst);
    /* Cancel outstanding disconnect later request since we are now disconnected */
    MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DISCONNECT_LATER_REQ);
}

static void appAvInstanceHandleAvAvrcpPlayStatusChangedInd(avInstanceTaskData *theOtherInst, AV_AVRCP_PLAY_STATUS_CHANGED_IND_T *ind)
{
    /* Look in table to find connected instance */
    for (int instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = AvGetTaskData()->av_inst[instance];
        if (theInst && (theInst != theOtherInst))
        {
            if (appAvIsAvrcpConnected(theInst) && appDeviceIsPeer(&theInst->bd_addr))
            {
                DEBUG_LOG("appAvInstanceHandleAvAvrcpPlayStatusChangedInd, send play status %u to %p", ind->play_status, theInst);
                appAvAvrcpPlayStatusNotification(theInst, ind->play_status);
            }
        }
    }
}

/*
 * Standard TWS:  Set volume from Slave to Master
 * Earbud (Slave) -> Earbud (Master) -> Phone
 */
static void appAvInstanceHandleAvAvrcpVolumeChangedInd(avInstanceTaskData *theInst, AV_AVRCP_VOLUME_CHANGED_IND_T *ind)
{
    assert(theInst == ind->av_instance);
    DEBUG_LOG("appAvInstanceHandleAvAvrcpVolumeChangedInd(%p), volume %u", (void *)theInst, ind->volume);
    /* Set volume and forward to phone if connected */
    appAvVolumeSet(ind->volume, theInst);
}

/*
 * TWS+: Set volume from phone to Earbud.
 * Phone -> Earbud (TWS+)
 *
 * Standard TWS:  Set volume from phone to Master, and from Master to Slave
 * Phone -> Earbud (Master) -> Earbud (Slave)
 */
static void appAvInstanceHandleAvAvrcpSetVolumeInd(avInstanceTaskData *theInst, AV_AVRCP_SET_VOLUME_IND_T *ind)
{
    assert(theInst == ind->av_instance);
    DEBUG_LOG("appAvInstanceHandleAvAvrcpSetVolumeInd(%p), volume %u", (void *)theInst, ind->volume);
    /* Set volume and forward to slave if connected */
    appAvVolumeSet(ind->volume, theInst);
}

void appAvInstanceHandleMessage(Task task, MessageId id, Message message)
{
    avInstanceTaskData *theInst = (avInstanceTaskData *)task;

    if (id >= AV_INTERNAL_AVRCP_BASE && id < AV_INTERNAL_AVRCP_TOP)
        appAvrcpInstanceHandleMessage(theInst, id, message);
    else if (id >= AV_INTERNAL_A2DP_BASE && id < AV_INTERNAL_A2DP_TOP)
        appA2dpInstanceHandleMessage(theInst, id, message);
    else if (id >= AVRCP_MESSAGE_BASE && id < AVRCP_MESSAGE_TOP)
        appAvrcpInstanceHandleMessage(theInst, id, message);
    else if (id >= A2DP_MESSAGE_BASE && id < A2DP_MESSAGE_TOP)
        appA2dpInstanceHandleMessage(theInst, id, message);
    else if (id >= AUDIO_SYNC_BASE && id < AUDIO_SYNC_TOP)
        appA2dpSyncHandleMessage(theInst, id, message);
    else
    {
        switch (id)
        {
            case AV_AVRCP_CONNECT_CFM:
                appAvInstanceHandleAvAvrcpConnectCfm(theInst, (AV_AVRCP_CONNECT_CFM_T *)message);
                break;

            case AV_AVRCP_DISCONNECT_IND:
                appAvInstanceHandleAvAvrcpDisconnectInd(theInst, (AV_AVRCP_DISCONNECT_IND_T *)message);
                break;

            case AV_AVRCP_SET_VOLUME_IND:
                appAvInstanceHandleAvAvrcpSetVolumeInd(theInst, (AV_AVRCP_SET_VOLUME_IND_T *)message);
                break;

            case AV_AVRCP_VOLUME_CHANGED_IND:
                appAvInstanceHandleAvAvrcpVolumeChangedInd(theInst, (AV_AVRCP_VOLUME_CHANGED_IND_T *)message);
                break;

            case AV_AVRCP_PLAY_STATUS_CHANGED_IND:
                appAvInstanceHandleAvAvrcpPlayStatusChangedInd(theInst, (AV_AVRCP_PLAY_STATUS_CHANGED_IND_T *)message);
                break;

            default:
                appAvError(AvGetTaskData(), id, message);
                break;
        }
    }
}

/*! \brief Find AV instance with A2DP state

    This function attempts to find the other AV instance with a matching A2DP state.
    The state is selected using a mask and matching state.

    \param  theInst     AV instance that we want to find a match to
    \param  mask        Mask value to be applied to the a2dp state of a connection
    \param  expected    State expected after applying the mask

    \return Pointer to the AV that matches, NULL if no matching AV was found
*/
avInstanceTaskData *appAvInstanceFindA2dpState(const avInstanceTaskData *theInst,
                                               uint8 mask, uint8 expected)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    PanicFalse(appAvIsValidInst((avInstanceTaskData*)theInst));

    /* Look in table to find entry with matching A2DP state */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theOtherInst = theAv->av_inst[instance];
        if (theOtherInst != NULL && theInst != theOtherInst)
        {
            if ((theOtherInst->a2dp.state & mask) == expected)
            {
                return theOtherInst;
            }
        }
    }

    /* No match found */
    return NULL;
}

/*! \brief Find AV instance for AVRCP passthrough

    This function finds the AV instance to send a AVRCP passthrough command.
    If an AV instance has a Sink SEID as it's last used SEID, then the
    passthrough command should be sent using that instance, otherwise use
    an AV instance with no last used SEID as this will be for an AV source that
    has just paired but hasn't yet connected the A2DP media channel.

    \return Pointer to the AV to use, NULL if no appropriate AV found
*/
avInstanceTaskData *appAvInstanceFindAvrcpForPassthrough(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appAvrcpIsConnected(theInst))
        {
            if (appA2dpIsSinkCodec(theInst))
                return theInst;
            else if (appDeviceIsHandset(&theInst->bd_addr))
                return theInst;
        }
    }

    /* No sink A2DP instance, if there's just one AVRCP, send passthrough on that instance */
    avInstanceTaskData *thePassthoughInstance = NULL;
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appAvrcpIsConnected(theInst))
        {
            /* If no previous instance, remember this one for passthrough */
            if (!thePassthoughInstance)
                thePassthoughInstance = theInst;
            else
            {
                /* Second instance found, can't do passthrough, so exit loop */
                thePassthoughInstance = NULL;
                break;
            }
        }
    }

    return thePassthoughInstance;
}


/*! \brief given a BDADDR, find an AV with a connected AVRCP to that device

    \param bd_addr  Address of bluetooth device to find a connection to

    \return    Pointer to AV data for the AV that is to the requested device
               and has an active avrcp connection.
*/
avInstanceTaskData *appAvInstanceFindAvrcpFromBdAddr(const bdaddr *bd_addr)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && BdaddrIsSame(&theInst->bd_addr, bd_addr))
            return appAvrcpIsConnected(theInst) ? theInst : NULL;
    }

    return NULL;
}

/*! \brief Find a different connected AV instance

    In TWS cases there may be an AV link to a headset as well as the AV to
    the peer device. Finding the other one, given details of 1 link, can be
    useful for forwarding information.

    \param theInst Instance used in check

    \returns NULL if there is no other instance that is connected, otherwise
             returns an AV instance that is connected and not the same as that
             supplied.
 */
avInstanceTaskData *appAvInstanceFindAvrcpOther(avInstanceTaskData *theInst)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    PanicFalse(appAvIsValidInst(theInst));

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theOtherInst = theAv->av_inst[instance];
        if (theOtherInst && theInst != theOtherInst)
            if (appAvrcpIsConnected(theOtherInst))
                return theOtherInst;
    }

    return NULL;

}

/*! \brief Find AV instance with Bluetooth Address

    \note This function returns the AV. It does not check for an
            active connection, or whether A2DP/AVRCP exists.

    \param  bd_addr Bluetooth address to search our AV connections for

    \return Pointer to AV data that matches the bd_addr requested. NULL if
            none was found.
*/
avInstanceTaskData *appAvInstanceFindFromBdAddr(const bdaddr *bd_addr)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find entry with matching Bluetooth address */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && BdaddrIsSame(bd_addr, &theInst->bd_addr))
            return theInst;
    }

    /* No match found */
    return NULL;
}

/*! \brief Query if theInst should connect its media channel.
    \param theInst The AV Instance.
    \param[out] seid If the function returns TRUE, this will be set to the
                stream endpoint ID that should be used on the media channel,
                otherwise AV_SEID_INVALID;
    \return TRUE if theInst should connect its media channel, otherwise FALSE.
    \note Checks if there is another connected media (or streaming) A2DP instance
          with a non-TWS sink codec. Since this instance is deciding whether to
          connect its media channel, it means it doesn't know its SEID yet,
          therefore the SEID category of theInst does not matter.
*/
bool appAvInstanceShouldConnectMediaChannel(const avInstanceTaskData *theInst, uint8 *seid)
{
    *seid = AV_SEID_INVALID;
    avInstanceTaskData *theOtherInst = appAvInstanceFindA2dpState(theInst,
                                                A2DP_STATE_MASK_CONNECTED_MEDIA,
                                                A2DP_STATE_CONNECTED_MEDIA);
    if (theOtherInst && appA2dpIsSinkNonTwsCodec(theOtherInst))
    {
        *seid = appA2dpConvertSeidFromSinkToSource(theOtherInst->a2dp.current_seid);
    }
    return (*seid != AV_SEID_INVALID);
}

/*! \brief Cancel any queued AVRCP disconnect requests

    \param[in] av_inst Instance to cancel queud AVRCP disconnect requests for.
*/
static void appAvAvrcpCancelQueuedDisconnectRequests(avInstanceTaskData *av_inst)
{
    Task task = &av_inst->av_task;
    MessageCancelAll(task, AV_INTERNAL_AVRCP_DISCONNECT_REQ);
    MessageCancelAll(task, AV_INTERNAL_AVRCP_DISCONNECT_LATER_REQ);
}

static void appAvAvrcpConnectLaterRequest(avInstanceTaskData *theInst, uint32 delay)
{
    Task task = &theInst->av_task;

    /* Cancel any queued disconnect requests */
    appAvAvrcpCancelQueuedDisconnectRequests(theInst);

    MessageSendLater(task, AV_INTERNAL_AVRCP_CONNECT_LATER_REQ, NULL, delay);
    DEBUG_LOG("appAvAvrcpConnectLaterRequest(0x%x) delay=%d", theInst, delay);
}

/*! \brief Cancel any queued AVRCP connect requests

    \param[in] av_inst Instance to cancel queued AVRCP connect requests for.
*/
static void appAvAvrcpCancelQueuedConnectRequests(avInstanceTaskData *av_inst)
{
    Task task = &av_inst->av_task;

    if (MessageCancelAll(task, AV_INTERNAL_AVRCP_CONNECT_REQ))
    {
        /* Call ConManagerReleaseAcl to decrement the reference count on the
           ACL that was added when ConManagerCreateAcl was called in
           appAvAvrcpConnectRequest for the queued connect request. */
        ConManagerReleaseAcl(&av_inst->bd_addr);
    }
    MessageCancelAll(task, AV_INTERNAL_AVRCP_CONNECT_LATER_REQ);
}

static void appAvAvrcpDisconnectLaterRequest(avInstanceTaskData *theInst, uint32 delay)
{
    Task task = &theInst->av_task;

    /* Cancel any queued connect requests */
    appAvAvrcpCancelQueuedConnectRequests(theInst);

    MessageSendLater(task, AV_INTERNAL_AVRCP_DISCONNECT_LATER_REQ, NULL, delay);
    DEBUG_LOG("appAvAvrcpDisconnectLaterRequest(0x%x) delay=%d", theInst, delay);
}

/*! \brief Setup A2DP syncronisation of an AV instance with its forwarding peer, if necessary

    Check if the given AV instance needs its A2DP instance to be synchronised
    with a peer AV instance. If it does, setup the link between the two AV
    instances.

    When linked, the peer AV instance will receive AV_INTERNAL_A2DP_INST_SYNC_IND
    messages and must act on them as required to mirror the state of the
    linked AV instance.

    #theInst can be either a handset or a peer AV instance. This function will
    find the other instance it should be sync'd with, if it is connected.

    The peer AV instance must always be synchronised to the handset AV instance,
    hence the direction of the synchronisation depends on whether #theInst is a
    handset or peer instance.

    \param theInst The AV Instance.
*/
static void appAvA2dpSyncEnable(avInstanceTaskData *theInst)
{
    /* Find the other AV instance to sync with */
    avInstanceTaskData *theOtherInst = appAvInstanceFindA2dpState(theInst,
                                                                  A2DP_STATE_MASK_CONNECTED_SIGNALLING,
                                                                  A2DP_STATE_CONNECTED_SIGNALLING);
    if (theOtherInst)
    {
        /* If the handset A2DP has connected and the peer A2DP is already connected,
            or
           If the peer A2DP has connected and the handset A2DP is already connected,
            then
           Register the peer instance with the handset instance. */
        if (appDeviceIsHandset(&theInst->bd_addr) && appDeviceIsPeer(&theOtherInst->bd_addr))
        {
            DEBUG_LOG("appAvA2dpSyncEnable, Handset connected, registering peer");
            appA2dpSyncRegisterInstance(theInst, theOtherInst);
        }
        else if (appDeviceIsPeer(&theInst->bd_addr) && appDeviceIsHandset(&theOtherInst->bd_addr))
        {
            DEBUG_LOG("appAvA2dpSyncEnable, Peer connected, registering peer");
            appA2dpSyncRegisterInstance(theOtherInst, theInst);
        }
    }
}

/*! \brief Disable A2DP synchronisation on an AV instance

    If #theInst is a peer AV instance, un-register it from the handset AV
    instance. If it is a handset AV instance there is no need to un-register
    the peer as it will be done as part of the disconnecting actions.

    \param theInst The AV Instance.
*/
static void appAvA2dpSyncDisable(avInstanceTaskData *theInst)
{
    /* If the peer has disconnected, un-register it from the handset instance */
    avInstanceTaskData *theOtherInst = appAvInstanceFindA2dpState(theInst,
                                                                A2DP_STATE_MASK_CONNECTED_SIGNALLING,
                                                                A2DP_STATE_CONNECTED_SIGNALLING);

    /* Unregister any sync instance registered with this instance */
    appA2dpSyncUnregisterInstance(theInst);

    DEBUG_LOG("VMCSA-2681 appAvA2dpSyncDisable called for instance %p", theInst);
    DEBUG_LOG("VMCSA-2681 appAvA2dpSyncDisable Instance. peer:%d handset:%d",
                            appDeviceIsPeer(&theInst->bd_addr),
                            appDeviceIsHandset(&theInst->bd_addr));
    if (theOtherInst)
    {
        DEBUG_LOG("VMCSA-2681 appAvA2dpSyncDisable Otherinst. peer:%d handset:%d",
                                appDeviceIsPeer(&theOtherInst->bd_addr),
                                appDeviceIsHandset(&theOtherInst->bd_addr));
    }
    else
    {
        DEBUG_LOG("VMCSA-2681 appAvA2dpSyncDisable Otherinst. None");
    }

    /* If this is the peer a2dp instance, un-register from the handset instance
       so that it doesn't try to sync after the peer a2dp instance has gone. */
    if (appDeviceIsPeer(&theInst->bd_addr) && theOtherInst
        && appDeviceIsHandset(&theOtherInst->bd_addr))
    {
        DEBUG_LOG("appAvA2dpSyncDisable, Peer dis-connected, un-registering peer from handset");
        appA2dpSyncUnregisterInstance(theOtherInst);
    }
}

static bool av_A2dpSendConnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, AV_A2DP_CONNECT_CFM_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("av_A2dpSendConnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, AV_A2DP_CONNECT_CFM, msg);

        TaskList_RemoveTask(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_connect_request_clients), task);
    }
    return !found_client_task;
}

static bool av_A2dpSendDisconnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, AV_A2DP_DISCONNECT_CFM_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("av_A2dpSendDisconnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, AV_A2DP_DISCONNECT_CFM, msg);

        TaskList_RemoveTask(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_disconnect_request_clients), task);
    }
    return !found_client_task;
}

static bool av_AvrcpSendConnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, AV_AVRCP_CONNECT_CFM_PROFILE_MANAGER_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("av_AvrcpSendConnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, AV_AVRCP_CONNECT_CFM_PROFILE_MANAGER, msg);

        TaskList_RemoveTask(TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_connect_request_clients), task);
    }
    return !found_client_task;
}

static bool av_AvrcpSendDisconnectCfm(Task task, task_list_data_t *data, void *arg)
{
    bool found_client_task  = FALSE;
    profile_manager_send_client_cfm_params * params = (profile_manager_send_client_cfm_params *)arg;

    if (data->ptr == params->device)
    {
        found_client_task = TRUE;
        MESSAGE_MAKE(msg, AV_AVRCP_DISCONNECT_CFM_T);
        msg->device = params->device;

        if(params->result == profile_manager_success)
        {
            msg->successful = TRUE;
        }
        else
        {
            msg->successful = FALSE;
        }

        DEBUG_LOG("av_AvrcpSendConnectCfm toTask=%x success=%d", task, params->result);
        MessageSend(task, AV_AVRCP_DISCONNECT_CFM, msg);

        TaskList_RemoveTask(TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_disconnect_request_clients), task);
    }
    return !found_client_task;
}

void appAvA2dpSendConnectCfm(avInstanceTaskData *theInst, bool successful)
{
    profile_manager_request_cfm_result_t result = successful ?
                profile_manager_success : profile_manager_failed;

    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_connect_request_clients),
                                            &theInst->bd_addr, result, av_A2dpSendConnectCfm);
}

void appAvAvrcpSendConnectCfmProfileManager(avInstanceTaskData *theInst, bool successful)
{
    profile_manager_request_cfm_result_t result = successful ?
                profile_manager_success : profile_manager_failed;

    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_connect_request_clients),
                                            &theInst->bd_addr, result, av_AvrcpSendConnectCfm);
}

void appAvInstanceA2dpConnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvInstanceA2dpConnected");

    /* Set link supervision timeout 5 seconds */
    //ConnectionSetLinkSupervisionTimeout(appAvGetSink(theInst), 0x1F80);

    /* Update most recent connected device */
    appDeviceUpdateMruDevice(&theInst->bd_addr);

    /* If A2DP was initiated by us, or AVRCP has already been brought up by someone else */
    if (theInst->a2dp.bitfields.local_initiated || appAvrcpIsConnected(theInst))
    {
        DEBUG_LOG("appAvInstanceA2dpConnected, locally initiated, connecting AVRCP");
        appAvAvrcpConnectRequest(&theInst->av_task, &theInst->bd_addr);
    }
    else if (appAvrcpIsDisconnected(theInst))
    {
        DEBUG_LOG("appAvInstanceA2dpConnected, remotely initiated");
        appAvAvrcpConnectLaterRequest(theInst, appConfigAvrcpConnectDelayAfterRemoteA2dpConnectMs());
    }

    appAvVolumeHandleA2dpConnect(theInst);

    /* Check if this instance must be synced with another one */
    appAvA2dpSyncEnable(theInst);

    /* Tell clients we have connected */
    MAKE_AV_MESSAGE(AV_A2DP_CONNECTED_IND);
    message->av_instance = theInst;
    message->bd_addr = theInst->bd_addr;
    message->local_initiated = theInst->a2dp.bitfields.local_initiated;
    appAvSendStatusMessage(AV_A2DP_CONNECTED_IND, message, sizeof(*message));

    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_connect_request_clients),
                                            &theInst->bd_addr, profile_manager_success, av_A2dpSendConnectCfm);
}


void appAvInstanceA2dpDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvInstanceA2dpDisconnected");

    appAvAvrcpDisconnectLaterRequest(theInst, D_SEC(2));

    appAvVolumeHandleAvDisconnect(theInst);

    /* If required, stop any A2DP synchronisation with another AV instance */
    appAvA2dpSyncDisable(theInst);

    if (theInst->a2dp.bitfields.disconnect_reason == AV_A2DP_CONNECT_FAILED)
    {
        ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_connect_request_clients),
                                                &theInst->bd_addr, profile_manager_failed, av_A2dpSendConnectCfm);
    }
    else
    {
        ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_disconnect_request_clients),
                                                &theInst->bd_addr,
                                                profile_manager_success,
                                                av_A2dpSendDisconnectCfm);
    }

    /* Tell clients we have disconnected */
    MAKE_AV_MESSAGE(AV_A2DP_DISCONNECTED_IND);
    message->av_instance = theInst;
    message->bd_addr = theInst->bd_addr;
    message->reason = theInst->a2dp.bitfields.disconnect_reason;
    appAvSendStatusMessage(AV_A2DP_DISCONNECTED_IND, message, sizeof(*message));
}

void appAvInstanceAvrcpConnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvInstanceAvrcpConnected");

    /* Update power table */
    appLinkPolicyUpdatePowerTable(&theInst->bd_addr);

    appAvInstanceStartMediaPlayback(theInst);

    /* Tell clients we have connected */
    MAKE_AV_MESSAGE(AV_AVRCP_CONNECTED_IND);
    message->av_instance = theInst;
    message->bd_addr = theInst->bd_addr;
    message->sink = AvrcpGetSink(theInst->avrcp.avrcp);
    appAvSendStatusMessage(AV_AVRCP_CONNECTED_IND, message, sizeof(*message));

    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_connect_request_clients),
                                            &theInst->bd_addr, profile_manager_success, av_AvrcpSendConnectCfm);
}

void appAvInstanceAvrcpDisconnected(avInstanceTaskData *theInst)
{
    DEBUG_LOG("appAvInstanceAvrcpDisconnected");

    ProfileManager_SendConnectCfmToTaskList(TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_disconnect_request_clients),
                                            &theInst->bd_addr,
                                            profile_manager_success,
                                            av_AvrcpSendDisconnectCfm);

    /* Tell clients we have disconnected */
    MAKE_AV_MESSAGE(AV_AVRCP_DISCONNECTED_IND);
    message->av_instance = theInst;
    message->bd_addr = theInst->bd_addr;
    appAvSendStatusMessage(AV_AVRCP_DISCONNECTED_IND, message, sizeof(*message));
}


/*  \brief Query if theInst should start media streaming on media open confirm.
    \param theInst The AV Instance.
    \return TRUE if theInst should start media streaming, otherwise FALSE.
    \note This function assumes the caller has already inspected its own state
          and determined it is in a state where streaming is possible.
          This function inspects the other instance's state and SEID compatibility.
          There is one case to handle:
            1. theInst is a source SEID, in which case the other instance must be
               streaming and be using a compatible non-TWS sink SEID.
          In all other cases, it is assumed the peer device will initiate streaming.
*/
bool appAvInstanceShouldStartMediaStreamingOnMediaOpenCfm(const avInstanceTaskData *theInst)
{
    /* Find another streaming instance */
    avInstanceTaskData *theOtherInst = appAvInstanceFindA2dpState(theInst,
                                            A2DP_STATE_MASK_CONNECTED_MEDIA_STREAMING,
                                            A2DP_STATE_CONNECTED_MEDIA_STREAMING);
    if (appA2dpIsSourceCodec(theInst))
    {
        return (theOtherInst && appA2dpSeidsAreCompatible(theInst, theOtherInst));
    }
    return FALSE;
}

/*! \brief Query if theInst should start media streaming on media start indication.
    \param theInst The AV Instance.
    \return TRUE if theInst should start media streaming, otherwise FALSE.
    \note This function assumes the caller has already inspected its own state
          and determined it is in a state where streaming is possible.
          This function inspects the other instance's state and SEID compatibility.
          There are a few cases to handle:
            1. theInst is a TWS sink SEID, in which case the other instance must
               not be streaming.
            2. theInst is a standard sink SEID, in which case always start streaming.
			   If there is another instance it will be notified via the audio_sync api.
            3. theInst is a (TWS) source SEID, in which case the other instance
               must be a compatible streaming standard sink SEID.
*/
bool appAvInstanceShouldStartMediaStreamingOnMediaStartInd(const avInstanceTaskData *theInst)
{
    /* Find another streaming instance */
    avInstanceTaskData *theOtherInst = appAvInstanceFindA2dpState(theInst,
                                            A2DP_STATE_MASK_CONNECTED_MEDIA_STREAMING,
                                            A2DP_STATE_CONNECTED_MEDIA_STREAMING);
    if (appA2dpIsSinkTwsCodec(theInst))
    {
        return !theOtherInst;
    }
    else if (appA2dpIsSinkNonTwsCodec(theInst))
    {
        return TRUE;
    }
    else if (appA2dpIsSourceCodec(theInst))
    {
        return theOtherInst && appA2dpSeidsAreCompatible(theInst, theOtherInst);
    }
    return FALSE;
}


/*! \brief Check if AVRCP is connected for AV usage

    \param[in]  theInst The AV Instance to be checked for an AVRCP connection.

    \return     TRUE if the AV task of this instance is registered for AVRCP messages
*/
bool appAvIsAvrcpConnected(avInstanceTaskData *theInst)
{
    return appAvrcpIsValidClient(theInst, &theInst->av_task);
}

/*! \brief provides AV(media player) current context to ui module

    \param[in]  void

    \return     current context of av module.
*/
unsigned AvGetCurrentContext(void)
{
    av_provider_context_t context = BAD_CONTEXT;

    if (appAvIsDisconnected())
    {
        context = context_av_disconnected;
    }
    else
    {
        if(appAvIsStreaming())
        {
            context = context_av_is_streaming;
        }
        else if (Av_IsA2dpConnected())
        {
            context = context_av_connected;
        }
    }
    return (unsigned)context;
}

/*! \brief Create AV instance for A2DP sink or source

    Creates an AV instance entry for the bluetooth address supplied (bd_addr).

    \note No check is made for there already being an instance
    for this address.

    \param bd_addr Address the created instance will represent

    \return Pointer to the created instance, or NULL if it was not
        possible to create

*/
avInstanceTaskData *appAvInstanceCreate(const bdaddr *bd_addr)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find empty entry */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        if (theAv->av_inst[instance] == NULL)
        {
            avInstanceTaskData *theInst;

            /* Allocate new instance */
            theInst = PanicUnlessNew(avInstanceTaskData);
            theAv->av_inst[instance] = theInst;

            DEBUG_LOG("appAvInstanceCreate %p", theInst);

            /* Initialise instance */
            appA2dpInstanceInit(theInst, theAv->suspend_state);
            appAvrcpInstanceInit(theInst);

            /* Set up task handler */
            theInst->av_task.handler = appAvInstanceHandleMessage;

            /* Set Bluetooth address of remote device */
            theInst->bd_addr = *bd_addr;

            /* Initially not synced to another AV instance */
            appA2dpSyncInitialise(theInst);

            /* Tell clients we have created new instance */
            appAvSendStatusMessage(AV_CREATE_IND, NULL, 0);

            /* Return pointer to new instance */
            return theInst;
        }
    }

    /* No free entry in table */
    return NULL;
}

/*! \brief Check whether there is an A2DP lock set

    \return TRUE is there is and A2DP lock pending, else FALSE
*/
static bool appAvA2dpLockPending(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;
    bool result = FALSE;
    avInstanceTaskData *theInst;
    
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        theInst = theAv->av_inst[instance];
        if (theInst && (appA2dpGetLock(theInst) & APP_A2DP_AUDIO_STOP_LOCK))
        {
            DEBUG_LOG("appAvA2dpLockPending %i %p %d",
                instance, &appA2dpGetLock(theInst), appA2dpGetLock(theInst));
            MessageSendConditionally(&theInst->av_task,
                AV_INTERNAL_A2DP_DESTROY_REQ, NULL, &appA2dpGetLock(theInst));
            result = TRUE;
            break;
        }
    }
    return result;
}

/*! \brief Destroy AV instance for A2DP sink or source

    This function should only be called if the instance no longer has
    either a connected A2DP or a connected AVRCP.  If either is still
    connected, the function will silently fail.

    The function will panic if theInst is not valid, for instance
    if already destroyed.

    \param  theInst The instance to destroy

*/
void appAvInstanceDestroy(avInstanceTaskData *theInst)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    DEBUG_LOG("appAvInstanceDestroy(%p)", theInst);

    PanicFalse(appAvIsValidInst(theInst));

    /* Destroy instance only both state machines are disconnected and there is no A2DP lock pending */
    if (appA2dpIsDisconnected(theInst) && appAvrcpIsDisconnected(theInst) && !appAvA2dpLockPending())
    {
        DEBUG_LOG("appAvInstanceDestroy(%p) permitted", theInst);

        /* Check there are no A2DP & AVRCP profile library instances */
        PanicFalse(theInst->a2dp.device_id == INVALID_DEVICE_ID);
        PanicNotNull(theInst->avrcp.avrcp);

        /* Clear client lists */
        if (theInst->avrcp.client_list)
        {
            TaskList_Destroy(theInst->avrcp.client_list);
            theInst->avrcp.client_list = NULL;
        }

        /* Flush any messages still pending delivery */
        MessageFlushTask(&theInst->av_task);

        /* Look in table to find matching entry */
        for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
        {
            if (theAv->av_inst[instance] == theInst)
            {
                /* Clear entry and free instance */
                theAv->av_inst[instance] = NULL;
                free(theInst);

                /* Tell clients we have destroyed instance */
                appAvSendStatusMessage(AV_DESTROY_IND, NULL, 0);
                return;
            }
        }

        /* No match found, fatal error! */
        Panic();
    }
    else
    {
        DEBUG_LOG("appAvInstanceDestroy(%p) A2DP (%d) or AVRCP (%d) not disconnected, or A2DP Lock Pending",
                   theInst, !appA2dpIsDisconnected(theInst), !appAvrcpIsDisconnected(theInst));
    }
}

/*! \brief Return AV instance for A2DP sink

    This function walks through the AV instance table looking for the
    first instance which is a connected sink that can use the
    specified codec.

    \param codec_type   Codec to look for

    \return Pointer to AV information for a connected source,NULL if none
        was found
*/
avInstanceTaskData *appAvGetA2dpSink(avCodecType codec_type)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find connected instance with sink SEID */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];

        /* Note that the sink SEID is also know when in state
           A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC. This is required because the
           audio sync code sends AUDIO_SYNC_CONNECT_IND when the remote device
           initiates media channel connection and A2DP is therefore in the state
           A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC. Audio sync clients can request
           media connect parameters at this point, so accessing instance is
           required when in this state.
        */
        if (theInst && (appA2dpIsStateConnectedMedia(theInst->a2dp.state) ||
                        theInst->a2dp.state == A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC))
        {
            switch (codec_type)
            {
                case AV_CODEC_ANY:
                    if (appA2dpIsSinkCodec(theInst))
                        return theInst;
                    break;

                case AV_CODEC_TWS:
                    if (appA2dpIsSinkTwsCodec(theInst))
                        return theInst;
                    break;

                case AV_CODEC_NON_TWS:
                    if (appA2dpIsSinkNonTwsCodec(theInst))
                        return theInst;
                    break;
            }
        }
    }

    /* No sink found so return NULL */
    return NULL;
}

/*! \brief Return AV instance for A2DP source

    This function walks through the AV instance table looking for the
    first instance which is a connected source.

    \return Pointer to AV information for a connected source,NULL if none
            was found
*/
avInstanceTaskData *appAvGetA2dpSource(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Look in table to find connected instance with source SEID */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsStateConnectedMedia(theInst->a2dp.state) && appA2dpIsSourceCodec(theInst))
            return theInst;
    }

    /* No sink found so return NULL */
    return NULL;
}

/*! \brief Entering `Initialising A2DP` state

    This function is called when the AV state machine enters
    the 'Initialising A2DP' state, it calls the A2dpInit() function
    to initialise the A2DP profile library and register the SEPs.
*/

static void appAvEnterInitialisingA2dp(avTaskData *theAv)
{
    /* Initialise A2DP role */
    uint16 role = avConfigA2dpSourceEnabled() ? A2DP_INIT_ROLE_SINK | A2DP_INIT_ROLE_SOURCE : A2DP_INIT_ROLE_SINK;
    
#ifdef INCLUDE_APTX_ADAPTIVE   
    bool enable_adaptive = FeatureVerifyLicense(APTX_ADAPTIVE_DECODE);
#ifdef INCLUDE_MIRRORING
    enable_adaptive |= FeatureVerifyLicense(APTX_ADAPTIVE_MONO_DECODE);
#endif
    /* Initialise the structure used by adaptive */
    A2dpProfileAptxAdInitServiceCapability();
#endif

    /* Initialise the Stream Endpoints... */
    sep_data_type seps[] = {
        /* Standard sinks */
        { .sep_config = &av_aptx_snk_sep,
          .in_use = (FeatureVerifyLicense(APTX_CLASSIC_MONO) && appConfigAptxEnabled()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_aac_snk_sep,
          .in_use = appConfigAacEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_sbc_snk_sep,
          .in_use = 0,
        },
#ifdef INCLUDE_APTX_ADAPTIVE
        { .sep_config = &av_aptx_adaptive_snk_sep,
          .in_use = (enable_adaptive && appConfigAptxAdaptiveEnabled()) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif
        /* TWS sinks */
        { .sep_config = &av_aptx_mono_tws_snk_sep,
          .in_use = FeatureVerifyLicense(APTX_CLASSIC_MONO) ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_aac_stereo_tws_snk_sep,
          .in_use = 0,
        },
        { .sep_config = &av_sbc_mono_tws_snk_sep,
          .in_use = 0,
        },
#ifdef INCLUDE_APTX_ADAPTIVE_TWS
        { .sep_config = &av_aptx_adaptive_tws_snk_sep,
          .in_use = FeatureVerifyLicense(APTX_ADAPTIVE_MONO) && appConfigAptxAdaptiveEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
#endif
        /* TWS sources */
        { .sep_config = &av_aptx_mono_tws_src_sep,
          .in_use = FeatureVerifyLicense(APTX_CLASSIC_MONO) && avConfigA2dpSourceEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_aac_stereo_tws_src_sep,
          .in_use = avConfigA2dpSourceEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        { .sep_config = &av_sbc_mono_tws_src_sep,
          .in_use = avConfigA2dpSourceEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
        /* PTS sources */
        { .sep_config = &av_sbc_src_sep,
          .in_use = avConfigA2dpSourceEnabled() ? 0 : A2DP_SEP_UNAVAILABLE,
        },
    };
    DEBUG_LOG("appAvEnterInitialisingA2dp");
    
    /* Initialise the A2DP Library */
    A2dpInit(&theAv->task, role, 0, ARRAY_DIM(seps), seps, 0);
}

/*! \brief Entering `Initialising AVRCP` state

    This function is called when the AV state machine enters
    the 'Initialising AVRCP' state, it calls the AvrcpInit() function
    to initialise the AVRCP profile library.
*/
static void appAvEnterInitialisingAvrcp(avTaskData *theAv)
{
    const avrcp_init_params avrcpConfig =
    {
        avrcp_target_and_controller,
        AVRCP_CATEGORY_1,
        AVRCP_CATEGORY_2 | AVRCP_CATEGORY_1,
        AVRCP_VERSION_1_6
    };

    DEBUG_LOG("appAvEnterInitialisingAvrcp");

    /* Go ahead and initialise the AVRCP library */
    AvrcpInit(&theAv->task, &avrcpConfig);
}

/*! \brief Set AV FSM state

    Called to change state.  Handles calling the state entry and exit
    functions for the new and old states.
*/
static void appAvSetState(avTaskData *theAv, avState state)
{
    DEBUG_LOG("appAvSetState(%d)", state);

    /* Set new state */
    theAv->bitfields.state = state;

    /* Handle state entry functions */
    switch (state)
    {
        case AV_STATE_INITIALISING_A2DP:
            appAvEnterInitialisingA2dp(theAv);
            break;

        case AV_STATE_INITIALISING_AVRCP:
            appAvEnterInitialisingAvrcp(theAv);
            break;

        default:
            break;
    }

    /* Set new state */
    theAv->bitfields.state = state;
}

/*! \brief Set AV FSM state

    Returns current state of the AV FSM.
*/
static avState appAvGetState(avTaskData *theAv)
{
    return theAv->bitfields.state;
}

/*! \brief Handle A2DP Library initialisation confirmation

    Check status of A2DP Library initialisation if successful store the SEP
    list for later use and move to the 'Initialising AVRCP' state to kick
    of AVRCP Library initialisation.
*/
static void appAvHandleA2dpInitConfirm(avTaskData *theAv, const A2DP_INIT_CFM_T *cfm)
{
    DEBUG_LOG("appAvHandleA2dpInitConfirm");

    /* Check if A2DP initialised successfully */
    if (cfm->status == a2dp_success)
    {
        /* Move to 'Initialising AVRCP' state */
        appAvSetState(theAv, AV_STATE_INITIALISING_AVRCP);
    }
    else
        Panic();
}

/*! \brief Handle AVRCP Library initialisation confirmation

    Check status of AVRCP Library initialisation if successful inform the main
    task that AV initialisation has completed and move into the 'Active' state.
*/
static void appAvHandleAvrcpInitConfirm(avTaskData *theAv, const AVRCP_INIT_CFM_T *cfm)
{
    DEBUG_LOG("appAvHandleAvrcpInitConfirm");

    /* Check if AVRCP successfully initialised */
    if (cfm->status == avrcp_success)
    {
        /* Tell main application task we have initialised */
        MessageSend(Init_GetInitTask(), AV_INIT_CFM, NULL);

        /* Change to 'idle' state */
        appAvSetState(theAv, AV_STATE_IDLE);
    }
    else
        Panic();
}



/*! \brief Handle indication of change in a connection status.

    Some phones will disconnect the ACL without closing any L2CAP/RFCOMM
    connections, so we check the ACL close reason code to determine if this
    has happened.

    If the close reason code was not link-loss and we have an AV profiles
    on that link, mark it as detach pending, so that we can gracefully handle
    the L2CAP or RFCOMM disconnection that will follow shortly.
 */
static void appAvHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *ind)
{
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(&ind->bd_addr);
    if (theInst)
    {
        if (!ind->connected && !ind->ble)
        {
            if (ind->reason != hci_error_conn_timeout)
            {
                DEBUG_LOG("appAvHandleConManagerConnectionInd, detach pending");
                theInst->detach_pending = TRUE;
            }
        }
    }
}

static void initAvVolume(audio_source_t source)
{
    AudioSources_RegisterVolume(source, A2dpProfile_GetAudioSourceVolumeInterface());
    AudioSources_RegisterObserver(source, AvrcpProfile_GetObserverInterface());
}

void Av_SetupForPrimaryRole(void)
{
    DEBUG_LOG("Av_SetupForPrimaryRole");
    AudioSources_RegisterAudioInterface(audio_source_a2dp_1, A2dpProfile_GetHandsetSourceAudioInterface());
}

void Av_SetupForSecondaryRole(void)
{
    DEBUG_LOG("Av_SetupForSecondaryRole");
    AudioSources_RegisterAudioInterface(audio_source_a2dp_1, A2dpProfile_GetPeerSourceAudioInterface());
}

/*! \brief Initialise AV task

    Called at start up to initialise the main AV task..
    Clear the AV instance table and initialises the state-machine.
*/
bool appAvInit(Task init_task)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* Set up task handler */
    theAv->task.handler = appAvHandleMessage;

    /* Default to unset volume. The volume will be set when A2DP or AVRCP connects. */
    theAv->volume = VOLUME_UNSET;

    /* Clear instance table */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        theAv->av_inst[instance] = NULL;
    }

    /* Initialise state */
    theAv->suspend_state = 0;
    theAv->bitfields.state = AV_STATE_NULL;
    appAvSetState(theAv, AV_STATE_INITIALISING_A2DP);

    /* Initialise client lists */
    TaskList_Initialise(&theAv->avrcp_client_list);
    TaskList_Initialise(&theAv->av_status_client_list);
    TaskList_Initialise(&theAv->av_ui_client_list);
    appAvAvrcpClientRegister(&theAv->task, 0);

    /* Create lists for connection/disconnection requests */
    TaskList_WithDataInitialise(&theAv->a2dp_connect_request_clients);
    TaskList_WithDataInitialise(&theAv->a2dp_disconnect_request_clients);
    TaskList_WithDataInitialise(&theAv->avrcp_connect_request_clients);
    TaskList_WithDataInitialise(&theAv->avrcp_disconnect_request_clients);

    /* Register to receive notifications of (dis)connections */
    ConManagerRegisterConnectionsClient(&theAv->task);

    Init_SetInitTask(init_task);

    Av_SetupForPrimaryRole();
    initAvVolume(audio_source_a2dp_1);

    Ui_RegisterUiInputConsumer(appGetAvPlayerTask(), av_ui_inputs, ARRAY_DIM(av_ui_inputs));

    ProfileManager_RegisterProfile(profile_manager_a2dp_profile, Av_A2dpConnectWithBdAddr, Av_A2dpDisconnectWithBdAddr);
    ProfileManager_RegisterProfile(profile_manager_avrcp_profile, Av_AvrcpConnectRequest, Av_AvrcpDisconnectRequest);

    return TRUE;
}


/*! \brief Register a task to receive avrcp messages from the av module

    \note This function can be called multiple times for the same task. It
      will only appear once on a list.

    \param  client_task Task to be added to the list of registered clients
    \param  interests   Not used at present
 */
void appAvAvrcpClientRegister(Task client_task, uint8 interests)
{
    UNUSED(interests);

    /* Add client task to list */
    TaskList_AddTask(&AvGetTaskData()->avrcp_client_list, client_task);
}


/*! \brief Register a task to receive AV status messages

    \note This function can be called multiple times for the same task. It
      will only appear once on a list.

    \param  client_task Task to be added to the list of registered clients
 */
void appAvStatusClientRegister(Task client_task)
{
    TaskList_AddTask(&AvGetTaskData()->av_status_client_list, client_task);
}

/*! \brief Function to send a status message to AV's status clients.
    \param id The message ID to send.
    \param msg The message content.
    \param size The size of the message.
*/
void appAvSendStatusMessage(MessageId id, void *msg, size_t size)
{
    TaskList_MessageSendWithSize(&AvGetTaskData()->av_status_client_list, id, msg, size);
}

/*! \brief Register a task to receive AV UI messages

    \note This function can be called multiple times for the same task. It
      will only appear once on a list.

    \param  client_task Task to be added to the list of registered clients
 */
void appAvUiClientRegister(Task client_task)
{
    TaskList_AddTask(&AvGetTaskData()->av_ui_client_list, client_task);
}

/*! \brief Function to send a UI message to AV's UI clients.
    \param id The message ID to send.
    \param msg The message content.
    \param size The size of the message.
*/
void appAvSendUiMessage(MessageId id, void *msg, size_t size)
{
    TaskList_MessageSendWithSize(&AvGetTaskData()->av_ui_client_list, id, msg, size);
}

/*! \brief Function to send a UI message without message content to AV's UI clients.
    \param id The message ID to send.
*/
void appAvSendUiMessageId(MessageId id)
{
     appAvSendUiMessage(id, NULL, 0);
}

/*! \brief Connect A2DP to a specific Bluetooth device

    This function requests an A2DP connection. If there is no AV entry
    for the device, it will be created. If the AV already exists any
    pending link destructions for AVRCP and A2DP will be cancelled.

    If there is an existing A2DP connection, then the function will
    return FALSE.

    \param  bd_addr     Bluetooth address of device to connect to
    \param  a2dp_flags  Flags to apply to connection. Can be combined as a bitmask.

    \return TRUE if the connection has been requested. FALSE otherwise, including
        when a connection already exists.
*/
bool appAvA2dpConnectRequest(const bdaddr *bd_addr, appAvA2dpConnectFlags a2dp_flags)
{
    avInstanceTaskData *theInst;



    /* Check if AV instance to this device already exists */
    theInst = appAvInstanceFindFromBdAddr(bd_addr);
    if (theInst == NULL)
    {
        /* No AV instance for this device, so create new instance */
        theInst = appAvInstanceCreate(bd_addr);
    }
    else
    {
        /* Make sure there's no pending destroy message */
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);
    }

    /* Now check we have an AV instance */
    if (theInst)
    {
        /* Check A2DP is not already connected */
        if (!appA2dpIsConnected(theInst))
        {
            /* Send AV_INTERNAL_A2DP_CONNECT_REQ to start A2DP connection */
            MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_REQ);

            DEBUG_LOG("appAvConnectWithBdAddr A2DP, %p, %x %x %lx",
                         (void *)theInst, bd_addr->nap, bd_addr->uap, bd_addr->lap);

            /* Send message to newly created AV instance to connect A2DP */
            message->num_retries = 2;
            message->flags = (unsigned)a2dp_flags;
            MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ);
            MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_REQ, message,
                                     ConManagerCreateAcl(&theInst->bd_addr));

            return TRUE;
        }
    }

    return FALSE;
}

/*! \brief Connect A2DP Media to a specific Bluetooth device

    This function requests an A2DP Media connection.

    If there is not an existing A2DP connection, then the function will
    return FALSE.

    \param  bd_addr     Bluetooth address of device to connect to

    \return TRUE if the meida connection has been requested. FALSE otherwise,
    including when a connection doesn't exists.
*/
bool appAvA2dpMediaConnectRequest(const bdaddr *bd_addr)
{
    avInstanceTaskData *theInst;

    /* Check if AV instance to this device already exists */
    theInst = appAvInstanceFindFromBdAddr(bd_addr);
    if (theInst != NULL)
    {
        /* Check A2DP is already connected */
        if (appA2dpIsConnected(theInst))
        {
            /* Connect media channel */
            MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);
            message->seid = AV_SEID_INVALID;
            MessageCancelFirst(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);
            MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ, message,&appA2dpGetLock(theInst));
            return TRUE;
        }
    }

    return FALSE;
}

/*! \brief Connect AVRCP to a specific Bluetooth device

    This function requests an AVRCP connection.
    If there is no AV entry for the device, it will be created. No check is
    made for an existing AVRCP connection, but if the AV already exists any
    pending link destructions for AVRCP and A2DP will be cancelled.

    If the function returns TRUE, then the client_task should receive an
    AV_AVRCP_CONNECT_CFM message whether the connection succeeds or not. See note.

    \note If there was no existing AV entry for the device, and hence no ACL,
    then the AV_AVRCP_CONNECT_CFM message will not be sent if the ACL could not
    be created.

    \param  client_task Task to receive response messages
    \param  bd_addr     Bluetooth address of device to connect to

    \return TRUE if the connection has been requested, FALSE otherwise
*/
bool appAvAvrcpConnectRequest(Task client_task, const bdaddr *bd_addr)
{
    avInstanceTaskData *theInst;

    /* Check if AV instance to this device already exists */
    theInst = appAvInstanceFindFromBdAddr(bd_addr);
    if (theInst == NULL)
    {
        /* No AV instance for this device, so create new instance */
        theInst = appAvInstanceCreate(bd_addr);
    }
    else
    {
        /* Make sure there's no pending disconnect/destroy message */
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ);
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ);
        if (client_task == &theInst->av_task)
        {
            MessageCancelAll(&theInst->av_task, AV_INTERNAL_AVRCP_DISCONNECT_LATER_REQ);
        }
    }

    /* Now check we have an AV instance */
    if (theInst)
    {
        /* Send AV_INTERNAL_AVRCP_CONNECT_REQ to start AVRCP connection */
        MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_CONNECT_REQ);

        DEBUG_LOG("appAvConnectWithBdAddr AVRCP, %p, %x %x %lx",
                     (void *)theInst, bd_addr->nap, bd_addr->uap, bd_addr->lap);

        /* Send message to newly created AV instance to connect AVRCP */
        message->client_task = client_task;
        MessageCancelFirst(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_REQ);
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_CONNECT_REQ, message,
                                 ConManagerCreateAcl(&theInst->bd_addr));

        return TRUE;
    }

    return FALSE;
}

static void av_CancelQueuedAvrcpDisconnectRequests(const bdaddr* bd_addr)
{
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(bd_addr);
    if (theInst != NULL)
    {
        appAvAvrcpCancelQueuedDisconnectRequests(theInst);
    }
}

void Av_AvrcpConnectRequest(Task client_task, bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        task_list_t * req_task_list = TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_connect_request_clients);

        av_CancelQueuedAvrcpDisconnectRequests(bd_addr);

        ProfileManager_AddRequestToTaskList(req_task_list, device, client_task);
        if (!appAvAvrcpConnectRequest(client_task, bd_addr))
        {
            /* If AVRCP is already connected send a connect cfm */
            ProfileManager_SendConnectCfmToTaskList(req_task_list, bd_addr, profile_manager_success, av_AvrcpSendConnectCfm);
        }
    }
}

void Av_AvrcpDisconnectRequest(Task client_task, bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        task_list_t * req_task_list = TaskList_GetBaseTaskList(&AvGetTaskData()->avrcp_disconnect_request_clients);
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(bd_addr);

        ProfileManager_AddRequestToTaskList(req_task_list, device, client_task);
        if (!appAvAvrcpDisconnectRequest(client_task, theInst))
        {
            /* If AVRCP is already disconnected send a disconnect cfm */
            ProfileManager_SendConnectCfmToTaskList(req_task_list, bd_addr, profile_manager_success, av_AvrcpSendDisconnectCfm);
        }
    }
}

/*! \brief Application response to a connection request

    After a connection request has been received and processed by the
    application, this function should be called to accept or reject the
    request.

    \param[in] ind-task       Task that received ind, that will no longer receive subsequent messages
    \param[in] client_task    Task responding, that will receive subsequent messages
    \param[in] bd_addr        Bluetooth address of connected device
    \param     connection_id  Connection ID
    \param     signal_id      Signal ID
    \param     accept         Whether to accept or reject the connection
 */
void appAvAvrcpConnectResponse(Task ind_task, Task client_task, const bdaddr *bd_addr,
                               uint16 connection_id, uint16 signal_id, avAvrcpAccept accept)
{
    avInstanceTaskData *av_inst;

    /* Get AV instance for this device */
    av_inst = appAvInstanceFindFromBdAddr(bd_addr);
    if (av_inst)
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_CONNECT_RES);
        message->ind_task = ind_task;
        message->client_task = client_task;
        message->accept = accept;
        message->connection_id = connection_id;
        message->signal_id = signal_id;
        MessageSend(&av_inst->av_task, AV_INTERNAL_AVRCP_CONNECT_RES, message);
    }
    else
    {
        Panic();
    }
}


/*! \brief Request disconnection of A2DP from specified AV

    \param[in] av_inst  Instance to disconnect A2DPfrom

    \return TRUE if disconnect has been requested
 */
bool appAvA2dpDisconnectRequest(avInstanceTaskData *av_inst)
{
    if (av_inst)
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_DISCONNECT_REQ);
        message->flags = 0;
        PanicFalse(appAvIsValidInst(av_inst));
        MessageSendConditionally(&av_inst->av_task, AV_INTERNAL_A2DP_DISCONNECT_REQ,
                                 message, &appA2dpGetLock(av_inst));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*! \brief Request disconnection of AVRCP notifying the specified client.

    \param[in] client_task  Task to receive disconnect confirmation
    \param[in] av_inst      Instance to disconnect AVRCP from

    \return TRUE if disconnect has been requested
 */
bool appAvAvrcpDisconnectRequest(Task client_task, avInstanceTaskData *av_inst)
{
    if (av_inst)
    {
        /* Cancel any queued connect messages and release the lock */
        appAvAvrcpCancelQueuedConnectRequests(av_inst);

        MAKE_AV_MESSAGE(AV_INTERNAL_AVRCP_DISCONNECT_REQ);
        message->client_task = client_task;
        PanicFalse(appAvIsValidInst(av_inst));
        MessageSendConditionally(&av_inst->av_task, AV_INTERNAL_AVRCP_DISCONNECT_REQ,
                                 message, &appAvrcpGetLock(av_inst));

        DEBUG_LOG("appAvAvrcpDisconnectRequest(0x%x)", client_task);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*! \brief Connect to peer device

    \return TRUE if a connection was requested, FALSE in all other cases
      including when there is already a connection
 */
bool appAvConnectPeer(const bdaddr* peer_addr)
{

    return appAvA2dpConnectRequest(peer_addr, A2DP_CONNECT_NOFLAGS);
}

/*! \brief Disconnect AVRCP and A2DP from the peer earbud.

    \return TRUE if a disconnection was requested and is pending, FALSE
        otherwise.
*/
bool appAvDisconnectPeer(const bdaddr* peer_addr)
{
    avInstanceTaskData *av_inst = appAvInstanceFindFromBdAddr(peer_addr);
    bool a2dp_pending = appAvA2dpDisconnectRequest(av_inst);
    bool avrcp_pending = appAvAvrcpDisconnectRequest(&av_inst->av_task, av_inst);
    return (a2dp_pending || avrcp_pending);
}


/*! \brief Connect to a handset

    This connects to the first handset from device manager if that handset
    supports A2DP (which is likely).

    \param play AV will attempt to start media playback after AV connects

    \return TRUE if a connection was requested, FALSE in all other cases
      including when there is already a connection
 */
bool appAvConnectHandset(bool play)
{
    bdaddr bd_addr = {0};

    /* Get handset device address */
    if (appDeviceGetHandsetBdAddr(&bd_addr) && BtDevice_IsProfileSupported(&bd_addr, DEVICE_PROFILE_A2DP))
    {
        appAvA2dpConnectFlags flags = A2DP_CONNECT_MEDIA;

        /* Clear flag that sets A2DP_START_MEDIA_PLAYBACK if connecting by bdaddr */
        appAvPlayOnHandsetConnection(FALSE);

        if (play)
        {
            flags |= A2DP_START_MEDIA_PLAYBACK;
        }

        return appAvA2dpConnectRequest(&bd_addr, flags);
    }

    return FALSE;
}

/*! \brief Connect the A2dp Media channel on a connected handset

    This connects the A2dp Media channel on an already connectd handest.

    \return TRUE if a connection was requested, FALSE in all other cases
      including when there is not a connection
 */
bool appAvConnectHandsetA2dpMedia(void)
{
    bdaddr bd_addr = {0};

    /* Get handset device address */
    if (appDeviceGetHandsetBdAddr(&bd_addr) && BtDevice_IsProfileSupported(&bd_addr, DEVICE_PROFILE_A2DP))
    {
        return appAvA2dpMediaConnectRequest(&bd_addr);
    }

    return FALSE;
}

/*! \brief Disconnect AVRCP and A2DP from the handset.

    Disconnect any AVRCP or A2DP connections from the handset.

    \return TRUE if a disconnection was requested and is pending, FALSE
        otherwise.
 */
bool appAvDisconnectHandset(void)
{
    bdaddr bd_addr = {0};

    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        avInstanceTaskData *av_inst = appAvInstanceFindFromBdAddr(&bd_addr);
        return appAvA2dpDisconnectRequest(av_inst) || appAvAvrcpDisconnectRequest(&av_inst->av_task, av_inst);
    }
    else
    {
        return FALSE;
    }
}

/*! \brief Initiate an AV connection to a Bluetooth address

    AV connections are started with an A2DP connection.

    \param  bd_addr Address to connect to

    \return TRUE if a connection has been initiated. FALSE in all other cases
            included if there is an existing connection.
 */
bool appAvConnectWithBdAddr(const bdaddr *bd_addr)
{
    /* See if known */
    if (BtDevice_isKnownBdAddr(bd_addr))
    {

        return appAvA2dpConnectRequest(bd_addr, A2DP_CONNECT_MEDIA);
    }
    else
    {
        return FALSE;
    }
}

static void av_CancelQueuedA2dpDisconnectRequests(const bdaddr* bd_addr)
{
    avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(bd_addr);
    if (theInst != NULL)
    {
        MessageCancelAll(&theInst->av_task, AV_INTERNAL_A2DP_DISCONNECT_REQ);
    }
}

/*! \brief Initiate an AV connection to a Bluetooth address

    AV connections are started with an A2DP connection.

    \param client_task the Task that shall recieve the CONNECT_CFM message
    \param bd_addr Address to connect to
 */
void Av_A2dpConnectWithBdAddr(const Task client_task, bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        task_list_t * req_task_list = TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_connect_request_clients);
        appAvA2dpConnectFlags connect_flags = A2DP_CONNECT_MEDIA;
        avTaskData *theAv = AvGetTaskData();

        av_CancelQueuedA2dpDisconnectRequests(bd_addr);

        ProfileManager_AddRequestToTaskList(req_task_list, device, client_task);
        if (theAv->play_on_connect && appDeviceIsHandset(bd_addr))
        {
            connect_flags |= A2DP_START_MEDIA_PLAYBACK;
            theAv->play_on_connect = FALSE;
        }

        if (!appAvA2dpConnectRequest(bd_addr, connect_flags))
        {
            /* If A2DP is already connected send a connect cfm */
            ProfileManager_SendConnectCfmToTaskList(req_task_list, bd_addr, profile_manager_success, av_A2dpSendConnectCfm);
        }
    }
}

void Av_A2dpDisconnectWithBdAddr(const Task client_task, bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        task_list_t * req_task_list = TaskList_GetBaseTaskList(&AvGetTaskData()->a2dp_disconnect_request_clients);
        avInstanceTaskData *theInst = appAvInstanceFindFromBdAddr(bd_addr);

        ProfileManager_AddRequestToTaskList(req_task_list, device, client_task);
        if (!appAvA2dpDisconnectRequest(theInst))
        {
            /* If A2DP is already disconnected send a disconnect cfm */
            ProfileManager_SendConnectCfmToTaskList(req_task_list, bd_addr, profile_manager_success, av_A2dpSendDisconnectCfm);
        }
    }
}

/*! \brief Suspend AV link

    This function is called whenever a module in the headset has a reason to
    suspend AV streaming.  An internal message is sent to every AV
    instance, if the instance is currently streaming it will attempt to
    suspend.

    \note There may be multple reasons that streaming is suspended at any time.

    \param  reason  Why streaming should be suspended. What activity has started.
*/

void appAvStreamingSuspend(avSuspendReason reason)
{
    int instance;
    avTaskData *theAv = AvGetTaskData();
    unsigned suspend_state_pre = theAv->suspend_state;
    DEBUG_LOG("appAvStreamingSuspend(0x%x, 0x%x)", suspend_state_pre, reason);

    /* Update suspend state for any newly created AV instances */
    theAv->suspend_state |= reason;

    /* Only send suspend messages if the suspend state has changed */
    if (theAv->suspend_state != suspend_state_pre)
    {
        /* Send suspend to all instances in the table */
        for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
        {
            avInstanceTaskData *av_inst = theAv->av_inst[instance];
            if (av_inst != NULL)
            {
                MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);

                /* Send message to AV instance */
                message->reason = reason;
                MessageSendConditionally(&av_inst->av_task, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ,
                                         message, &appA2dpGetLock(av_inst));
            }
        }
    }
}

/*! \brief Resume AV link

    This function is called whenever a module in the headset has cleared it's
    reason to suspend AV streaming.  An internal message is sent to every AV
    instance.

    \note There may be multple reasons why streaming is currently suspended.
      All suspend reasons have to be cleared before the AV instance will
      attempt to resume streaming.

    \param  reason  Why streaming can now be resumed. What activity has completed.
*/
void appAvStreamingResume(avSuspendReason reason)
{
    int instance;
    avTaskData *theAv = AvGetTaskData();
    unsigned suspend_state_pre = theAv->suspend_state;
    DEBUG_LOG("appAvStreamingResume(0x%x, 0x%x)", suspend_state_pre, reason);

    /* Update suspend state for any newly created AV instances */
    theAv->suspend_state &= ~reason;

    /* Only send resume messages if the suspend state has changed */
    if (theAv->suspend_state != suspend_state_pre)
    {
        /* Send suspend to all instances in the table */
        for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
        {
            avInstanceTaskData *av_inst = theAv->av_inst[instance];
            if (av_inst != NULL)
            {
                MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);

                /* Send message to AV instance */
                message->reason = reason;
                MessageSendConditionally(&av_inst->av_task, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ,
                                        message, &appA2dpGetLock(av_inst));
            }
        }
    }
}

/*! \brief Check if an instance is valid

    Checks if the instance passed is still a valid AV. This allows
    you to check whether theInst is still valid.

    \param  theInst Instance to check

    \returns TRUE if the instance is valid, FALSE otherwise
 */
bool appAvIsValidInst(avInstanceTaskData* theInst)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    if (!theInst)
        return FALSE;

    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        if (theInst == theAv->av_inst[instance])
            return TRUE;
    }
    return FALSE;
}

/*! \brief Schedules media playback if in correct AV state and flag is set.
    \param  theInst The AV instance.
    \return TRUE if media play is scheduled, otherwise FALSE.
 */
bool appAvInstanceStartMediaPlayback(avInstanceTaskData *theInst)
{
        
    if (appA2dpIsConnectedMedia(theInst) && appAvIsAvrcpConnected(theInst))
    {
        if (theInst->a2dp.bitfields.flags & A2DP_START_MEDIA_PLAYBACK)
        {
            DEBUG_LOG("appAvInstanceStartMediaPlayback(%p)", theInst);
            theInst->a2dp.bitfields.flags &= ~A2DP_START_MEDIA_PLAYBACK;
            MessageSendLater(&theInst->av_task, AV_INTERNAL_AVRCP_PLAY_REQ, NULL,
                             appConfigHandoverMediaPlayDelay());
            return TRUE;
        }
    }
    return FALSE;
}

static int16 appAvGetVolumeChange(ui_input_t ui_input)
{
    uint16 num_pending_msgs = 0;
    uint16 num_steps = 1;
    int16 av_change = 0;

    if(ui_input == ui_input_av_volume_up_start)
    {
        num_pending_msgs = MessageCancelAll(&AvGetTaskData()->task, ui_input_av_volume_up_start);
        num_steps += num_pending_msgs;
        av_change = avConfigGetAvVolumeStep() * num_steps;
    }
    else if(ui_input == ui_input_av_volume_down_start)
    {
        num_pending_msgs = MessageCancelAll(&AvGetTaskData()->task, ui_input_av_volume_down_start);
        num_steps += num_pending_msgs;
        av_change = -(avConfigGetAvVolumeStep() * num_steps);
    }
    return av_change;
}

/*! \brief Stop volume change

    Stop a repeated volume change either remotely (tell the handset)
    or locally.

    \param step The step is required so that the correct direction of
    volume can be cancelled at the handset.
*/
void appAvVolumeStop(int16 step)
{
    /* TODO: Check if using legacy or AbsoluteVolume & Notifications */
    if (1)
    {
        if (MessageCancelFirst(&AvGetTaskData()->task, AV_INTERNAL_VOLUME_UP_REPEAT))
        {
            if (AvGetTaskData()->bitfields.volume_repeat)
            {
                appAvSendUiMessageId(AV_VOLUME_UP);
            }
        }
        if (MessageCancelFirst(&AvGetTaskData()->task, AV_INTERNAL_VOLUME_DOWN_REPEAT))
        {
            if (AvGetTaskData()->bitfields.volume_repeat)
            {
                appAvSendUiMessageId(AV_VOLUME_DOWN);
            }
        }
    }
    else
    {
        if (step > 0)
            appAvVolumeUpRemoteStop();
        else
            appAvVolumeDownRemoteStop();
    }
}

/*! \brief Start a repeated volume change

    Change the volume by the supplied step, and then request a repeated
    volume increase.

    If the volume is already at a limit (max or min) then a UI tone
    will be played, and there will not be a repeat.

    \param  step    Relative amount to adjust the volume by.
                    This can be negative.

    \return TRUE if the volume was changed by at least 1 level,
            FALSE in all other cases
*/
static bool appAvVolumeRepeat(int16 step)
{
    /* Handle volume change locally */
    if (appAvVolumeChange(step))
    {
        MAKE_AV_MESSAGE(AV_INTERNAL_VOLUME_REPEAT);

        /* Send repeat message later */
        message->step = step;
        MessageSendLater(&AvGetTaskData()->task, step > 0 ? AV_INTERNAL_VOLUME_UP_REPEAT : AV_INTERNAL_VOLUME_DOWN_REPEAT,
                         message, AVRCP_CONFIG_VOLUME_REPEAT_TIME);
        AvGetTaskData()->bitfields.volume_repeat = 1;

        /* Return indicating volume changed */
        return TRUE;
    }

    /* Play volume limit tone */
    appAvSendUiMessageId(AV_VOLUME_LIMIT);

    /* Return indicating volume not changed */
    return FALSE;
}

/*! \brief Start volume change

    Start a repeated volume change either remotely (tell the handset)
    or locally.

    \param step Relative amount to adjust the volume by.
     This can be negative.
*/
void appAvVolumeStart(int16 step)
{
    DEBUG_LOG("appAvVolumeStart(%d)", step);

    /* TODO: Check if using legacy or AbsoluteVolume & Notifications */
    if (1)
    {
        if (appAvVolumeRepeat(step))
        {
            AvGetTaskData()->bitfields.volume_repeat = 0;
            if (step > 0)
            {
                appAvSendUiMessageId(AV_VOLUME_UP);
            }
            else
            {
                appAvSendUiMessageId(AV_VOLUME_DOWN);
            }
        }
    }
    else
    {
        /* Use AVRCP to send volume to TWS source */
        if (step > 0)
            appAvVolumeUpRemoteStart();
        else
            appAvVolumeDownRemoteStart();
    }
}

/*! \brief handles av module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void appAvHandleUiInput(MessageId ui_input)
{
    switch (ui_input)
    {

        case ui_input_av_volume_down_remote_stop:
            appAvVolumeStop(avConfigGetAvVolumeStep());
            break;

        case ui_input_av_volume_up_remote_stop:
            appAvVolumeStop(avConfigGetAvVolumeStep());
            break;

        case ui_input_av_volume_up_start:
        case ui_input_av_volume_down_start:
            appAvVolumeStart(appAvGetVolumeChange(ui_input));
            break;

        default:
            break;
    }
}

/*! \brief Message Handler

    This function is the main message handler for the AV module, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
void appAvHandleMessage(Task task, MessageId id, Message message)
{
    avTaskData *theAv = (avTaskData *)task;
    avState state = appAvGetState(theAv);

    if (isMessageUiInput(id))
    {
        appAvHandleUiInput(id);
        return;
    }

    /* Handle connection manager messages */
    switch (id)
    {
        case CON_MANAGER_CONNECTION_IND:
            appAvHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *)message);
            return;
    }

    /* Handle A2DP messages */
    switch (id)
    {
        case A2DP_INIT_CFM:
        {
            switch (state)
            {
                case AV_STATE_INITIALISING_A2DP:
                    appAvHandleA2dpInitConfirm(theAv, (A2DP_INIT_CFM_T *)message);
                    return;
                default:
                    UnexpectedMessage_HandleMessage(id);
                    return;
            }
        }

        case AVRCP_INIT_CFM:
        {
            switch (state)
            {
                case AV_STATE_INITIALISING_AVRCP:
                    appAvHandleAvrcpInitConfirm(theAv, (AVRCP_INIT_CFM_T *)message);
                    return;
                default:
                    UnexpectedMessage_HandleMessage(id);
                    return;
            }
        }

        case A2DP_SIGNALLING_CONNECT_IND:
        {
            switch (state)
            {
                case AV_STATE_IDLE:
                    appA2dpSignallingConnectIndicationNew(theAv, (A2DP_SIGNALLING_CONNECT_IND_T *)message);
                    return;
                default:
                    appA2dpRejectA2dpSignallingConnectIndicationNew(theAv, (A2DP_SIGNALLING_CONNECT_IND_T *)message);
                    return;
            }
        }

        case AVRCP_CONNECT_IND:
        {
            switch (state)
            {
                case AV_STATE_IDLE:
                    appAvrcpHandleAvrcpConnectIndicationNew(theAv, (AVRCP_CONNECT_IND_T *)message);
                    return;
                default:
                    appAvrcpRejectAvrcpConnectIndicationNew(theAv, (AVRCP_CONNECT_IND_T *)message);
                    return;
            }
        }

        case AV_INTERNAL_VOLUME_UP_REPEAT:
        case AV_INTERNAL_VOLUME_DOWN_REPEAT:
        {
            int new_volume = (AudioSources_GetVolume(audio_source_a2dp_1).value + ((AV_INTERNAL_VOLUME_REPEAT_T *)message)->step);
            Volume_SendAudioSourceVolumeUpdateRequest(audio_source_a2dp_1, event_origin_local, new_volume);
            return;
        }

        case AV_INTERNAL_VOLUME_STORE_REQ:
            appAvVolumeAttributeStore(theAv);
            return;

        case AV_AVRCP_CONNECT_IND:
            appAvHandleAvAvrcpConnectIndication(theAv, (AV_AVRCP_CONNECT_IND_T *)message);
            return;

        default:
            appAvError(theAv, id, message);
            return;
    }
}

/*! \brief Get the current play status of the AV

    \return The play status, the status of the stream from a headset,
            which can include avrcp_play_status_error.
            avrcp_play_status_error is also returned if there is no
            current link.
*/
avrcp_play_status appAvPlayStatus(void)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* First of all in table to find entry that is streaming as sink */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsSinkCodec(theInst) && appA2dpIsConnectedMedia(theInst))
        {
            if (theInst->avrcp.play_status != avrcp_play_status_error)
                return theInst->avrcp.play_status;
            else
                return theInst->avrcp.play_hint;
        }
    }

    return avrcp_play_status_error;
}

/*! \brief Set the play status if the real status is not known

    The AV should know whether we are playing audio, based on AVRCP
    status messages. This information can be missing, in which case
    this function allows you to set a status. It won't override
    a known status.

    \param status   The status hint to be used
 */
void appAvHintPlayStatus(avrcp_play_status status)
{
    avTaskData *theAv = AvGetTaskData();
    int instance;

    /* First of all in table to find entry that is streaming as sink */
    for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
    {
        avInstanceTaskData *theInst = theAv->av_inst[instance];
        if (theInst && appA2dpIsSinkCodec(theInst) && appA2dpIsConnectedMedia(theInst))
        {
            if (theInst->avrcp.play_status == avrcp_play_status_error)
                theInst->avrcp.play_hint = status;

            return;
        }
    }
}

void appAvPlayOnHandsetConnection(bool play)
{
    avTaskData *theAv = AvGetTaskData();

    theAv->play_on_connect = play;
}


void appAvConfigStore(void)
{
    /* Cancel any pending messages */
    MessageCancelFirst(appGetAvPlayerTask(), AV_INTERNAL_VOLUME_STORE_REQ);

    /* Store configuration after 5 seconds */
    MessageSendLater(appGetAvPlayerTask(), AV_INTERNAL_VOLUME_STORE_REQ, 0, D_SEC(5));
}

static void av_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group = AV_UI_MESSAGE_GROUP);
    appAvUiClientRegister(task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(AV_UI, av_RegisterMessageGroup, NULL);

#endif
