/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_audio.c
\brief  Implementation of audio functionality for Amazon Voice Service
*/

#ifdef INCLUDE_AMA
#include "ama_audio.h"
#include "ama_data.h"
#include "ama_speech.h"
#include "ama_rfcomm.h"
#include "voice_audio_manager.h"
#include "voice_sources.h"
#include "ama_ble.h"

#include <source.h>
#include <stdlib.h>

#define AmaAudio_GetOpusFrameSize(config)  (config->u.opus_req_kbps == AMA_OPUS_16KBPS) ? 40 : 80;
#define AmaAudio_SetAudioFormat(config) (config->u.opus_req_kbps == AMA_OPUS_16KBPS) ?\
                                                    AmaSpeech_SetAudioFormat(ama_audio_format_opus_16khz_16kbps_cbr_0_20ms) :\
                                                    AmaSpeech_SetAudioFormat(ama_audio_format_opus_16khz_32kbps_cbr_0_20ms)

/* Function pointer to send the encoded voice data */
bool (*amaAudio_SendVoiceData)(Source src);

static bool amaAudio_SendMsbcVoiceData(Source source)
{

    #define AMA_HEADER_LEN 4
    #define MSBC_ENC_PKT_LEN 60
    #define MSBC_FRAME_LEN 57
    #define MSBC_FRAME_COUNT 5
    #define MSBC_BLE_FRAME_COUNT 1

    uint8 frames_to_send;
    uint16 payload_posn;
    uint16 lengthSourceThreshold;
    uint8 *buffer = NULL;
    uint8 no_of_transport_pkt = 0;
    uint8 initial_position = 0;

    bool sent_if_necessary = FALSE;

    if(AmaData_GetActiveTransport() == ama_transport_ble)
    {
        frames_to_send = MSBC_BLE_FRAME_COUNT;
        initial_position = AMA_HEADER_LEN - 1;
    }
    else
    {
        frames_to_send = MSBC_FRAME_COUNT;
        initial_position = AMA_HEADER_LEN;
    }

    lengthSourceThreshold = MSBC_ENC_PKT_LEN * frames_to_send;

    DEBUG_LOG("SourceSize In = %d", SourceSize(source));

    while ((SourceSize(source) >= (lengthSourceThreshold + 2)) && no_of_transport_pkt < 3)
    {
        const uint8 *source_ptr = SourceMap(source);
        uint32 copied = 0;
        uint32 frame;
        uint16 length;

        if(!buffer)
            buffer = PanicUnlessMalloc((MSBC_FRAME_LEN * frames_to_send) + AMA_HEADER_LEN);

        payload_posn = initial_position;
        
        for (frame = 0; frame < frames_to_send; frame++)
        {
            memmove(&buffer[payload_posn], &source_ptr[(frame * MSBC_ENC_PKT_LEN) + 2], MSBC_FRAME_LEN);
            payload_posn += MSBC_FRAME_LEN;
            copied += MSBC_FRAME_LEN;
        }

        length = AmaProtocol_PrepareVoicePacket(buffer, copied);

        sent_if_necessary = Ama_SendData(buffer, length);

        if(sent_if_necessary)
        {
            DEBUG_LOG("Sent length %d", copied);
            SourceDrop(source, lengthSourceThreshold);
        }
        else
        {
            DEBUG_LOG("Send Failed");
            break;
        }
        no_of_transport_pkt++;
    }

    free(buffer);

    DEBUG_LOG("SourceSize Remaining = %d", SourceSize(source));

    return sent_if_necessary;
}

