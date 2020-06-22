/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio_sources_volume_control composite.
*/

#include "audio_sources.h"

#include <panic.h>

#include "audio_sources_interface_registry.h"


/*! \brief Interface obtaining function

    \param source Audio source type

    \return Pointer to the interface
*/
static audio_source_volume_control_interface_t *audioSources_GetVolumeControlInterface(audio_source_t source)
{
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_volume_control);
    return (((audio_source_volume_control_interface_t **)interface_list.interfaces)[0]);
}


void AudioSources_RegisterVolumeControl(audio_source_t source, const audio_source_volume_control_interface_t * interface)
{
    /* Retrieve the registered inteface list */
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_volume_control);
    audio_source_volume_control_interface_t **volume_control_interface_list = (audio_source_volume_control_interface_t **)interface_list.interfaces;

    /* Check if the interface is already registered */
    if(interface_list.number_of_interfaces)
    {
        if(interface == volume_control_interface_list[0])
            return;

        /* If the interface does not match, unregister the old interface */
        AudioInterface_UnRegister(source, audio_interface_type_volume_control, volume_control_interface_list[0]);
    }

    /* Register the interface */
    AudioInterface_Register(source, audio_interface_type_volume_control, interface);
}

bool AudioSources_IsVolumeControlRegistered(audio_source_t source)
{
    return ((audioSources_GetVolumeControlInterface(source) == NULL) ? FALSE : TRUE);
}

void AudioSources_VolumeUp(audio_source_t source)
{
    audio_source_volume_control_interface_t * interface = audioSources_GetVolumeControlInterface(source);

    if ((interface != NULL) && (interface->VolumeUp))
    {
        interface->VolumeUp(source);
    }
}

void AudioSources_VolumeDown(audio_source_t source)
{
    audio_source_volume_control_interface_t * interface = audioSources_GetVolumeControlInterface(source);

    if ((interface != NULL) && (interface->VolumeDown))
    {
        interface->VolumeDown(source);
    }
}

void AudioSources_VolumeSetAbsolute(audio_source_t source, volume_t volume)
{
    audio_source_volume_control_interface_t * interface = audioSources_GetVolumeControlInterface(source);

    if ((interface != NULL) && (interface->VolumeSetAbsolute))
    {
        interface->VolumeSetAbsolute(source, volume);
    }
}

void AudioSources_Mute(audio_source_t source, mute_state_t state)
{
    audio_source_volume_control_interface_t * interface = audioSources_GetVolumeControlInterface(source);

    if ((interface != NULL) && (interface->Mute))
    {
        interface->Mute(source, state);
    }
}
