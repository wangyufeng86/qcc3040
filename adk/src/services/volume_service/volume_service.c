/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the Volume Service.
*/

#include "volume_service.h"

#include "audio_sources.h"
#include "kymera_adaptation.h"
#include "volume_system.h"
#include "voice_sources.h"
#include "volume_messages.h"
#include "volume_mute.h"
#include "volume_utils.h"

#include <panic.h>
#include <task_list.h>
#include <message_broker.h>
#include <logging.h>
#include <stdio.h>

#define INTERNAL_MSG_APPLY_AUDIO_VOLUME    0
#define VOLUME_SERVICE_CLIENT_TASK_LIST_INIT_CAPACITY 1

typedef struct
{
    TASK_LIST_WITH_INITIAL_CAPACITY(VOLUME_SERVICE_CLIENT_TASK_LIST_INIT_CAPACITY)  client_list;
    TaskData volume_message_handler_task;
} volume_service_data;

static volume_service_data the_volume_service;

#define VolumeServiceGetClientLIst() (task_list_flexible_t *)(&the_volume_service.client_list)

static void volumeService_InternalMessageHandler( Task task, MessageId id, Message message );
static void volumeService_RefreshAudioVolume(event_origin_t origin);

static TaskData internal_message_task = { volumeService_InternalMessageHandler };

static void volumeService_InternalMessageHandler( Task task, MessageId id, Message message )
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case INTERNAL_MSG_APPLY_AUDIO_VOLUME:
            {
                volumeService_RefreshAudioVolume(event_origin_local);
            }
            break;
        default:
            Panic();
            break;
    }
}

static void volumeService_NotifyMinOrMaxVolume(volume_t volume)
{
    if(volume.value >= volume.config.range.max)
    {
        TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(VolumeServiceGetClientLIst()), VOLUME_SERVICE_MAX_VOLUME);
    }
    if(volume.value <= volume.config.range.min)
    {
        TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(VolumeServiceGetClientLIst()), VOLUME_SERVICE_MIN_VOLUME);
    }
}

static void volumeService_RefreshVoiceVolume(void)
{
    volume_t volume = Volume_CalculateOutputVolume(VoiceSources_GetVolume(VoiceSources_GetRoutedSource()));
    volume_parameters_t volume_params = { .source_type = source_type_voice, .volume = volume };
    KymeraAdaptation_SetVolume(&volume_params);
}

static bool isVolumeToBeSynchronised(void)
{
    /* Doesn't exist yet */
    return FALSE;
}

static uint16 getSynchronisedVolumeDelay(void)
{
    return 0;
}

static void volumeService_RefreshAudioVolume(event_origin_t origin)
{
    if ((origin == event_origin_local) && isVolumeToBeSynchronised())
    {
        MessageSendLater(&internal_message_task, INTERNAL_MSG_APPLY_AUDIO_VOLUME, 0, getSynchronisedVolumeDelay());
    }
    else
    {
        volume_t volume = Volume_CalculateOutputVolume(AudioSources_GetVolume(AudioSources_GetRoutedSource()));
        volume_parameters_t volume_params = { .source_type = source_type_audio, .volume = volume };
        KymeraAdaptation_SetVolume(&volume_params);
    }
}

static void volumeService_RefreshCurrentVolume(event_origin_t origin)
{
    if(VoiceSources_GetRoutedSource() != voice_source_none)
    {
        volumeService_RefreshVoiceVolume();
    }
    else
    {
        if(AudioSources_GetRoutedSource() != audio_source_none)
        {
            volumeService_RefreshAudioVolume(origin);
        }
    }
}

static void volumeService_UpdateAudioSourceVolume(audio_source_t source, volume_t new_volume, event_origin_t origin)
{
    AudioSources_SetVolume(source, new_volume);
    AudioSources_OnVolumeChange(source, origin, new_volume);
    if(source == AudioSources_GetRoutedSource())
    {
        volumeService_RefreshAudioVolume(origin);
    }
}

static void volumeService_UpdateSystemVolume(volume_t new_volume, event_origin_t origin)
{
    Volume_SetSystemVolume(new_volume);
    volumeService_RefreshCurrentVolume(origin);
}

