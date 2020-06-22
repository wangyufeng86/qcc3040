/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to volume control for audio sources - provides a mechanism for controlling
            volume on sources that require an alternate implementation.
*/

#ifndef AUDIO_SOURCES_VOLUME_CONTROL_INTERFACE_H_
#define AUDIO_SOURCES_VOLUME_CONTROL_INTERFACE_H_

#include "audio_sources_list.h"
#include "volume_types.h"

#define MAX_VOLUME_CONTROL_INTERFACES (1)

/*! \brief The audio source volume control interface
*/
typedef struct
{
    void (*VolumeUp)(audio_source_t source);
    void (*VolumeDown)(audio_source_t source);
    void (*VolumeSetAbsolute)(audio_source_t source, volume_t volume);
    void (*Mute)(audio_source_t source, mute_state_t state);
} audio_source_volume_control_interface_t;

#endif /* AUDIO_SOURCES_VOLUME_CONTROL_INTERFACE_H_ */