static bool amaAudio_SendOpusVoiceData(Source source)
{
    /* Parameters used by Opus codec*/
    #define AMA_OPUS_HEADER_LEN         3
    #define OPUS_16KBPS_ENC_PKT_LEN     40
    #define OPUS_32KBPS_ENC_PKT_LEN     80
    #define OPUS_16KBPS_LE_FRAME_COUNT      4
    #define OPUS_16KBPS_RFCOMM_FRAME_COUNT  5
    #define OPUS_32KBPS_RFCOMM_FRAME_COUNT  3
    #define OPUS_32KBPS_LE_FRAME_COUNT      2

    uint16 payload_posn;
    uint16 lengthSourceThreshold;
    uint8 *buffer = NULL;
    bool sent_if_necessary = FALSE;
    uint8 no_of_transport_pkt = 0;
    ama_transport_t transport;
    uint16 opus_enc_pkt_len = OPUS_16KBPS_ENC_PKT_LEN; /* Make complier happy. */
    uint16 opus_frame_count = OPUS_16KBPS_RFCOMM_FRAME_COUNT;

    transport = AmaData_GetActiveTransport();
    DEBUG_LOG("amaSendOpusSourceSpeechData on transport %d", transport);

    switch(AmaSpeech_GetAudioFormat())
    {
        case AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS :

            if(transport == ama_transport_rfcomm)
            {
                DEBUG_LOG("16KBPS RFComm");
                opus_enc_pkt_len = OPUS_16KBPS_ENC_PKT_LEN;
                opus_frame_count = OPUS_16KBPS_RFCOMM_FRAME_COUNT;
            }
            else
            {
                DEBUG_LOG("16KBPS LE");
                opus_enc_pkt_len = OPUS_16KBPS_ENC_PKT_LEN;
                opus_frame_count = OPUS_16KBPS_LE_FRAME_COUNT;
            }
            break;

        case AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS :

            if(transport == ama_transport_rfcomm)
            {
                DEBUG_LOG("32KBPS RFComm");
                opus_enc_pkt_len = OPUS_32KBPS_ENC_PKT_LEN;
                opus_frame_count = OPUS_32KBPS_RFCOMM_FRAME_COUNT;
            }
            else
            {
                
                DEBUG_LOG("32KBPS LE");
                opus_enc_pkt_len = OPUS_32KBPS_ENC_PKT_LEN;
                opus_frame_count = OPUS_32KBPS_LE_FRAME_COUNT;
            }
            break;

        case AUDIO_FORMAT__PCM_L16_16KHZ_MONO :
        case AUDIO_FORMAT__MSBC:
            default :
                DEBUG_LOG("Not Expected Codec on ");
                Panic();
                break;
    }

    lengthSourceThreshold = (opus_frame_count * opus_enc_pkt_len);

    DEBUG_LOG("SourceSize In = %d, lengthSourceThreshold = %d", SourceSize(source), lengthSourceThreshold);

    while (SourceSize(source) && (SourceSize(source) >= lengthSourceThreshold) && (no_of_transport_pkt < 3))
    {
        const uint8 *opus_ptr = SourceMap(source);
        uint16 frame;
        uint16 copied = 0;
        uint16 length;
        DEBUG_LOG("Pkt Num %d", no_of_transport_pkt);

        if(!buffer)
            buffer = PanicUnlessMalloc((lengthSourceThreshold) + 3);

        payload_posn = AMA_OPUS_HEADER_LEN;

        for (frame = 0; frame < opus_frame_count; frame++)
        {
            memmove(&buffer[payload_posn], &opus_ptr[(frame*opus_enc_pkt_len)], opus_enc_pkt_len);
            payload_posn += opus_enc_pkt_len;
            copied += opus_enc_pkt_len;
        }

        length = AmaProtocol_PrepareVoicePacket(buffer, copied);

        sent_if_necessary = Ama_SendData(buffer, length);

        if(sent_if_necessary)
        {
            DEBUG_LOG("Sent length %d", copied);
            SourceDrop(source, lengthSourceThreshold);
        }
        else
        {
            DEBUG_LOG("Send Failed");
            break;
        }
        no_of_transport_pkt++;
    }

    if(buffer)
        free(buffer);

    DEBUG_LOG("SourceSize Remaining = %d", SourceSize(source));

    return sent_if_necessary;

}


/***************************************************************************/
static va_audio_codec_t amaAudio_ConvertCodecType(ama_codec_t codec_type)
{
    switch(codec_type)
    {
        case ama_codec_sbc:
            return va_audio_codec_sbc;
        case ama_codec_msbc:
            return va_audio_codec_msbc;
        case ama_codec_opus:
            return va_audio_codec_opus;
        default:
            Panic();
            return ama_codec_last;
    }
}

