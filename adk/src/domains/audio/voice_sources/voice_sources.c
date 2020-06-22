/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Miscellaneous voice source functions
*/

#include "voice_sources.h"

bool VoiceSources_Init(Task init_task)
{
    UNUSED(init_task);
    VoiceSources_AudioRegistryInit();
    VoiceSources_VolumeRegistryInit();
    VoiceSources_VolumeControlRegistryInit();
    VoiceSources_ObserverRegistryInit();
    return TRUE;
}

voice_source_t VoiceSources_GetRoutedSource(void)
{
    voice_source_t source = voice_source_none;
    while(++source < max_voice_sources)
    {
        if(VoiceSources_IsAudioAvailable(source))
        {
            break;
        }
    }
    if(source == max_voice_sources)
    {
        source = voice_source_none;
    }
    return source;
}

bool VoiceSources_IsVoiceRouted(void)
{
    return (VoiceSources_GetRoutedSource()? TRUE :FALSE);
}
