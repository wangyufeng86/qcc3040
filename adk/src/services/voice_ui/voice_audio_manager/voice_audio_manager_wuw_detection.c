/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of functions to handle Wake-Up-Word detection
*/

#include "voice_audio_manager_wuw_detection.h"
#include "voice_audio_manager_capture.h"
#include "voice_audio_manager_sm.h"
#include <panic.h>

static VaAudioWakeUpWordDetected wuw_detected = NULL;

void VoiceAudioManager_SetWuwDetectedCallback(VaAudioWakeUpWordDetected callback)
{
    wuw_detected = callback;
}

kymera_wuw_detected_response_t VoiceAudioManager_WuwDetected(const va_audio_wuw_detection_info_t *wuw_info)
{
    kymera_wuw_detected_response_t response = {.start_capture = FALSE};

    if (VoiceAudioManager_IsWuwDetectionExpected())
    {
        PanicFalse(wuw_detected != NULL);
        va_audio_wuw_detected_response_t clients_response = wuw_detected(wuw_info);

        if (clients_response.start_capture)
        {
            PanicFalse(VoiceAudioManager_UpdateState(wuw_capture_start));
            PanicFalse(clients_response.capture_callback != NULL);
            VoiceAudioManager_CaptureStarting(clients_response.capture_callback);

            response.start_capture = TRUE;
            response.capture_callback = VoiceAudioManager_CaptureStarted;
            response.capture_params = clients_response.capture_params;
        }
    }

    return response;
}

void VoiceAudioManager_WuwTestReset(void)
{
    wuw_detected = NULL;
}
