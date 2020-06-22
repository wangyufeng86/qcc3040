/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio_sources_audio composite.
*/

#include "audio_sources.h"

#include <logging.h>
#include <panic.h>

#include "audio_sources_interface_registry.h"


/*! \brief Interface obtaining function

    \param source Audio source type

    \return Pointer to the interface
*/
static audio_source_audio_interface_t *audioSources_GetRegisteredInterface(audio_source_t source)
{
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_audio_source_registry);
    return (((audio_source_audio_interface_t **)interface_list.interfaces)[0]);
}

void AudioSources_RegisterAudioInterface(audio_source_t source, const audio_source_audio_interface_t * interface)
{
    PanicNull((void *)interface);
    DEBUG_LOG("AudioSources_RegisterAudioInterface source=%d interface=%p", source, interface);

    /* Retrieve the registered inteface list */
    interface_list_t interface_list = AudioInterface_Get(source, audio_interface_type_audio_source_registry);
    audio_source_audio_interface_t **audio_interface_list = (audio_source_audio_interface_t **)interface_list.interfaces;

    /* Check if the interface is already registered */
    if(interface_list.number_of_interfaces)
    {
        if(interface == audio_interface_list[0])
            return;

        /* If the interface does not match, unregister the old interface */
        AudioInterface_UnRegister(source, audio_interface_type_audio_source_registry, audio_interface_list[0]);
    }

    /* Register the interface*/
    AudioInterface_Register(source, audio_interface_type_audio_source_registry, interface);
}

bool AudioSources_GetConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    bool return_value = FALSE;
    audio_source_audio_interface_t * interface = audioSources_GetRegisteredInterface(source);

    if ((interface != NULL) && (interface->GetConnectParameters))
    {
        DEBUG_LOG("AudioSources_GetConnectParameters calling interface->GetConnectParameters(%d, %p)", source, source_params);
        return_value = interface->GetConnectParameters(source, source_params);
    }
    return return_value;
}

void AudioSources_ReleaseConnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    audio_source_audio_interface_t * interface = audioSources_GetRegisteredInterface(source);

    if ((interface != NULL) && (interface->ReleaseConnectParameters))
    {
        DEBUG_LOG("AudioSources_ReleaseConnectParameters calling interface->ReleaseConnectParameters(%d, %p)", source, source_params);
        interface->ReleaseConnectParameters(source, source_params);
    }
}

bool AudioSources_GetDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    bool return_value = FALSE;
    audio_source_audio_interface_t * interface = audioSources_GetRegisteredInterface(source);

    if ((interface != NULL) && (interface->GetDisconnectParameters))
    {
        DEBUG_LOG("AudioSources_GetDisconnectParameters calling interface->GetDisconnectParameters(%d, %p)", source, source_params);
        return_value = interface->GetDisconnectParameters(source, source_params);
    }
    return return_value;
}

void AudioSources_ReleaseDisconnectParameters(audio_source_t source, source_defined_params_t * source_params)
{
    audio_source_audio_interface_t * interface = audioSources_GetRegisteredInterface(source);

    if ((interface != NULL) && (interface->ReleaseDisconnectParameters))
    {
        DEBUG_LOG("AudioSources_ReleaseDisconnectParameters calling interface->ReleaseDisconnectParameters(%d, %p)", source, source_params);
        interface->ReleaseDisconnectParameters(source, source_params);
    }
}

bool AudioSources_IsAudioAvailable(audio_source_t source)
{
    audio_source_audio_interface_t * interface = audioSources_GetRegisteredInterface(source);
    bool is_available = FALSE;

    if ((interface != NULL) && (interface->IsAudioAvailable))
    {
        is_available = interface->IsAudioAvailable(source);
        DEBUG_LOG("AudioSources_IsAudioAvailable source=%d, available=%d", source, is_available);
    }
    return is_available;
}