static void volumeService_UpdateVoiceSourceLocalVolume(voice_source_t source, volume_t new_volume, event_origin_t origin)
{
    VoiceSources_SetVolume(source, new_volume);
    VoiceSources_OnVolumeChange(source, origin, new_volume);
    if(source == VoiceSources_GetRoutedSource())
    {
        volumeService_RefreshVoiceVolume();
    }
}

void VolumeService_SetAudioSourceVolume(audio_source_t source, event_origin_t origin, volume_t new_volume)
{
    volume_t source_volume = AudioSources_GetVolume(source);
    DEBUG_LOG("VolumeService_SetAudioSourceVolume, source %u, origin %u, volume %u", source, origin, new_volume.value);
    source_volume.value = VolumeUtils_ConvertToVolumeConfig(new_volume, source_volume.config);

    if(AudioSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        AudioSources_VolumeSetAbsolute(source, source_volume);
    }
    else
    {
        volumeService_UpdateAudioSourceVolume(source, source_volume, origin);
    }
    volumeService_NotifyMinOrMaxVolume(source_volume);
}

void VolumeService_IncrementAudioSourceVolume(audio_source_t source, event_origin_t origin)
{
    DEBUG_LOG("VolumeService_IncrementAudioSourceVolume, source %u, origin %u", source, origin);
    if (AudioSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        AudioSources_VolumeUp(source);
    }
    else
    {
        volume_t source_volume = AudioSources_GetVolume(source);
        source_volume.value = VolumeUtils_IncrementVolume(source_volume);
        volumeService_UpdateAudioSourceVolume(source, source_volume, origin);
        volumeService_NotifyMinOrMaxVolume(source_volume);
    }
}

void VolumeService_DecrementAudioSourceVolume(audio_source_t source, event_origin_t origin)
{
    DEBUG_LOG("VolumeService_DecrementAudioSourceVolume, source %u, origin %u", source, origin);
    if (AudioSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        AudioSources_VolumeDown(source);
    }
    else
    {
        volume_t source_volume = AudioSources_GetVolume(source);
        source_volume.value = VolumeUtils_DecrementVolume(source_volume);
        volumeService_UpdateAudioSourceVolume(source, source_volume, origin);
        volumeService_NotifyMinOrMaxVolume(source_volume);
    }
}

void VolumeService_SetSystemVolume(event_origin_t origin, volume_t new_volume)
{
    volume_t system_volume = Volume_GetSystemVolume();
    system_volume.value = VolumeUtils_ConvertToVolumeConfig(new_volume, system_volume.config);
    volumeService_UpdateSystemVolume(system_volume, origin);
}

void VolumeService_IncrementSystemVolume(event_origin_t origin)
{
    volume_t system_volume = Volume_GetSystemVolume();
    system_volume.value = VolumeUtils_IncrementVolume(system_volume);
    volumeService_UpdateSystemVolume(system_volume, origin);
}

void VolumeService_DecrementSystemVolume(event_origin_t origin)
{
    volume_t system_volume = Volume_GetSystemVolume();
    system_volume.value = VolumeUtils_DecrementVolume(system_volume);
    volumeService_UpdateSystemVolume(system_volume, origin);
}

void VolumeService_SetVoiceSourceVolume(voice_source_t source, event_origin_t origin, volume_t new_volume)
{
    volume_t source_volume = VoiceSources_GetVolume(source);
    DEBUG_LOG("VolumeService_SetVoiceSourceVolume, source %u, origin %u, volume %u", source, origin, new_volume.value);
    source_volume.value = VolumeUtils_ConvertToVolumeConfig(new_volume, source_volume.config);

    if(VoiceSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        VoiceSources_VolumeSetAbsolute(source, source_volume);
    }
    else
    {
        volumeService_UpdateVoiceSourceLocalVolume(source, source_volume, origin);
    }
    volumeService_NotifyMinOrMaxVolume(source_volume);
}

void VolumeService_IncrementVoiceSourceVolume(voice_source_t source, event_origin_t origin)
{
    DEBUG_LOG("VolumeService_IncrementVoiceSourceVolume, source %u, origin %u", source, origin);
    if (VoiceSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        VoiceSources_VolumeUp(source);
    }
    else
    {
        volume_t source_volume = VoiceSources_GetVolume(source);
        source_volume.value = VolumeUtils_IncrementVolume(source_volume);
        volumeService_UpdateVoiceSourceLocalVolume(source, source_volume, origin);
        volumeService_NotifyMinOrMaxVolume(source_volume);
    }
}

