/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   volume_messages Volume Messages
\ingroup    volume_group
\brief      Functions for generating volume update request messages

            These are intended primarily for the volume service in order for it to
            carry out the volume request
*/

#ifndef VOLUME_MESSAGES_H_
#define VOLUME_MESSAGES_H_

#include "audio_sources_list.h"
#include "domain_message.h"
#include "voice_sources_list.h"
#include "volume_types.h"

/*\{*/

/*! \Messages sent by the volume domain to interested clients.
*/
enum volume_domain_messages
{
    VOICE_SOURCE_VOLUME_UPDATE_REQUEST = VOLUME_MESSAGE_BASE,
    VOICE_SOURCE_VOLUME_INCREMENT_REQUEST,
    VOICE_SOURCE_VOLUME_DECREMENT_REQUEST,
    AUDIO_SOURCE_VOLUME_UPDATE_REQUEST,
    AUDIO_SOURCE_VOLUME_INCREMENT_REQUEST,
    AUDIO_SOURCE_VOLUME_DECREMENT_REQUEST,
    MUTE_VOLUME_REQUEST,
};

/*! Volume message data typedefs */
typedef struct
{
    voice_source_t voice_source;
    event_origin_t origin;
    volume_t volume;
} voice_source_volume_update_request_message_t;

typedef struct
{
    voice_source_t voice_source;
    event_origin_t origin;
} voice_source_volume_increment_request_message_t, voice_source_volume_decrement_request_message_t;

typedef struct
{
    audio_source_t audio_source;
    event_origin_t origin;
    volume_t volume;
} audio_source_volume_update_request_message_t;

typedef struct
{
    audio_source_t audio_source;
    event_origin_t origin;
} audio_source_volume_increment_request_message_t, audio_source_volume_decrement_request_message_t;

typedef struct
{
    bool mute_state;
} mute_volume_request_message_t;


/*! \brief Setup the volume message delivery framework.

    \param init_task Not used.

    \return TRUE
 */
bool Volume_InitMessages(Task init_task);

/*! \brief Register task to receive volume messages.

    \param task_to_register
 */
void Volume_RegisterForMessages(Task task_to_register);

/*! \brief Request an update to a voice source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
    \param volume The requested volume value

    The volume value is expected to be in the form of whatever the current volume config for
    the specified source is
 */
void Volume_SendVoiceSourceVolumeUpdateRequest(voice_source_t source, event_origin_t origin, int volume);

/*! \brief Request an increment to a voice source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
 */
void Volume_SendVoiceSourceVolumeIncrementRequest(voice_source_t source, event_origin_t origin);

/*! \brief Request an decrement to a voice source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
 */
void Volume_SendVoiceSourceVolumeDecrementRequest(voice_source_t source, event_origin_t origin);

/*! \brief Request an update to an audio source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
    \param volume The requested volume (value)

    The volume value is expected to be in the form of whatever the current volume config for
    the specified source is
 */
void Volume_SendAudioSourceVolumeUpdateRequest(audio_source_t source, event_origin_t origin, int volume);

/*! \brief Request an increment to an audio source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
 */
void Volume_SendAudioSourceVolumeIncrementRequest(audio_source_t source, event_origin_t origin);

/*! \brief Request a decrement to an audio source volume.

    \param source The source volume to update
    \param origin The origin of the volume update request
 */
void Volume_SendAudioSourceVolumeDecrementRequest(audio_source_t source, event_origin_t origin);

/*! \brief Request to mute the audio output.

    \param mute_state TRUE to mute, FALSE to unmute
 */
void Volume_MuteRequest(bool mute_state);

/*\}*/

#endif /* VOLUME_MESSAGES_H_ */
