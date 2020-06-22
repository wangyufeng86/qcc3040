/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to volume control for voice sources - provides a mechanism for controlling
            volume on sources that require an alternate implementation.
*/

#ifndef VOICE_SOURCES_VOLUME_CONTROL_INTERFACE_H_
#define VOICE_SOURCES_VOLUME_CONTROL_INTERFACE_H_

#include "voice_sources_list.h"
#include "volume_types.h"

/*! \brief The voice source volume control interface
*/
typedef struct
{
    void (*VolumeUp)(voice_source_t source);
    void (*VolumeDown)(voice_source_t source);
    void (*VolumeSetAbsolute)(voice_source_t source, volume_t volume);
    void (*Mute)(voice_source_t source, mute_state_t state);
} voice_source_volume_control_interface_t;


#endif /* VOICE_SOURCES_VOLUME_CONTROL_INTERFACE_H_ */
