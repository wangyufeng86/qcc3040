/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to media control for audio sources - provides a mechanism for controlling
            media playback.
*/

#ifndef AUDIO_SOURCES_MEDIA_CONTROL_INTERFACE_H_
#define AUDIO_SOURCES_MEDIA_CONTROL_INTERFACE_H_

#include "media_control_types.h"
#include "audio_sources_list.h"

#define MAX_MEDIA_CONTROL_INTERFACES (1)

typedef struct
{
    void (*Play)(audio_source_t source);
    void (*Pause)(audio_source_t source);
    void (*PlayPause)(audio_source_t source);
    void (*Stop)(audio_source_t source);
    void (*Forward)(audio_source_t source);
    void (*Back)(audio_source_t source);
    void (*FastForward)(audio_source_t source, bool state);
    void (*FastRewind)(audio_source_t source, bool state);
    void (*NextGroup)(audio_source_t source);
    void (*PreviousGroup)(audio_source_t source);
    void (*Shuffle)(audio_source_t source, shuffle_state_t state);
    void (*Repeat)(audio_source_t source, repeat_state_t state);
    unsigned (*Context)(audio_source_t source);
} media_control_interface_t;

#endif /* AUDIO_SOURCES_MEDIA_CONTROL_INTERFACE_H_ */