void VolumeService_DecrementVoiceSourceVolume(voice_source_t source, event_origin_t origin)
{
    DEBUG_LOG("VolumeService_DecrementVoiceSourceVolume, source %u, origin %u", source, origin);
    if (VoiceSources_IsVolumeControlRegistered(source) && (origin == event_origin_local))
    {
        VoiceSources_VolumeDown(source);
    }
    else
    {
        volume_t source_volume = VoiceSources_GetVolume(source);
        source_volume.value = VolumeUtils_DecrementVolume(source_volume);
        volumeService_UpdateVoiceSourceLocalVolume(source, source_volume, origin);
        volumeService_NotifyMinOrMaxVolume(source_volume);
    }
}

void VolumeService_Mute(void)
{
    Volume_SetMuteState(TRUE);
    volumeService_RefreshCurrentVolume(event_origin_local);
}

void VolumeService_Unmute(void)
{
    Volume_SetMuteState(FALSE);
    volumeService_RefreshCurrentVolume(event_origin_local);
}

static void volumeMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case VOICE_SOURCE_VOLUME_UPDATE_REQUEST:
            {
                voice_source_volume_update_request_message_t *msg = (voice_source_volume_update_request_message_t *)message;
                VolumeService_SetVoiceSourceVolume(msg->voice_source, msg->origin, msg->volume);
            }
            break;
        case VOICE_SOURCE_VOLUME_INCREMENT_REQUEST:
            {
                voice_source_volume_increment_request_message_t *msg = (voice_source_volume_increment_request_message_t *)message;
                VolumeService_IncrementVoiceSourceVolume(msg->voice_source, msg->origin);
            }
            break;
        case VOICE_SOURCE_VOLUME_DECREMENT_REQUEST:
            {
                voice_source_volume_decrement_request_message_t *msg = (voice_source_volume_decrement_request_message_t *)message;
                VolumeService_DecrementVoiceSourceVolume(msg->voice_source, msg->origin);
            }
            break;
        case AUDIO_SOURCE_VOLUME_UPDATE_REQUEST:
            {
                audio_source_volume_update_request_message_t *msg = (audio_source_volume_update_request_message_t *)message;
                VolumeService_SetAudioSourceVolume(msg->audio_source, msg->origin, msg->volume);
            }
            break;
        case AUDIO_SOURCE_VOLUME_INCREMENT_REQUEST:
            {
                audio_source_volume_increment_request_message_t *msg = (audio_source_volume_increment_request_message_t *)message;
                VolumeService_IncrementAudioSourceVolume(msg->audio_source, msg->origin);
            }
            break;
        case AUDIO_SOURCE_VOLUME_DECREMENT_REQUEST:
            {
                audio_source_volume_decrement_request_message_t *msg = (audio_source_volume_decrement_request_message_t *)message;
                VolumeService_DecrementAudioSourceVolume(msg->audio_source, msg->origin);
            }
            break;
        case MUTE_VOLUME_REQUEST:
            {
                mute_volume_request_message_t *msg = (mute_volume_request_message_t *)message;
                if(msg->mute_state)
                {
                    VolumeService_Mute();
                }
                else
                {
                    VolumeService_Unmute();
                }
            }
            break;
        default:
            break;
    }
}

bool VolumeService_Init(Task init_task)
{
    UNUSED(init_task);
    TaskList_InitialiseWithCapacity(VolumeServiceGetClientLIst(), VOLUME_SERVICE_CLIENT_TASK_LIST_INIT_CAPACITY);

    the_volume_service.volume_message_handler_task.handler = volumeMessageHandler;
    Volume_RegisterForMessages(&the_volume_service.volume_message_handler_task);

    return TRUE;
}

static void volumeService_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == VOLUME_SERVICE_MESSAGE_GROUP);
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(VolumeServiceGetClientLIst()), task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(VOLUME_SERVICE, volumeService_RegisterMessageGroup, NULL);
