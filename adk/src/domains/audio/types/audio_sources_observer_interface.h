/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to audio_sources_observer - provides a mechanism for observing specific
            changes within an audio source.
*/

#ifndef AUDIO_SOURCES_OBSERVER_INTERFACE_H_
#define AUDIO_SOURCES_OBSERVER_INTERFACE_H_

#include "audio_sources_list.h"
#include "volume_types.h"

#define MAX_OBSERVER_INTERFACES (2)

/*! \brief The audio source observer interface
*/
typedef struct
{
    void (*OnVolumeChange)(audio_source_t source, event_origin_t origin, volume_t volume);
} audio_source_observer_interface_t;

#endif /* AUDIO_SOURCES_OBSERVER_INTERFACE_H_ */
