/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice assistants.
*/
#include <panic.h>
#include <stdlib.h>
#include "voice_ui_container.h"
#include "voice_ui_gaia_plugin.h"
#include "voice_ui_peer_sig.h"

#include <bt_device.h>
#include <device_db_serialiser.h>
#include <device_properties.h>

static voice_ui_handle_t *active_va = NULL;

/*! \brief Container that holds the handle to created voice assistants */
static voice_ui_handle_t* voice_assistant_list[MAX_NO_VA_SUPPORTED] = {0};

static voice_ui_handle_t* voiceUi_FindHandleForProvider(voice_ui_provider_t provider_name);
static voice_ui_handle_t* voiceUi_GetHandleFromProvider(voice_ui_provider_t provider_name);
static bool voiceUi_ProviderIsValid(voice_ui_provider_t provider_name);
static voice_ui_provider_t voiceUi_GetActiveVaProvider(void);
static void voiceUi_SetA2dpStreamState(voice_ui_a2dp_state_t a2dp_stream_state);

static voice_ui_protected_if_t voice_assistant_protected_if =
{
    voiceUi_GetActiveVaProvider,
    voiceUi_SetA2dpStreamState,
};

static voice_ui_provider_t voiceUi_GetActiveVaProvider(void)
{
    if (active_va && active_va->voice_assistant)
        return active_va->voice_assistant->va_provider;
    else
        return voice_ui_provider_none;
}

void voiceUi_SetA2dpStreamState(voice_ui_a2dp_state_t a2dp_stream_state)
{
    active_va->va_a2dp_state = a2dp_stream_state;
}

static voice_ui_handle_t* voiceUi_FindHandleForProvider(voice_ui_provider_t provider_name)
{
    /* Find corresponding handle in the container */
    voice_ui_handle_t *va_provider_handle = NULL;
    for (uint8 va_index = 0; va_index < MAX_NO_VA_SUPPORTED; va_index++)
    {
        voice_ui_handle_t* va_handle = voice_assistant_list[va_index];
        if (va_handle && va_handle->voice_assistant)
        {
            if (va_handle->voice_assistant->va_provider == provider_name)
            {
                va_provider_handle = va_handle;
                break;
            }
        }
    }
	
    return va_provider_handle;
}

static voice_ui_handle_t* voiceUi_GetHandleFromProvider(voice_ui_provider_t provider_name)
{
    return PanicNull(voiceUi_FindHandleForProvider(provider_name));
}

static bool voiceUi_ProviderIsValid(voice_ui_provider_t provider_name)
{
    return voiceUi_FindHandleForProvider(provider_name) != NULL;
}

void VoiceUi_SetSelectedAssistant(uint8 voice_ui_provider)
{
   bdaddr bd_addr;
   appDeviceGetMyBdAddr(&bd_addr);
   device_t device = BtDevice_GetDeviceForBdAddr(&bd_addr);
   bool device_instance_exists = device != NULL;
   if(device_instance_exists)
   {
       Device_SetPropertyU8(device, device_property_voice_assistant, voice_ui_provider);
       DeviceDbSerialiser_Serialise();
   }
    VoiceUiGaiaPlugin_NotifyAssistantChanged(voice_ui_provider);
}

static void voiceUi_DeselectCurrentAssistant(void)
{
    if (active_va)
    {
        active_va->voice_assistant->DeselectVoiceAssitant();
        active_va = NULL;
    }
}

voice_ui_protected_if_t* VoiceUi_Register(voice_ui_if_t *va_table)
{
    int va_index;
    voice_ui_handle_t* va_handle = NULL;

    /* Find a free slot in the container */
    for(va_index=0;va_index<MAX_NO_VA_SUPPORTED;va_index++)
    {
        if(!voice_assistant_list[va_index])
            break;
    }
    PanicFalse(va_index < MAX_NO_VA_SUPPORTED);

    va_handle = (voice_ui_handle_t*)PanicUnlessMalloc(sizeof(voice_ui_handle_t));
    va_handle->va_a2dp_state = voice_ui_a2dp_state_suspended;
    va_handle->voice_assistant = va_table;
    va_handle->va_protected_if = &voice_assistant_protected_if;
    voice_assistant_list[va_index] = va_handle;

    voice_ui_provider_t registering_va_provider = va_handle->voice_assistant->va_provider;
    if (VoiceUi_GetSelectedAssistant() == registering_va_provider)
    {
        active_va = va_handle;
    }

    return va_handle->va_protected_if;
}

