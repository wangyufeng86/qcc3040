/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama.c
\brief  Implementation of the service interface for Amazon AVS
*/

#ifdef INCLUDE_AMA
#include "ama.h"
#include "ama_audio.h"
#include "ama_data.h"
#include "ama_config.h"
#include "ama_protocol.h"
#include "ama_send_command.h"
#include "ama_speech.h"
#include "ama_rfcomm.h"
#include "ama_actions.h"
#include "ama_ble.h"
#include "bt_device.h"
#include "local_name.h"
#include <ps.h>
#include <stdlib.h>
#include "gatt_server_gap.h"

#ifdef INCLUDE_ACCESSORY
#include "ama_accessory.h"
#endif

extern ama_config_t *ama_config;

static voice_ui_protected_if_t *voice_ui_protected_if = NULL;

/* AMA_TODO bring back our callback array! */
static ama_tx_callback_t ama_SendIap2Data;

/* Forward declaration */
static void ama_MessageHandler(Task task, MessageId id, Message message);
static void ama_Suspend(void);
static void ama_EventHandler(ui_input_t   event_id);

static const TaskData ama_task = {ama_MessageHandler};
static void ama_DeselectVoiceAssistant(void);
static void ama_SelectVoiceAssistant(void);
static void ama_ConfigureCodec(ama_codec_t codec);

/* Return the task associated with Voice Assistant */
#define Ama_GetTask() ((Task)&ama_task)

static voice_ui_if_t ama_interface =
{
    voice_ui_provider_ama,
    ama_EventHandler,
    ama_Suspend,
    ama_DeselectVoiceAssistant,
    ama_SelectVoiceAssistant,
};

static void ama_DeselectVoiceAssistant(void)
{
    DEBUG_LOG("ama_DeselectVoiceAssistant");
    AmaSendCommand_NotifyDeviceConfig(ASSISTANT_OVERRIDE_REQUIRED);
}

static void ama_SelectVoiceAssistant(void)
{
    DEBUG_LOG("ama_SelectVoiceAssistant");
    AmaSendCommand_NotifyDeviceConfig(ASSISTANT_OVERRIDEN);
}

/****************************************************************************/
voice_ui_protected_if_t *Ama_GetVoiceUiProtectedInterface(void)
{
    DEBUG_LOG("Ama_GetVoiceUiProtectedInterface");

    return voice_ui_protected_if;
}

/****************************************************************************/
static void ama_GetDeviceInfo(ama_device_config_t *device_info)
{
    device_info->serial_number = NULL;
    uint16 name_len = 0;
    device_info->name = (char*)LocalName_GetName(&name_len);
    device_info->device_type = AMA_CONFIG_DEVICE_TYPE;
}

/*****************************************************************************/
static void ama_HandleSwitchTransportInd(ama_transport_t transport)
{
    DEBUG_LOG("ama_HandleSwitchTransportInd transport=%u", transport);
    
    AmaData_SetActiveTransport(transport);

    switch(transport)
    {
        case ama_transport_ble:
            DEBUG_LOG("ama_transport_ble with OPUS codec");
            ama_ConfigureCodec(ama_codec_opus);
            break;

        case ama_transport_rfcomm:
            DEBUG_LOG("ama_transport_rfcomm with (%u) codec (1->mSBC, 2->OPUS)", (ama_codec_t)AMA_DEFAULT_CODEC_OVER_RFCOMM);
            ama_ConfigureCodec((ama_codec_t)AMA_DEFAULT_CODEC_OVER_RFCOMM);
            /* AMA_TODO  sinkAmaServerConnectionParameterUpdate(FALSE); */
            break;

        case ama_transport_iap:
            DEBUG_LOG("ama_transport_iap with (%u) codec (1->mSBC, 2->OPUS)", (ama_codec_t)AMA_DEFAULT_CODEC_OVER_IAP2);
            ama_ConfigureCodec((ama_codec_t)AMA_DEFAULT_CODEC_OVER_IAP2);
            break;

        default:
            DEBUG_LOG("UNKNOWN transport");
            break;
    }
}

/*****************************************************************************/
static void ama_HandleSendPacket(AMA_SEND_PKT_IND_T* ind)
{
    Ama_SendData(ind->packet, ind->pkt_size);
}


