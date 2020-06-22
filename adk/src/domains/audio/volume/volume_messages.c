/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Functions for generating volume update request messages

*/

#include "audio_sources.h"
#include "voice_sources.h"
#include "volume_messages.h"

#include <message.h>
#include <task_list.h>
#include <panic.h>
#include <logging.h>

static task_list_t * client_list;

static task_list_t * volume_GetMessageClients(void)
{
    return client_list;
}

void Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_t source, event_origin_t origin, int volume)
{
    voice_source_volume_update_request_message_t * const message = (voice_source_volume_update_request_message_t*)PanicUnlessMalloc(sizeof(voice_source_volume_update_request_message_t));
    message->voice_source = source;
    message->origin = origin;
    message->volume = VoiceSources_GetVolume(source);
    message->volume.value = volume;
    DEBUG_LOG("Volume_SendVoiceSourceVolumeUpdateRequest, source %u, origin %u, volume %u", source, origin, volume);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), VOICE_SOURCE_VOLUME_UPDATE_REQUEST, message,
            sizeof(voice_source_volume_update_request_message_t));
}

void Volume_SendVoiceSourceVolumeIncrementRequest(voice_source_t source, event_origin_t origin)
{
    voice_source_volume_increment_request_message_t * const message = (voice_source_volume_increment_request_message_t*)PanicUnlessMalloc(sizeof(voice_source_volume_increment_request_message_t));
    message->voice_source = source;
    message->origin = origin;
    DEBUG_LOG("Volume_SendVoiceSourceVolumeIncrementRequest, source %u, origin %u", source, origin);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), VOICE_SOURCE_VOLUME_INCREMENT_REQUEST, message,
            sizeof(voice_source_volume_increment_request_message_t));
}

void Volume_SendVoiceSourceVolumeDecrementRequest(voice_source_t source, event_origin_t origin)
{
    voice_source_volume_decrement_request_message_t * const message = (voice_source_volume_decrement_request_message_t*)PanicUnlessMalloc(sizeof(voice_source_volume_decrement_request_message_t));
    message->voice_source = source;
    message->origin = origin;
    DEBUG_LOG("Volume_SendVoiceSourceVolumeDecrementRequest, source %u, origin %u", source, origin);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), VOICE_SOURCE_VOLUME_DECREMENT_REQUEST, message,
            sizeof(voice_source_volume_decrement_request_message_t));
}

void Volume_SendAudioSourceVolumeUpdateRequest(audio_source_t source, event_origin_t origin, int volume)
{
    audio_source_volume_update_request_message_t * const message = (audio_source_volume_update_request_message_t*)PanicUnlessMalloc(sizeof(audio_source_volume_update_request_message_t));
    message->audio_source = source;
    message->origin = origin;
    message->volume = AudioSources_GetVolume(source);
    message->volume.value = volume;
    DEBUG_LOG("Volume_SendAudioSourceVolumeUpdateRequest, source %u, origin %u, volume %u", source, origin, volume);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), AUDIO_SOURCE_VOLUME_UPDATE_REQUEST, message,
            sizeof(audio_source_volume_update_request_message_t));
}

void Volume_SendAudioSourceVolumeIncrementRequest(audio_source_t source, event_origin_t origin)
{
    audio_source_volume_increment_request_message_t * const message = (audio_source_volume_increment_request_message_t*)PanicUnlessMalloc(sizeof(audio_source_volume_increment_request_message_t));
    message->audio_source = source;
    message->origin = origin;
    DEBUG_LOG("Volume_SendAudioSourceVolumeIncrementRequest, source %u, origin %u", source, origin);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), AUDIO_SOURCE_VOLUME_INCREMENT_REQUEST, message,
            sizeof(audio_source_volume_increment_request_message_t));
}

void Volume_SendAudioSourceVolumeDecrementRequest(audio_source_t source, event_origin_t origin)
{
    audio_source_volume_decrement_request_message_t * const message = (audio_source_volume_decrement_request_message_t*)PanicUnlessMalloc(sizeof(audio_source_volume_decrement_request_message_t));
    message->audio_source = source;
    message->origin = origin;
    DEBUG_LOG("Volume_SendAudioSourceVolumeDecrementRequest, source %u, origin %u", source, origin);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), AUDIO_SOURCE_VOLUME_DECREMENT_REQUEST, message,
            sizeof(audio_source_volume_decrement_request_message_t));
}

void Volume_MuteRequest(bool mute_state)
{
    mute_volume_request_message_t * const message = (mute_volume_request_message_t *)PanicUnlessMalloc(sizeof(mute_volume_request_message_t));
    message->mute_state = mute_state;
    DEBUG_LOG("Volume_MuteRequest, mute_state %u", mute_state);
    TaskList_MessageSendWithSize(volume_GetMessageClients(), MUTE_VOLUME_REQUEST, message,
                sizeof(mute_volume_request_message_t));
}

bool Volume_InitMessages(Task init_task)
{
    UNUSED(init_task);
    client_list = TaskList_Create();
    return TRUE;
}

void Volume_RegisterForMessages(Task task_to_register)
{
    TaskList_AddTask(volume_GetMessageClients(), task_to_register);
}

