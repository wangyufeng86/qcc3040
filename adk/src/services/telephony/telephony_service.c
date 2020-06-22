/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the Telephony Service
*/

#include "telephony_service.h"

#include "kymera_adaptation.h"
#include "ui.h"
#include "ui_inputs.h"
#include "telephony_messages.h"
#include "voice_sources.h"
#include "voice_sources_list.h"

#include <panic.h>
#include <logging.h>

static void telephonyService_CallStateNotificationMessageHandler(Task task, MessageId id, Message message);
static void telephonyService_HandleUiInput(Task task, MessageId ui_input, Message message);

static TaskData telephony_message_handler_task = { telephonyService_CallStateNotificationMessageHandler };
static TaskData ui_handler_task = { telephonyService_HandleUiInput };

static const message_group_t ui_inputs[] =
{
    UI_INPUTS_TELEPHONY_MESSAGE_GROUP,
};

static voice_source_t telephonyService_GetVoiceSourceInFocus(void)
{
    return voice_source_hfp_1;
}

static void telephonyService_ConnectAudio(voice_source_t source)
{
    source_defined_params_t source_params;
    if(VoiceSources_GetConnectParameters(source, &source_params))
    {
        connect_parameters_t connect_params = { .source_type = source_type_voice, .source_params = source_params};
        KymeraAdaptation_Connect(&connect_params);
        VoiceSources_ReleaseConnectParameters(source, &source_params);
    }
}

static void telephonyService_DisconnectAudio(voice_source_t source)
{
    source_defined_params_t source_params;
    if(VoiceSources_GetDisconnectParameters(source, &source_params))
    {
        disconnect_parameters_t disconnect_params = { .source_type = source_type_voice, .source_params = source_params};
        KymeraAdaptation_Disconnect(&disconnect_params);
        VoiceSources_ReleaseDisconnectParameters(source, &source_params);
    }
}

static void telephonyService_CallStateNotificationMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch(id)
    {
        case TELEPHONY_AUDIO_CONNECTED:
            if(message && telephonyService_GetVoiceSourceInFocus() == ((TELEPHONY_AUDIO_CONNECTED_T *)message)->voice_source)
            {
                telephonyService_ConnectAudio(((TELEPHONY_AUDIO_CONNECTED_T *)message)->voice_source);
            }
            break;
        case TELEPHONY_AUDIO_DISCONNECTED:
            if(message && telephonyService_GetVoiceSourceInFocus() == ((TELEPHONY_AUDIO_DISCONNECTED_T *)message)->voice_source)
            {
                telephonyService_DisconnectAudio(((TELEPHONY_AUDIO_DISCONNECTED_T *)message)->voice_source);
            }
            break;
        default:
            DEBUG_LOG("telephonyService_CallStateNotificationMessageHandler: Unhandled event 0x%x", id);
            break;
    }
}

static void telephonyService_HandleUiInput(Task task, MessageId ui_input, Message message)
{
    voice_source_t source = telephonyService_GetVoiceSourceInFocus();
    UNUSED(task);
    UNUSED(message);
    switch(ui_input)
    {
        case ui_input_voice_call_hang_up:
            VoiceSources_TerminateOngoingCall(source);
            break;

        case ui_input_voice_call_accept:
            VoiceSources_AcceptIncomingCall(source);
            break;

        case ui_input_voice_call_reject:
            VoiceSources_RejectIncomingCall(source);
            break;

        case ui_input_hfp_transfer_to_ag:
            VoiceSources_TransferOngoingCallAudioToAg(source);
            break;

        case ui_input_hfp_transfer_to_headset:
            VoiceSources_TransferOngoingCallAudioToSelf(source);
            break;

        case ui_input_hfp_voice_dial:
            VoiceSources_InitiateVoiceDial(source);
            break;

        default:
            break;
    }
}

bool TelephonyService_Init(Task init_task)
{
    UNUSED(init_task);
    Telephony_RegisterForMessages(&telephony_message_handler_task);
    Ui_RegisterUiInputConsumer(&ui_handler_task, (uint16*)ui_inputs, sizeof(ui_inputs)/sizeof(uint16));
    return TRUE;
}



