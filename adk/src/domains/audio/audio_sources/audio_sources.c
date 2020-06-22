/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Miscellaneous audio source functions
*/

#include "audio_sources.h"

#include "audio_sources_interface_registry.h"

bool AudioSources_Init(Task init_task)
{
    UNUSED(init_task);
    AudioInterface_Init();

    return TRUE;
}

audio_source_t AudioSources_GetRoutedSource(void)
{
    audio_source_t source = audio_source_none;
    while(++source < max_audio_sources)
    {
        if(AudioSources_IsAudioAvailable(source))
        {
            break;
        }
    }
    if(source == max_audio_sources)
    {
        source = audio_source_none;
    }
    return source;
}
