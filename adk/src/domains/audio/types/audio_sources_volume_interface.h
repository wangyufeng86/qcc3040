/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to volume for audio sources - provides a mechanism for accessing
            the volume associated with an audio source.
*/

#ifndef AUDIO_SOURCES_VOLUME_INTERFACE_H_
#define AUDIO_SOURCES_VOLUME_INTERFACE_H_

#include "audio_sources_list.h"
#include "volume_types.h"

#define MAX_VOLUME_INTERFACES (1)

/*! \brief The audio source volume interface
*/
typedef struct
{
    volume_t (*GetVolume)(audio_source_t source);
    void (*SetVolume)(audio_source_t source, volume_t volume);
} audio_source_volume_interface_t;

#endif /* AUDIO_SOURCES_VOLUME_INTERFACE_H_ */
