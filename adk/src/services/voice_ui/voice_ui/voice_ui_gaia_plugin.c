/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      GAIA interface with Voice Assistants.
*/

#include "gaia_framework.h"
#include "voice_ui_container.h"
#include "voice_ui_gaia_plugin.h"
#include <gaia_features.h>
#include <logging.h>


/* may report voice_ui_provider_none as well as implemented providers */
#define MAX_NO_VA_REPORTED (MAX_NO_VA_SUPPORTED + 1)

static void voiceUiGaiaPlugin_SendNotSupported(uint8 pdu_id)
{
    GaiaFramework_SendError(GAIA_VOICE_UI_FEATURE_ID, pdu_id, GAIA_STATUS_NOT_SUPPORTED);
}


static void voiceUiGaiaPlugin_SendInvalidParameter(uint8 pdu_id)
{
    GaiaFramework_SendError(GAIA_VOICE_UI_FEATURE_ID, pdu_id, GAIA_STATUS_INVALID_PARAMETER);
}


static void voiceUiGaiaPlugin_SendResponse(uint8 pdu_id, uint8 length, uint8 * payload)
{
    GaiaFramework_SendResponse(GAIA_VOICE_UI_FEATURE_ID, pdu_id, length, payload);
}


static void voiceUiGaiaPlugin_GetSelectedAssistant(void)
{
    uint8 selected_assistant = VoiceUi_GetSelectedAssistant();
    voiceUiGaiaPlugin_SendResponse(voice_ui_gaia_get_selected_assistant, 1, &selected_assistant);
}


static void voiceUiGaiaPlugin_SetSelectedAssistant(uint8 payload_length, const uint8 *payload)
{
    bool status = FALSE;

    if (payload_length == 1)
    {
        status = VoiceUi_SelectVoiceAssistant(payload[0]);
    }

    if (status)
    {
        voiceUiGaiaPlugin_SendResponse(voice_ui_gaia_set_selected_assistant, 0, NULL);
    }
    else
    {
        voiceUiGaiaPlugin_SendInvalidParameter(voice_ui_gaia_set_selected_assistant);
    }
}


static void voiceUiGaiaPlugin_GetSupportedAssistants(void)
{
    uint16 count;
    uint8 response[MAX_NO_VA_REPORTED + 1];

    count = VoiceUi_GetSupportedAssistants(response + 1);
    response[0] = count;
    voiceUiGaiaPlugin_SendResponse(voice_ui_gaia_get_supported_assistants, count + 1, response);
}


static void voiceUiGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("voiceUiGaiaPlugin_MainHandler pdu_id=%u", pdu_id);

    switch (pdu_id)
    {
    case voice_ui_gaia_get_selected_assistant:
        voiceUiGaiaPlugin_GetSelectedAssistant();
        break;

    case voice_ui_gaia_set_selected_assistant:
        voiceUiGaiaPlugin_SetSelectedAssistant(payload_length, payload);
        break;

    case voice_ui_gaia_get_supported_assistants:
        voiceUiGaiaPlugin_GetSupportedAssistants();
        break;

    default:
        voiceUiGaiaPlugin_SendNotSupported(pdu_id);
        break;
    }
}


static void voiceUiGaiaPlugin_SendAllNotifications(void)
{
    DEBUG_LOG("voiceUiGaiaPlugin_SendAllNotifications");
    VoiceUiGaiaPlugin_NotifyAssistantChanged(VoiceUi_GetSelectedAssistant());
}


void VoiceUiGaiaPlugin_Init(void)
{
    GaiaFramework_RegisterFeature(GAIA_VOICE_UI_FEATURE_ID, GAIA_VOICE_UI_VERSION, voiceUiGaiaPlugin_MainHandler, voiceUiGaiaPlugin_SendAllNotifications);
}


void VoiceUiGaiaPlugin_NotifyAssistantChanged(voice_ui_provider_t va_provider)
{
    uint8 provider_id = va_provider;
    GaiaFramework_SendNotification(GAIA_VOICE_UI_FEATURE_ID, voice_ui_gaia_assistant_changed, 1, &provider_id);
}
