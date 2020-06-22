/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       audio_sync.c
\brief      API to synchronise an audio sink with and audio source.
*/

#include <logging.h>
#include <message.h>
#include <panic.h>

#include "audio_sync.h"


/*! \{
    Macros for diagnostic output that can be suppressed.
    Allows debug of this module at two levels. */
#define AUDIO_SYNC_LOG(...)       DEBUG_LOG(__VA_ARGS__)
/*! \} */

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)


/*! \brief Send a AUDIO_SYNC_CONNECT_IND to a synchronised AV instance. */
void AudioSync_ConnectIndication(struct audio_sync_t *sync_inst, Task reply_task,
                                 audio_source_t source_id,
                                 uint8 seid, uint8 sync_id)
{
    uint32 delay = D_IMMEDIATE;

    AUDIO_SYNC_LOG("AudioSync_ConnectIndication sync_inst %p replay_task %p",
                    sync_inst, reply_task);

    assert(reply_task);

    if (sync_inst)
    {
        MESSAGE_MAKE(message, AUDIO_SYNC_CONNECT_IND_T);
        message->task = reply_task;
        message->source_id = source_id;
        message->seid = seid;
        message->sync_id = sync_id;
        sync_inst->sync_if.SendSyncMessage(sync_inst, AUDIO_SYNC_CONNECT_IND, message);

        /* After delay calling task will carry on */
        delay = AudioSync_MaxSyncDelayMs();
    }

    /* This message is sent in two circumstances when a response is required:
       1. No other instance registered to synchronise with this one, in which
          case no ind was sent and we need to send the response to ourself.
       2. The ind was sent to the other instance, but we have to send a message
          later in case the other instance fails to respond for some reason.
    */
    {
        MESSAGE_MAKE(message, AUDIO_SYNC_CONNECT_RES_T);
        message->sync_id = sync_id;
        MessageSendLater(reply_task, AUDIO_SYNC_CONNECT_RES, message, delay);
    }
}

/*! \brief Send a AUDIO_SYNC_ACTIVATE_IND to a synchronised AV instance. */
void AudioSync_ActivateIndication(struct audio_sync_t *sync_inst, Task reply_task,
                                  audio_source_t source_id,
                                  uint8 seid, uint8 sync_id)
{
    uint32 delay = D_IMMEDIATE;

    AUDIO_SYNC_LOG("AudioSync_ActivateIndication sync_inst %p replay_task %p",
                    sync_inst, reply_task);

    assert(reply_task);

    if (sync_inst)
    {
        MESSAGE_MAKE(message, AUDIO_SYNC_ACTIVATE_IND_T);
        message->task = reply_task;
        message->source_id = source_id;
        message->seid = seid;
        message->sync_id = sync_id;
        sync_inst->sync_if.SendSyncMessage(sync_inst, AUDIO_SYNC_ACTIVATE_IND, message);

        /* After delay calling task will carry on */
        delay = AudioSync_MaxSyncDelayMs();
    }

    /* This message is sent in two circumstances when a response is required:
       1. No other instance registered to synchronise with this one, in which
          case no ind was sent and we need to send the response to ourself.
       2. The ind was sent to the other instance, but we have to send a message
          later in case the other instance fails to respond for some reason.
    */
    {
        MESSAGE_MAKE(message, AUDIO_SYNC_ACTIVATE_RES_T);
        message->sync_id = sync_id;
        MessageSendLater(reply_task, AUDIO_SYNC_ACTIVATE_RES, message, delay);
    }
}

/*! \brief Send a AUDIO_SYNC_STATE_IND to a synchronised AV instance. */
void AudioSync_StateIndication(struct audio_sync_t *sync_inst,
                               audio_source_t source_id,
                               audio_sync_state_t state, uint8 seid)
{
    if (sync_inst)
    {
        AUDIO_SYNC_LOG("AudioSync_StateIndication sync_inst %p state 0x%x",
                        sync_inst, state);

        MESSAGE_MAKE(message, AUDIO_SYNC_STATE_IND_T);
        message->state = state;
        message->source_id = source_id;
        message->seid = seid;
        sync_inst->sync_if.SendSyncMessage(sync_inst, AUDIO_SYNC_STATE_IND, message);
    }

    /* No reply required */
}

/*! \brief Send a AUDIO_SYNC_CODEC_RECONFIGURED_IND to a synchronised AV instance. */
void AudioSync_CodecReconfiguredIndication(struct audio_sync_t *sync_inst,
                                           audio_source_t source_id,
                                           uint8 seid, uint8 device_id, uint8 stream_id)
{
    if (sync_inst)
    {
        AUDIO_SYNC_LOG("AudioSync_CodecReconfiguredIndication sync_inst %p seid 0x%x d_id %u s_id %u",
                        sync_inst, seid, device_id, stream_id);

        MESSAGE_MAKE(message, AUDIO_SYNC_CODEC_RECONFIGURED_IND_T);
        message->source_id = source_id;
        message->seid = seid;
        message->device_id = device_id;
        message->stream_id = stream_id;
        sync_inst->sync_if.SendSyncMessage(sync_inst, AUDIO_SYNC_CODEC_RECONFIGURED_IND, message);
    }

    /* No reply required */
}

/* \brief Cancel any pending AUDIO_SYNC_* messages on a Task. */
void AudioSync_CancelQueuedMessages(struct audio_sync_t *sync_inst)
{
    if (sync_inst && sync_inst->task)
    {
        MessageId id;

        for (id = AUDIO_SYNC_BASE; id < AUDIO_SYNC_TOP; id++)
        {
            /* The AUDIO_SYNC_STATE_IND message should not be cancelled.
               Cancelling that message would lead to audio sync state 
               being out of sync with e.g. a2dp state.
               That can cause situation when the media channel between 
               earbuds is not closed. When media channel is not closed then
               codec won't be changed. In case when another phone would 
               use different codec, that change won't be propagated to 
               media channel between earbuds. As a result the secondary earbud 
               won't render audio.
            */
            if(id != AUDIO_SYNC_STATE_IND)
            {
                MessageCancelAll(sync_inst->task, id);
            }
        }
    }
}
