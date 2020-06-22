/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of functions to handle Wake-Up-Word detection
*/

#ifndef VOICE_AUDIO_MANAGER_WUW_DETECTION_H_
#define VOICE_AUDIO_MANAGER_WUW_DETECTION_H_

#include "voice_audio_manager.h"
#include "kymera.h"

/*! \brief Set callback for when Wake-Up-Word is detected
    \param callback The aforementioned callback
*/
void VoiceAudioManager_SetWuwDetectedCallback(VaAudioWakeUpWordDetected callback);

/*! \brief Must be called when Wake-Up-Word is detected
    \param wuw_info All information related to the detection
    \return The response to the Wake-Up-Word being detected
*/
kymera_wuw_detected_response_t VoiceAudioManager_WuwDetected(const va_audio_wuw_detection_info_t *wuw_info);

/*! \brief Reset for test purposes
*/
void VoiceAudioManager_WuwTestReset(void);

#endif /* VOICE_AUDIO_MANAGER_WUW_DETECTION_H_ */
