/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module that implements basic build block functions to handle Voice Assistant related actions
*/

#include "kymera_va_handlers.h"
#include "kymera_va_common.h"
#include "kymera_va_encode_chain.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_wuw_chain.h"
#include "kymera_private.h"

static void kymera_CreateMicChain(const va_audio_mic_config_t *mic_config, bool support_wuw, uint32 pre_roll_needed_in_ms)
{
    va_mic_chain_create_params_t params = {0};
    params.chain_params.clear_voice_capture = TRUE;
    params.chain_params.number_of_mics = MIN(Microphones_MaxSupported(), Kymera_GetVaMicChainMaxMicrophonesSupported());

#ifdef KYMERA_VA_USE_1MIC
    params.chain_params.number_of_mics = 1;
#endif

    params.operators_params.mic_sample_rate = mic_config->sample_rate;
    params.chain_params.wake_up_word_detection = support_wuw;
    params.operators_params.max_pre_roll_in_ms = pre_roll_needed_in_ms;

    Kymera_CreateVaMicChain(&params);
}

static void kymera_CreateEncodeChain(const va_audio_encode_config_t *encoder_config)
{
    va_encode_chain_create_params_t chain_params = {0};
    chain_params.chain_params.encoder = encoder_config->encoder;
    chain_params.operators_params.encoder_params = &encoder_config->encoder_params;
    Kymera_CreateVaEncodeChain(&chain_params);
}

static void kymera_CreateVaWuwChain(Task detection_handler, const va_audio_wuw_config_t *wuw_config)
{
    va_wuw_chain_create_params_t wuw_params = {0};
    wuw_params.chain_params.wuw_engine = wuw_config->engine;
    wuw_params.operators_params.wuw_model = wuw_config->model;
    wuw_params.operators_params.wuw_detection_handler = detection_handler;
    Kymera_CreateVaWuwChain(&wuw_params);
}

static void kymera_WuwDetectionChainSleep(void)
{
    Kymera_VaWuwChainSleep();
    Kymera_VaMicChainSleep();
}

static void kymera_WuwDetectionChainWake(void)
{
    Kymera_VaMicChainWake();
    Kymera_VaWuwChainWake();
}

void Kymera_CreateMicChainForLiveCapture(const void *params)
{
    const va_audio_voice_capture_params_t *capture = params;
    kymera_CreateMicChain(&capture->mic_config, FALSE, 0);
}

void Kymera_CreateMicChainForWuw(const void *params)
{
    const wuw_detection_start_t *wuw_detection = params;
    kymera_CreateMicChain(&wuw_detection->params->mic_config, TRUE, wuw_detection->params->max_pre_roll_in_ms);
}

void Kymera_StartMicChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaMicChain();
}

void Kymera_StopMicChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaMicChain();
}

void Kymera_DestroyMicChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaMicChain();
}

void Kymera_ActivateMicChainEncodeOutputForLiveCapture(const void *params)
{
    UNUSED(params);
    Kymera_ActivateVaMicChainEncodeOutput();
}

void Kymera_ActivateMicChainEncodeOutputForWuwCapture(const void *params)
{
    const va_audio_wuw_capture_params_t *capture = params;
    Kymera_ActivateVaMicChainEncodeOutputAfterTimestamp(capture->start_timestamp);
}

void Kymera_DeactivateMicChainEncodeOutput(const void *params)
{
    UNUSED(params);
    Kymera_DeactivateVaMicChainEncodeOutput();
}

void Kymera_BufferMicChainEncodeOutput(const void *params)
{
    UNUSED(params);
    Kymera_BufferVaMicChainEncodeOutput();
}

void Kymera_ActivateMicChainWuwOutput(const void *params)
{
    UNUSED(params);
    Kymera_ActivateVaMicChainWuwOutput();
}

void Kymera_DeactivateMicChainWuwOutput(const void *params)
{
    UNUSED(params);
    Kymera_DeactivateVaMicChainWuwOutput();
}

void Kymera_CreateEncodeChainForLiveCapture(const void *params)
{
    const va_audio_voice_capture_params_t *capture = params;
    kymera_CreateEncodeChain(&capture->encode_config);
}

void Kymera_CreateEncodeChainForWuwCapture(const void *params)
{
    const va_audio_wuw_capture_params_t *capture = params;
    kymera_CreateEncodeChain(&capture->encode_config);
}

void Kymera_StartEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaEncodeChain();
}

void Kymera_StopEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaEncodeChain();
}

void Kymera_DestroyEncodeChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaEncodeChain();
}

void Kymera_CreateWuwChain(const void *params)
{
    const wuw_detection_start_t *wuw_detection = params;
    kymera_CreateVaWuwChain(wuw_detection->handler, &wuw_detection->params->wuw_config);
}

void Kymera_StartWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_StartVaWuwChain();
}

void Kymera_StopWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_StopVaWuwChain();
}

void Kymera_DestroyWuwChain(const void *params)
{
    UNUSED(params);
    Kymera_DestroyVaWuwChain();
}

void Kymera_StartGraphManagerDelegation(const void *params)
{
    UNUSED(params);
    Kymera_VaWuwChainStartGraphManagerDelegation();
    kymera_WuwDetectionChainSleep();
}

void Kymera_StopGraphManagerDelegation(const void *params)
{
    UNUSED(params);
    kymera_WuwDetectionChainWake();
    Kymera_VaWuwChainStopGraphManagerDelegation();
}

void Kymera_UpdateAudioFrameworkConfig(const void *params)
{
    UNUSED(params);
    appKymeraConfigureDspPowerMode();
    OperatorsFrameworkSetKickPeriod(KICK_PERIOD_VOICE);
}
