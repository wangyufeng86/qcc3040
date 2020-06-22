/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice_sources_observer composite.
*/

#include "voice_sources.h"
#include "volume_types.h"

#include <panic.h>

/*! \brief The voice source observer registry

    References to observer interfaces are stored here as they are registered
*/
static const voice_source_observer_interface_t * voice_source_observers[max_voice_sources];

static void voiceSources_ValidateSource(voice_source_t source)
{
    if((source <= voice_source_none) || (source >= max_voice_sources))
    {
        Panic();
    }
}

static bool voiceSources_IsSourceRegistered(voice_source_t source)
{
    return ((voice_source_observers[source] == NULL) ? FALSE : TRUE);
}

void VoiceSources_ObserverRegistryInit(void)
{
    memset(voice_source_observers, 0, sizeof(voice_source_observers));
}

void VoiceSources_RegisterObserver(voice_source_t source, const voice_source_observer_interface_t * interface)
{
    voiceSources_ValidateSource(source);
    PanicNull((void *)interface);
    voice_source_observers[source] = interface;
}

void VoiceSources_OnVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && voice_source_observers[source]->OnVolumeChange)
    {
        voice_source_observers[source]->OnVolumeChange(source, origin, volume);
    }
}
