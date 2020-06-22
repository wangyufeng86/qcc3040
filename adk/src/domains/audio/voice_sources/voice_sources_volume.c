/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice_sources_volume composite.
*/

#include "voice_sources.h"
#include "volume_types.h"

#include <panic.h>

/*! \brief The voice source volume registry

    References to volume interfaces are stored here as they are registered
*/
static const voice_source_volume_interface_t * voice_volumes[max_voice_sources];

static void voiceSources_ValidateSource(voice_source_t source)
{
    if((source <= voice_source_none) || (source >= max_voice_sources))
    {
        Panic();
    }
}

static bool voiceSources_IsSourceRegistered(voice_source_t source)
{
    return ((voice_volumes[source] == NULL) ? FALSE : TRUE);
}

void VoiceSources_VolumeRegistryInit(void)
{
    memset(voice_volumes, 0, sizeof(voice_volumes));
}

void VoiceSources_RegisterVolume(voice_source_t source, const voice_source_volume_interface_t * interface)
{
    voiceSources_ValidateSource(source);
    PanicNull((void *)interface);
    voice_volumes[source] = interface;
}

volume_t VoiceSources_GetVolume(voice_source_t source)
{
    volume_t volume = FULL_SCALE_VOLUME;
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && voice_volumes[source]->GetVolume)
    {
        volume = voice_volumes[source]->GetVolume(source);
    }
    return volume;
}

void VoiceSources_SetVolume(voice_source_t source, volume_t volume)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && voice_volumes[source]->SetVolume)
    {
        voice_volumes[source]->SetVolume(source, volume);
    }
}