void VoiceUi_UnRegister(voice_ui_provider_t va_provider)
{
   int i;
   for(i=0; i < MAX_NO_VA_SUPPORTED; i++)
   {
       if(voice_assistant_list[i] && (va_provider == voice_assistant_list[i]->voice_assistant->va_provider))
       {
           free((void*)voice_assistant_list[i]);
           voice_assistant_list[i] = NULL;
           break;
       }
    }
}

voice_ui_handle_t* VoiceUi_GetActiveVa(void)
{
    return active_va;
}

voice_ui_provider_t VoiceUi_GetSelectedAssistant(void)
{
    uint8 selected_va = VOICE_UI_PROVIDER_DEFAULT;
    bdaddr bd_addr;
    appDeviceGetMyBdAddr(&bd_addr);
    device_t device = BtDevice_GetDeviceForBdAddr(&bd_addr);
    bool device_instance_exists = device != NULL;
    if(device_instance_exists)
    {
        if (Device_GetPropertyU8(device, device_property_voice_assistant, &selected_va))
        {
            if (selected_va == voice_ui_provider_none)
            {
               selected_va = VOICE_UI_PROVIDER_DEFAULT;
            }
        }
    }
    return selected_va;
}

uint16 VoiceUi_GetSupportedAssistants(uint8 *assistants)
{
    uint16 count = 0;
    uint16 va_index;

    PanicNull(assistants);

    /* Explicit support for 'none' */
    assistants[count++] = voice_ui_provider_none;

    for (va_index = 0; va_index < MAX_NO_VA_SUPPORTED; ++va_index)
    {
        if (voice_assistant_list[va_index] && voice_assistant_list[va_index]->voice_assistant)
        {
            assistants[count++] = voice_assistant_list[va_index]->voice_assistant->va_provider;
        }
    }

    return count;
}

bool VoiceUi_SelectVoiceAssistant(voice_ui_provider_t va_provider)
{
    bool status = FALSE;
    bool reboot = FALSE;

    if (va_provider == VoiceUi_GetSelectedAssistant())
    {
    /*  Nothing to do  */
        status = TRUE;
    }
    else
    {
        if (va_provider == voice_ui_provider_none)
        {
            voiceUi_DeselectCurrentAssistant();
            status = TRUE;
        }
        else if (voiceUi_ProviderIsValid(va_provider))
        {
            voiceUi_DeselectCurrentAssistant();
            active_va = voiceUi_GetHandleFromProvider(va_provider);
            active_va->voice_assistant->SelectVoiceAssitant();
            status = TRUE;
            reboot = va_provider == voice_ui_provider_gaa;
        }
    }

    if (status)
    {
        VoiceUi_SetSelectedAssistant(va_provider);
        VoiceUi_UpdateSelectedPeerVaProvider(reboot);
    }

    return status;
}

void VoiceUi_EventHandler(voice_ui_handle_t* va_handle, ui_input_t event_id)
{
   if(va_handle)
   {
        if(va_handle->voice_assistant->EventHandler)
        {
            va_handle->voice_assistant->EventHandler(event_id);
        }
   }
}

void VoiceUi_Suspend(voice_ui_handle_t* va_handle)
{
   if(va_handle)
   {
        if(va_handle->voice_assistant->Suspend)
        {
            va_handle->voice_assistant->Suspend();
        }
   }
}

bool VoiceUi_IsVoiceAssistantA2dpStreamActive(void)
{
    bool voice_ui_a2dp_stream_state = FALSE;
    if(active_va) {
        voice_ui_a2dp_stream_state = (active_va->va_a2dp_state == voice_ui_a2dp_state_streaming);
    }
    return voice_ui_a2dp_stream_state;
}
