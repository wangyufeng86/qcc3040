/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_profile_sync.c
\brief      Coordination between the Sink & Source (aka forwarding) A2DP roles.
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include <logging.h>
#include <message.h>
#include <panic.h>
#include <timestamp_event.h>

#include "av.h"
#include "a2dp_profile.h"
#include "a2dp_profile_sync.h"
#include "a2dp_profile_config.h"

/*! \{
    Macros for diagnostic output that can be suppressed.
    Allows debug of this module at two levels. */
#define A2DP_SYNC_LOG        DEBUG_LOG
/*! \} */

/*! Code assertion that can be checked at run time        . This will cause a panic. */
#define assert(x) PanicFalse(x)

/*! Macro for simplifying creating messages */
#define MAKE_AV_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);


/*
    Functions that implement the audio_sync_interface_t for an a2dp profile
*/

/*! \brief Send an audio_sync_msg_t to an AV instance.

    The audio_sync_msg_t messages must only be handled in a known stable
    state, so they are sent conditionally on the a2dp state machine lock.
*/
static void appA2dpSyncSendAudioSyncMessage(struct audio_sync_t *sync_inst, MessageId id, Message message)
{
    /* We know that sync_inst->task is actually a avInstanceTaskData, so cast it appropriately. */
    avInstanceTaskData *theInst = (avInstanceTaskData *)sync_inst->task;

    PanicFalse(MessageCancelAll(&theInst->av_task, id) <= 1);
    MessageSendConditionally(&theInst->av_task, id, message, &appA2dpGetLock(theInst));
}

/*
    Helpers for sending AV_INTERNAL_A2DP_* messages to an AV instance.
*/

/* \brief Helper function to send an internal message to an a2dp instance. */
static void appA2dpSyncSendInternalMessage(avInstanceTaskData *theInst,
                                           MessageId msg_id, Message msg_data)
{
    MessageCancelFirst(&theInst->av_task, msg_id);
    MessageSendConditionally(&theInst->av_task, msg_id, msg_data, &appA2dpGetLock(theInst));
}

static void appA2dpSyncSendInternalMediaConnectReq(avInstanceTaskData *theInst, uint8 seid)
{
    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ);
    message->seid = seid;
    message->delay_ms = 0;
    if (A2dpProfile_IsPtsMode())
    {
        /* Add delay to allow the remote side to set up A2DP in PTS tests */
        message->delay_ms = appConfigA2dpMediaConnectDelayAfterLocalA2dpConnectMs();
    }
    appA2dpSyncSendInternalMessage(theInst, AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ, message);
}

static void appA2dpSyncSendInternalMediaSuspendReq(avInstanceTaskData *theInst)
{
    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ);
    message->reason = AV_SUSPEND_REASON_RELAY;
    appA2dpSyncSendInternalMessage(theInst, AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ, message);
}

static void appA2dpSyncSendInternalMediaResumeReq(avInstanceTaskData *theInst)
{
    MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_RESUME_MEDIA_REQ);
    message->reason = AV_SUSPEND_REASON_RELAY;
    appA2dpSyncSendInternalMessage(theInst, AV_INTERNAL_A2DP_RESUME_MEDIA_REQ, message);
}

/*
    Handlers for AUDIO_SYNC_... messages
*/

static void appA2dpSyncHandleA2dpSyncConnectIndication(avInstanceTaskData *theInst,
                                                       const AUDIO_SYNC_CONNECT_IND_T *ind)
{
    bool reply_is_locked = FALSE;
    avA2dpState local_state = appA2dpGetState(theInst);
    uint8 source_seid = appA2dpConvertSeidFromSinkToSource(ind->seid);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectIndication(%p) state(0x%x)", theInst, local_state);

    switch (local_state)
    {
    case A2DP_STATE_CONNECTED_SIGNALLING:
        if (source_seid != AV_SEID_INVALID)
        {
            /* Connect media channel  */
            appA2dpSyncSendInternalMediaConnectReq(theInst, source_seid);
        }
        break;

    case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
    case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        if (theInst->a2dp.current_seid == source_seid)
        {
            /* Suspend the media channel */
            appA2dpSyncSendInternalMediaSuspendReq(theInst);
        }
        break;

    default:
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectIndication - unhandled (wrong state)");
        break;
    }

    if (ind->task)
    {
        MAKE_AV_MESSAGE(AUDIO_SYNC_CONNECT_RES);
        message->sync_id = ind->sync_id;
        /* If the state machine needs to process one or more internal messages
            before replying, send the reply conditional on the state machine
            lock. */
        MessageSendConditionally(ind->task, AUDIO_SYNC_CONNECT_RES, message,
                                 reply_is_locked ? NULL : &appA2dpGetLock(theInst));
    }
}