/***************************************************************************/
static unsigned amaAudio_HandleVoiceData(Source src)
{
    unsigned timeout = 0;

    if(AmaData_IsSendingVoiceData() && (amaAudio_SendVoiceData(src) == FALSE))
    {
        timeout = 30;
    }
    else
    {
        SourceDrop(src, SourceSize(src));
    }

    return timeout;
}

/***************************************************************************/
static void amaAudio_StartVoiceCapture(void)
{
    va_audio_voice_capture_params_t audio_cfg = {0};
    ama_audio_data_t *ama_audio_cfg;
    va_audio_codec_t codec;

    audio_cfg.mic_config.sample_rate = 16000;

    ama_audio_cfg = AmaData_GetAudioData();
    codec = amaAudio_ConvertCodecType(ama_audio_cfg->codec);
    audio_cfg.encode_config.encoder = codec;
    
    if(ama_audio_cfg->codec == ama_codec_msbc)
    {
        amaAudio_SendVoiceData = &amaAudio_SendMsbcVoiceData;
        audio_cfg.encode_config.encoder_params.msbc.bitpool_size = ama_audio_cfg->u.msbc_bitpool_size;
    }
    else if(ama_audio_cfg->codec == ama_codec_opus)
    {
        amaAudio_SendVoiceData = &amaAudio_SendOpusVoiceData;
        audio_cfg.encode_config.encoder_params.opus.frame_size = AmaAudio_GetOpusFrameSize(ama_audio_cfg);
    }

    VoiceAudioManager_StartCapture(amaAudio_HandleVoiceData, &audio_cfg);
}

/***************************************************************************/
static void amaAudio_StopVoiceCapture(void)
{
    VoiceAudioManager_StopCapture();
}

/***************************************************************************/
static inline bool amaAudio_IsReadyToCaptureVoice(void)
{
    return (AmaData_IsReadyToSendStartSpeech() &&
            !VoiceSources_IsVoiceRouted());
}

/***************************************************************************/
bool AmaAudio_Start(ama_audio_trigger_t type)
{
    bool return_val = FALSE;
     ama_audio_data_t *ama_audio_cfg;

    ama_audio_cfg = AmaData_GetAudioData();
    if(ama_audio_cfg->codec == ama_codec_msbc)
        AmaSpeech_SetAudioFormat(ama_audio_format_msbc);
    else if(ama_audio_cfg->codec == ama_codec_opus)
        AmaAudio_SetAudioFormat(ama_audio_cfg);
    
    if(amaAudio_IsReadyToCaptureVoice())
    {
        switch(type)
        {
            case ama_audio_trigger_tap:
                return_val = AmaSpeech_StartTapToTalk();
                break;
            case ama_audio_trigger_press:
                return_val = AmaSpeech_StartPushToTalk();
                break;
            case ama_audio_trigger_wake_word:
            default:
                break;
        }
    }

    if(return_val)
    {
        amaAudio_StartVoiceCapture();
    }
    
    return return_val;
}

/***************************************************************************/
void AmaAudio_Stop(void)
{
    DEBUG_LOG("AmaAudio_Stop");
    amaAudio_StopVoiceCapture();
}

/***************************************************************************/
bool AmaAudio_Provide(const AMA_SPEECH_PROVIDE_IND_T* ind)
{
    bool return_val = FALSE;
    if(amaAudio_IsReadyToCaptureVoice())
    {
        amaAudio_StartVoiceCapture();
        return_val = TRUE;
    }
    AmaProtocol_ProvideSpeechRsp(return_val, ind);
    return return_val;
}

/***************************************************************************/
void AmaAudio_End(void)
{
    DEBUG_LOG("AmaAudio_End");
    AmaSpeech_End();
    amaAudio_StopVoiceCapture();
}

/***************************************************************************/
void AmaAudio_Suspend(void)
{
    DEBUG_LOG("AmaAudio_Suspend");
    AmaSpeech_Stop();
    if(AmaData_IsSendingVoiceData())
        amaAudio_StopVoiceCapture();
}

/***************************************************************************/
void AmaAudio_Resume(void)
{
    DEBUG_LOG("AmaAudio_Resume");
}

#endif /* INCLUDE_AMA */

