/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Voice UI audio interface
*/

#include "voice_audio_manager.h"
#include "voice_audio_manager_sm.h"
#include "voice_audio_manager_capture.h"
#include "voice_audio_manager_wuw_detection.h"
#include <kymera.h>
#include <logging.h>
#include <panic.h>

bool VoiceAudioManager_StartCapture(VaAudioCaptureDataReceived callback, const va_audio_voice_capture_params_t *capture_config)
{
    bool status = FALSE;
    DEBUG_LOG("VoiceAudioManager_StartCapture");

    /* If caller passes invalid parameters assume something is seriously wrong */
    if (!callback || !capture_config)
        Panic();

    if (VoiceAudioManager_UpdateState(live_capture_start))
    {
        VoiceAudioManager_CaptureStarting(callback);
        Kymera_StartVoiceCapture(VoiceAudioManager_CaptureStarted, capture_config);
        status = TRUE;
    }
    
    return status;
}

bool VoiceAudioManager_StopCapture(void)
{
    bool status = FALSE;
    DEBUG_LOG("VoiceAudioManager_StopCapture");

    if (VoiceAudioManager_UpdateState(capture_stop))
    {
        VoiceAudioManager_CaptureStopping();
        Kymera_StopVoiceCapture();
        status = TRUE;
    }
    
    return status;
}

bool VoiceAudioManager_StartDetection(VaAudioWakeUpWordDetected callback, const va_audio_wuw_detection_params_t *wuw_config)
{
    bool status = FALSE;
    DEBUG_LOG("VoiceAudioManager_StartDetection");

    /* If caller passes invalid parameters assume something is seriously wrong */
    if (!callback || !wuw_config)
        Panic();

    if (VoiceAudioManager_UpdateState(wuw_detect_start))
    {
        VoiceAudioManager_SetWuwDetectedCallback(callback);
        Kymera_StartWakeUpWordDetection(VoiceAudioManager_WuwDetected, wuw_config);
        status = TRUE;
    }

    return status;
}

bool VoiceAudioManager_StopDetection(void)
{
    bool status = FALSE;
    DEBUG_LOG("VoiceAudioManager_StopDetection");

    if (VoiceAudioManager_UpdateState(wuw_detect_stop))
    {
        Kymera_StopWakeUpWordDetection();
        status = TRUE;
    }

    return status;
}