static void appA2dpSyncHandleA2dpSyncConnectResponse(avInstanceTaskData *theInst,
                                                     const AUDIO_SYNC_CONNECT_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    if (((res->sync_id + 1) & 0xff) != theInst->a2dp.sync_counter)
    {
        /* This means whilst waiting for a sync response from the other instance,
        something else triggered the instance to exit the _SYNC state. So this sync
        response is late, and now irrelevant. */
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse(%p) late state(0x%x) sync_id(%d) sync_count(%d)",
            theInst, local_state, res->sync_id, theInst->a2dp.sync_counter);
        return;
    }

    /* This will cancel any responses sent 'later' to catch the other instance
       not responding in time. */
    PanicFalse(MessageCancelAll(&theInst->av_task, AUDIO_SYNC_CONNECT_RES) <= 1);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse(%p) state(0x%x) sync_id(%d)",
               theInst, local_state, res->sync_id);

    switch (local_state)
    {
    case A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC:
        /* Accept media connection */
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncConnectResponse accepting A2dpMediaOpen device_id %u", theInst->a2dp.device_id);
        PanicFalse(A2dpMediaOpenResponse(theInst->a2dp.device_id, TRUE));
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_OPEN_CFM. */
        break;

    default:
        appA2dpError(theInst, AUDIO_SYNC_CONNECT_RES, NULL);
        break;
    }
    theInst->a2dp.sync_counter++;
}

static void appA2dpSyncHandleA2dpSyncActivateIndication(avInstanceTaskData *theInst,
                                                        const AUDIO_SYNC_ACTIVATE_IND_T *ind)
{
    bool reply_is_locked = FALSE;
    avA2dpState local_state = appA2dpGetState(theInst);
    uint8 source_seid = appA2dpConvertSeidFromSinkToSource(ind->seid);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateIndication(%p) state(0x%x)", theInst, local_state);

    switch (local_state)
    {
    case A2DP_STATE_DISCONNECTED:
    case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
    case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
        /* Nothing to do */
        break;

    case A2DP_STATE_CONNECTED_SIGNALLING:
        if (source_seid != AV_SEID_INVALID)
        {
            /* Queue two separate internal messages - one to connect media
               & the other to start streaming */
            appA2dpSyncSendInternalMediaConnectReq(theInst, source_seid);
            appA2dpSyncSendInternalMediaResumeReq(theInst);
        }
        break;

    case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateIndication MEDIA_SUSPENDED inst->seid 0x%x source_seid 0x%x",
                      theInst->a2dp.current_seid, source_seid);
        if (theInst->a2dp.current_seid == source_seid)
        {
            /* Resume streaming */
            appA2dpSyncSendInternalMediaResumeReq(theInst);
        }
        break;

    default:
        A2DP_SYNC_LOG("appA2dpHandleInternalA2dpInstSyncIndication(%p) unhandled state 0x%x",
                       theInst, local_state);
        break;
    }

    if (ind->task)
    {
        MAKE_AV_MESSAGE(AUDIO_SYNC_ACTIVATE_RES);
        message->sync_id = ind->sync_id;
        /* If the state machine needs to process one or more internal messages
            before replying, send the reply conditional on the state machine
            lock. */
        MessageSendConditionally(ind->task, AUDIO_SYNC_ACTIVATE_RES, message,
                                 reply_is_locked ? NULL : &appA2dpGetLock(theInst));
    }
}

static void appA2dpSyncHandleA2dpSyncActivateResponse(avInstanceTaskData *theInst,
                                                      const AUDIO_SYNC_ACTIVATE_RES_T *res)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    if (((res->sync_id + 1) & 0xff) != theInst->a2dp.sync_counter)
    {
        /* This means whilst waiting for a sync response from the other instance,
        something else triggered the instance to exit the _SYNC state. So this sync
        response is late, and now irrelevant. */
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateResponse(%p) late state(0x%x) sync_id(%d) sync_count(%d)",
                       theInst, local_state, res->sync_id, theInst->a2dp.sync_counter);
        return;
    }

    /* This will cancel any responses sent 'later' to catch the other instance
       not responding in time. */
    PanicFalse(MessageCancelAll(&theInst->av_task, AUDIO_SYNC_ACTIVATE_RES) <= 1);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncActivateResponse(%p) state(0x%x) sync_id(%d)",
                   theInst, local_state, res->sync_id);

    switch (local_state)
    {
    case A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC:
        /* Start streaming request */
        PanicFalse(A2dpMediaStartRequest(theInst->a2dp.device_id, theInst->a2dp.stream_id));
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_START_CFM. */
        break;

    case A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC:
    {
        TimestampEvent(TIMESTAMP_EVENT_A2DP_START_RSP);
        PanicFalse(A2dpMediaStartResponse(theInst->a2dp.device_id, theInst->a2dp.stream_id, TRUE));
        /* The sync is complete, remain in this state waiting for the
            A2DP_MEDIA_START_CFM. */
        break;
    }

    default:
        appA2dpError(theInst, AUDIO_SYNC_CONNECT_RES, NULL);
        break;
    }
    theInst->a2dp.sync_counter++;
}

