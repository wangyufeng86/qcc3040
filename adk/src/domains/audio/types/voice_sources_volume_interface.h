/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to volume for voice sources - provides a mechanism for accessing
            the volume associated with a voice source.
*/

#ifndef VOICE_SOURCES_VOLUME_INTERFACE_H_
#define VOICE_SOURCES_VOLUME_INTERFACE_H_

#include "voice_sources_list.h"
#include "volume_types.h"

/*! \brief The voice source volume interface
*/
typedef struct
{
    volume_t (*GetVolume)(voice_source_t source);
    void (*SetVolume)(voice_source_t source, volume_t volume);
} voice_source_volume_interface_t;

#endif /* VOICE_SOURCES_VOLUME_INTERFACE_H_ */