/************************************************************************/
static void ama_MessageHandler(Task task, MessageId id, Message message)
{

    UNUSED(task);

    switch(id)
    {
        case AMA_SWITCH_TRANSPORT_IND:
            ama_HandleSwitchTransportInd(((AMA_SWITCH_TRANSPORT_IND_T*)message)->transport);
            break;
            
        case AMA_SEND_TRANSPORT_VERSION_ID:
            {
                ama_HandleSendPacket((AMA_SEND_PKT_IND_T*)message);
                /* Now we are ready for accepting any AVS commands */
                AmaData_SetState(ama_state_idle);
            }
            break;

         case AMA_SPEECH_PROVIDE_IND:
            {
                if(AmaAudio_Provide(message))
                {
                    AmaData_SetState(ama_state_sending);
                }
            }
            break;

         case AMA_SPEECH_STOP_IND:
            {
                AmaAudio_Stop();
                AmaData_SetState(ama_state_idle);
            }
            break;

        case AMA_SEND_PKT_IND:
            {
                ama_HandleSendPacket((AMA_SEND_PKT_IND_T*)message);
            }
            break;
            
        case AMA_SYNCHRONIZE_SETTING_IND:
        case AMA_UPGRADE_TRANSPORT_IND:
        case AMA_ENABLE_CLASSIC_PAIRING_IND:
        case AMA_START_ADVERTISING_AMA_IND:
        case AMA_STOP_ADVERTISING_AMA_IND:
        case AMA_SEND_AT_COMMAND_IND:
        case AMA_SPEECH_STATE_IND:
            /* AMA_TODO: Yet to handle */
            break;

        default:
            DEBUG_LOG("ama_MessageHandler: unhandled 0x%04X", id);
            break;
    }

}

/************************************************************************/
static void ama_EventHandler(ui_input_t   event_id)
{
    DEBUG_LOG("ama_EventHandler, event_id %u", event_id);
    if (!AmaActions_HandleVaEvent(event_id))
        DEBUG_LOG("ama_EventHandler unhandled event %u", event_id);

}

/************************************************************************/
static void ama_Suspend(void)
{
    DEBUG_LOG("ama_Suspend()");
    AmaAudio_Suspend();
    AmaData_SetState(ama_state_idle);
}

/************************************************************************/
static void ama_ConfigureCodec(ama_codec_t codec)
{
    ama_audio_data_t audio_config;

    audio_config.codec = codec;

    if(audio_config.codec == ama_codec_opus)
    {
        audio_config.u.opus_req_kbps = AMA_DEFAULT_OPUS_CODEC_BIT_RATE;
    }
    else if(audio_config.codec == ama_codec_msbc)
    {
        audio_config.u.msbc_bitpool_size = MSBC_ENCODER_BITPOOL_VALUE;
    }

    AmaData_SetAudioData(&audio_config);
}

/************************************************************************/
bool Ama_Init(Task init_task)
{
    ama_config_t ama_info;
    bool status = TRUE;
    
    UNUSED(init_task);

    DEBUG_LOG("Ama_Init");

    appDeviceGetMyBdAddr(&ama_info.device_config.local_addr);
    ama_GetDeviceInfo(&ama_info.device_config);
    ama_info.num_transports_supported = 2;

    voice_ui_protected_if = VoiceUi_Register(&ama_interface);
    AmaProtocol_Init(Ama_GetTask(), &ama_info);
    AmaActions_Init();

#ifdef INCLUDE_AMA_LE
    ama_ConfigureCodec(ama_codec_opus);
    AmaData_SetActiveTransport(ama_transport_ble);
    AmaBle_RegisterAdvertising();
    GattServerGap_UseCompleteLocalName(TRUE);
#else
    ama_ConfigureCodec(ama_codec_msbc);
    AmaData_SetActiveTransport(ama_transport_rfcomm);
#endif
    
    AmaRfcomm_Init();
    AmaData_SetState(ama_state_initialized);
    AmaSpeech_SetToDefault();

#ifdef INCLUDE_ACCESSORY
    status = AmaAccessory_Init();
#endif

    return status;
}

/************************************************************************/
/* AMA_TODO: bring back our callback array! */
void Ama_SetTxCallback(ama_tx_callback_t callback, ama_transport_t transport)
{
    if (transport == ama_transport_iap)
    {
        ama_SendIap2Data = callback;
        ama_config->num_transports_supported = 3;
    }
}

/************************************************************************/
bool Ama_SendData(uint8 *data, uint16 size_data)
{
    bool status;
    
    switch (AmaData_GetActiveTransport())
    {
    case ama_transport_rfcomm:
        status = AmaRfcomm_SendData(data, size_data);
        break;
    
    case ama_transport_iap:
        status = ama_SendIap2Data(data, size_data);
        break;
    
    case ama_transport_ble:
         status = AmaBle_SendData(data, size_data);
         break;

    default:
        status = FALSE;
        break;
    }
    
    return status;
}

/************************************************************************/
bool Ama_ParseData(uint8 *data, uint16 size_data)
{
    return AmaParse_ParseData(data, size_data);
}

/************************************************************************/
void Ama_TransportSwitched(ama_transport_t transport)
{
    ama_HandleSwitchTransportInd(transport);
}

/************************************************************************/
void Ama_TransportDisconnected(void)
{
    DEBUG_LOG("AMA Transport Disconnect");

    Ama_TransportSwitched(ama_transport_ble);
    AmaProtocol_ResetParser();
    AmaData_SetState(ama_state_initialized);
    AmaSpeech_SetToDefault();
    //AMA_TODO: amaSendSinkMessage(AMA_START_ADVERTISING_AMA_IND, NULL);
}

/************************************************************************/
ama_transport_t Ama_GetActiveTransport(void)
{
    return AmaData_GetActiveTransport();
}

#endif /* INCLUDE_AMA */

