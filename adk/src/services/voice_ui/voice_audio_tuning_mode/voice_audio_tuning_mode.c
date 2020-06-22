/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       voice_audio_tuning_mode.c
\defgroup   audio_tuning_mode
\brief      The file contains the implementation details of all the interface and helper functions to VA audio tuning functionality
*/

#ifdef ENABLE_AUDIO_TUNING_MODE
#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif
#include "ui.h"
#include "voice_audio_tuning_mode.h"
#include "voice_audio_manager.h"
#include <audio_sbc_encoder_params.h>
#include <voice_ui_container.h>
#include <source.h>
#include <kymera.h>

/* function declaration*/
static bool audio_tuning_mode_enabled = FALSE;
static void voiceAudioTuningMode_EventHandler(ui_input_t event_id);
static void voiceAudioTuningMode_AudioInputDataReceived(Source source);
static void voiceAudioTuningMode_Toggle(void);
static void voiceAudioTuningMode_DeselectVoiceAssistant(void){}
static void voiceAudioTuningMode_SelectVoiceAssistant(void){}

static const va_audio_encoder_sbc_params_t sbc_params =
{
    .bitpool_size = 24,
    .block_length = 16,
    .number_of_subbands = 8,
    .allocation_method = sbc_encoder_allocation_method_loudness,
};

/*voice assistant interface function mapping */
static voice_ui_if_t voice_audio_tuning_interface =
{
    voice_ui_provider_audio_tuning,
    voiceAudioTuningMode_EventHandler,
    NULL,
    voiceAudioTuningMode_DeselectVoiceAssistant,
    voiceAudioTuningMode_SelectVoiceAssistant
};

/*! \brief function to handle audio input data
    \param source.
    \return none.
*/
static void voiceAudioTuningMode_AudioInputDataReceived(Source source)
{
    uint16 size = SourceSize(source);
    if(size)
    {
        SourceDrop(source, size);
    }
}
/*! \brief  Event handler for audio tuning mode
    \param event_id.
    \return none.
*/
static void voiceAudioTuningMode_EventHandler(ui_input_t event_id)
{
    switch(event_id)
    {
        case ui_input_va_3:
            voiceAudioTuningMode_Toggle();
            break;
        default:
            break;
    }
}
/*! \brief  The function to toggle audio tuning mode from on to off and vice versa
    \param none
    \return none.
*/
void voiceAudioTuningMode_Toggle(void)
{
    if(!audio_tuning_mode_enabled)
    {
        va_audio_voice_capture_params_t audio_cfg =
        {
            .encode_config =
            {
                .encoder = va_audio_codec_sbc,
                .encoder_params.sbc = sbc_params,
            },
            .mic_config.sample_rate = 16000,
        };

        VoiceAudioManager_StartCapture(voiceAudioTuningMode_AudioInputDataReceived, &audio_cfg);
        audio_tuning_mode_enabled = TRUE ;
    }
    else
    {
        VoiceAudioManager_StopCapture();
        audio_tuning_mode_enabled = FALSE ;
    }
}
/*! \brief  The function to initialize the audio tuning mode
    \param Task
    \return  true if initializatioin sucessfull.
*/
bool VoiceAudioTuningMode_Init(Task init_task)
{
    UNUSED(init_task);
    voice_ui_protected_if_t *audio_tuning_protected_if = VoiceUi_Register(&voice_audio_tuning_interface);
    audio_tuning_protected_if->SetVaState(voice_ui_provider_audio_tuning, VOICE_UI_STATE_IDLE);
    return TRUE;
}

#endif /*ENABLE_AUDIO_TUNING_MODE */
