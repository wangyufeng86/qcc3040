/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice_sources_volume_control composite.
*/

#include "voice_sources.h"
#include <panic.h>

/*! \brief The voice source volume control registry

    References to volume control interfaces are stored here as they are registered
*/
static const voice_source_volume_control_interface_t * volume_control_interface[max_voice_sources];

static void voiceSources_ValidateSource(voice_source_t source)
{
    if((source <= voice_source_none) || (source >= max_voice_sources))
    {
        Panic();
    }
}

static bool voiceSources_IsSourceRegistered(voice_source_t source)
{
    return ((volume_control_interface[source] == NULL) ? FALSE : TRUE);
}

void VoiceSources_VolumeControlRegistryInit(void)
{
    memset(volume_control_interface, 0, sizeof(volume_control_interface));
}

void VoiceSources_RegisterVolumeControl(voice_source_t source, const voice_source_volume_control_interface_t * interface)
{
    voiceSources_ValidateSource(source);
    PanicNull((void *)interface);
    volume_control_interface[source] = interface;
}

bool VoiceSources_IsVolumeControlRegistered(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    return voiceSources_IsSourceRegistered(source);
}

void VoiceSources_VolumeUp(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (volume_control_interface[source]->VolumeUp))
    {
        volume_control_interface[source]->VolumeUp(source);
    }
}

void VoiceSources_VolumeDown(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (volume_control_interface[source]->VolumeDown))
    {
        volume_control_interface[source]->VolumeDown(source);
    }
}

void VoiceSources_VolumeSetAbsolute(voice_source_t source, volume_t volume)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (volume_control_interface[source]->VolumeSetAbsolute))
    {
        volume_control_interface[source]->VolumeSetAbsolute(source, volume);
    }
}

void VoiceSources_Mute(voice_source_t source, mute_state_t state)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (volume_control_interface[source]->Mute))
    {
        volume_control_interface[source]->Mute(source, state);
    }
}

