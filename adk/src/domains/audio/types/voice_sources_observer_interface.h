/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to voice_sources_observer - provides a mechanism for observing specific
            changes within an voice source.
*/

#ifndef VOICE_SOURCES_OBSERVER_INTERFACE_H_
#define VOICE_SOURCES_OBSERVER_INTERFACE_H_

#include "voice_sources_list.h"
#include "volume_types.h"

/*! \brief The voice source observer interface
*/
typedef struct
{
    void (*OnVolumeChange)(voice_source_t source, event_origin_t origin, volume_t volume);
} voice_source_observer_interface_t;

#endif /* VOICE_SOURCES_OBSERVER_INTERFACE_H_ */