static void appA2dpSyncHandleA2dpSyncStateIndication(avInstanceTaskData *theInst,
                                                     const AUDIO_SYNC_STATE_IND_T *ind)
{
    avA2dpState local_state = appA2dpGetState(theInst);
    uint8 source_seid = appA2dpConvertSeidFromSinkToSource(ind->seid);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncStateIndication(%p) local_state(0x%x) state(0x%x)", theInst, local_state, ind->state);

    switch (ind->state)
    {
    case AUDIO_SYNC_STATE_DISCONNECTED:
        /* Only request a disconnect if media is connected and this instance
            is a source instance with matching seid. */
        if (appA2dpIsStateConnectedMedia(theInst->a2dp.state) &&
            (theInst->a2dp.current_seid == source_seid))
        {
            /* Disconnect media channel */
            appA2dpSyncSendInternalMessage(theInst, AV_INTERNAL_A2DP_DISCONNECT_MEDIA_REQ, NULL);
        }
        break;

    case AUDIO_SYNC_STATE_CONNECTED:
        switch (local_state)
        {
        case A2DP_STATE_DISCONNECTED:
        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            /* Nothing to do */
            break;

        case A2DP_STATE_CONNECTED_SIGNALLING:
            if (source_seid != AV_SEID_INVALID)
            {
                /* Connect media channel  */
                appA2dpSyncSendInternalMediaConnectReq(theInst, source_seid);
            }
            break;

        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            if (theInst->a2dp.current_seid == source_seid)
            {
                /* Suspend the media channel */
                appA2dpSyncSendInternalMediaSuspendReq(theInst);
            }
            break;

        default:
            A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncStateIndication(%p) unhandled state 0x%x",
                           theInst, local_state);
            break;
        }
        break;

    case AUDIO_SYNC_STATE_ACTIVE:
        switch (local_state)
        {
        case A2DP_STATE_DISCONNECTED:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING:
        case A2DP_STATE_CONNECTED_MEDIA_STREAMING_MUTED:
            /* Nothing to do */
            break;

        case A2DP_STATE_CONNECTED_SIGNALLING:
            if (source_seid != AV_SEID_INVALID)
            {
                /* Queue two separate internal messages - one to connect media
                    & the other to start streaming */
                appA2dpSyncSendInternalMediaConnectReq(theInst, source_seid);
                appA2dpSyncSendInternalMediaResumeReq(theInst);
            }
            break;

        case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
            if (theInst->a2dp.current_seid == source_seid)
            {
                /* Resume streaming */
                appA2dpSyncSendInternalMediaResumeReq(theInst);
            }
            break;

        default:
            A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncStateIndication(%p) unhandled state 0x%x",
                           theInst, local_state);
            break;
        }
        break;

    default:
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncStateIndication(%p) unhandled state 0x%x",
                       theInst, ind->state);
        break;
    }

    /* No reply required */
}

static void appA2dpSyncHandleA2dpSyncCodecReconfiguredIndication(avInstanceTaskData *theInst,
                                                                 const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *ind)
{
    avA2dpState local_state = appA2dpGetState(theInst);

    A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncCodecReconfiguredIndication(%p) state(0x%x)", theInst, appA2dpGetState(theInst));

    switch (local_state)
    {
    case A2DP_STATE_CONNECTED_MEDIA_SUSPENDED:
        {
            /* Reconfigure codec */
            MAKE_AV_MESSAGE(AV_INTERNAL_A2DP_CODEC_RECONFIG_IND);
            message->device_id = ind->device_id;
            message->stream_id = ind->stream_id;
            appA2dpSyncSendInternalMessage(theInst, AV_INTERNAL_A2DP_CODEC_RECONFIG_IND, message);
        }
        break;

    default:
        A2DP_SYNC_LOG("appA2dpSyncHandleA2dpSyncCodecReconfiguredIndication(%p) unhandled state 0x%x",
                       theInst, local_state);
        break;
    }

    /* No reply required */
}


/*! \brief Initialise a audio_sync_t instance for use with an a2dp profile instance */
void appA2dpSyncInitialise(avInstanceTaskData *theInst)
{
    /* Start with no audio_sync_t instance to synchronise with. */
    theInst->a2dp.sync_inst = NULL;

    /* Setup the audio_sync_t instance that clients can use synchronise
       with this AV isntance. */
    theInst->a2dp.sync_if.task = &theInst->av_task;
    theInst->a2dp.sync_if.sync_if.SendSyncMessage = appA2dpSyncSendAudioSyncMessage;
}

