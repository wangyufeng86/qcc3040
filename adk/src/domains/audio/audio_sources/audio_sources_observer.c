/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio_sources_observer composite.
*/

#include "audio_sources.h"

#include <panic.h>

#include "audio_sources_interface_registry.h"

void AudioSources_RegisterObserver(audio_source_t source, const audio_source_observer_interface_t * observer)
{
    uint8 index = 0;

    /* Retrieve the registered inteface list */
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_observer_registry);
    audio_source_observer_interface_t **observer_interface_list = (audio_source_observer_interface_t **)interface_list.interfaces;

    /* Check if the interface is already registered */
    if(interface_list.number_of_interfaces)
    {
        while(index < interface_list.number_of_interfaces)
        {
            if(observer == observer_interface_list[index])
                return;
            index++;
        }

        if(interface_list.number_of_interfaces == MAX_OBSERVER_INTERFACES)
            Panic();
    }

    /* Register the interface */
    AudioInterface_Register(source, audio_interface_type_observer_registry, observer);
}

void AudioSources_OnVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume)
{
    uint8 index =0;
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_observer_registry);
    audio_source_observer_interface_t **observer_interface_list = (audio_source_observer_interface_t **)interface_list.interfaces;
    
    while(index < interface_list.number_of_interfaces)
    {
        audio_source_observer_interface_t *interface = observer_interface_list[index];
        if((interface != NULL) && (interface->OnVolumeChange))
        {
            interface->OnVolumeChange(source, origin, volume);
        }
        index++;
    }
}