/*! \brief Get the audio_sync_t interface to synchronise with this AV instance. */
struct audio_sync_t *appA2dpSyncGetAudioSyncInterface(avInstanceTaskData *theInst)
{
    return &theInst->a2dp.sync_if;
}

/*! \brief Get the audio_sync_state_t for a given avA2dpState. */
static audio_sync_state_t appA2dpSyncGetAudioSyncState(avA2dpState a2dp_state)
{
    audio_sync_state_t audio_state = AUDIO_SYNC_STATE_DISCONNECTED;

    if (appA2dpIsStateConnectedMediaStreaming(a2dp_state))
    {
        audio_state = AUDIO_SYNC_STATE_ACTIVE;
    }
    else if (appA2dpIsStateConnectedMedia(a2dp_state))
    {
        audio_state = AUDIO_SYNC_STATE_CONNECTED;
    }

    return audio_state;
}

/*! \brief Register AV instance to synchronise with another AV instance */
void appA2dpSyncRegisterInstance(avInstanceTaskData *theInst, avInstanceTaskData *theSyncInst)
{
    audio_sync_t *sync_if = theSyncInst ? appA2dpSyncGetAudioSyncInterface(theSyncInst) : NULL;
    appA2dpSyncRegister(theInst, sync_if);
}

/*! \brief Un-register the audio_sync_t registered to an AV instance. */
void appA2dpSyncUnregisterInstance(avInstanceTaskData *theInst)
{
    appA2dpSyncUnregister(theInst, theInst->a2dp.sync_inst);
}

void appA2dpSyncRegister(avInstanceTaskData *theInst, audio_sync_t *sync_if)
{
    avA2dpState local_state = appA2dpGetState(theInst);
    DEBUG_LOG("appA2dpSyncRegister(%p) sync_if(%p) state(0x%x)",
                (void *)theInst, sync_if, local_state);

    PanicFalse(theInst->a2dp.sync_inst == NULL);
    theInst->a2dp.sync_inst = sync_if;

    /* Notify the current state to the synchronised instance. */
    AudioSync_StateIndication(theInst->a2dp.sync_inst,
                              audio_source_a2dp_1,
                              appA2dpSyncGetAudioSyncState(local_state),
                              theInst->a2dp.current_seid);
}

void appA2dpSyncUnregister(avInstanceTaskData *theInst, audio_sync_t *sync_if)
{
    DEBUG_LOG("appA2dpSyncUnregister theInst %p is_valid %d", theInst, appAvIsValidInst(theInst));

    if (appAvIsValidInst(theInst)
        && theInst->a2dp.sync_inst)
    {
        PanicFalse(theInst->a2dp.sync_inst == sync_if);
        AudioSync_CancelQueuedMessages(theInst->a2dp.sync_inst);
        theInst->a2dp.sync_inst = NULL;
    }
}

/*! \brief Handler function for all audio_sync_msg_t messages */
void appA2dpSyncHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message)
{
    switch (id)
    {
    case AUDIO_SYNC_CONNECT_IND:
        appA2dpSyncHandleA2dpSyncConnectIndication(theInst, (const AUDIO_SYNC_CONNECT_IND_T *)message);
        break;

    case AUDIO_SYNC_CONNECT_RES:
        appA2dpSyncHandleA2dpSyncConnectResponse(theInst, (const AUDIO_SYNC_CONNECT_RES_T *)message);
        break;

    case AUDIO_SYNC_ACTIVATE_IND:
        appA2dpSyncHandleA2dpSyncActivateIndication(theInst, (const AUDIO_SYNC_ACTIVATE_IND_T *)message);
        break;

    case AUDIO_SYNC_ACTIVATE_RES:
        appA2dpSyncHandleA2dpSyncActivateResponse(theInst, (const AUDIO_SYNC_ACTIVATE_RES_T *)message);
        break;

    case AUDIO_SYNC_STATE_IND:
        appA2dpSyncHandleA2dpSyncStateIndication(theInst, (const AUDIO_SYNC_STATE_IND_T *)message);
        break;

    case AUDIO_SYNC_CODEC_RECONFIGURED_IND:
        appA2dpSyncHandleA2dpSyncCodecReconfiguredIndication(theInst, (const AUDIO_SYNC_CODEC_RECONFIGURED_IND_T *)message);
        break;

    default:
        A2DP_SYNC_LOG("appA2dpSyncHandleMessage unhandled msg id 0x%x", id);
        break;
    }
}

#endif /* INCLUDE_AV */
