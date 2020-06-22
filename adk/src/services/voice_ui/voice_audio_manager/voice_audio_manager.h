/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Voice UI audio interface
*/

#ifndef _VOICE_AUDIO_MANAGER_
#define _VOICE_AUDIO_MANAGER_

#include "va_audio_types.h"

/*! \brief Function to call once data arrives at the source.
    \param capture_source The aforementioned source.
    \return Call this function again after this many milliseconds if no additional data arrives
            and the source is not empty. Ignored if 0.
*/
typedef unsigned (* VaAudioCaptureDataReceived)(Source capture_source);

/*! \brief Response to a Wake-Up-Word detected indication */
typedef struct
{
    bool start_capture;
    VaAudioCaptureDataReceived capture_callback;
    va_audio_wuw_capture_params_t capture_params;
} va_audio_wuw_detected_response_t;

/*! \brief Function to call once the Wake-Up-Word is detected.
    \param wuw_info Information regarding the aforementioned detection.
    \return Response on how to proceed after the detection.
*/
typedef va_audio_wuw_detected_response_t (* VaAudioWakeUpWordDetected)(const va_audio_wuw_detection_info_t *wuw_info);

/*@{*/

/*! \brief Start capturing and sending live mic data to the application
    \param callback Callback function to call once mic data becomes available
    \param audio_config Configuration related to capturing/encoding mic data
    \return TRUE on success else FALSE
*/
bool VoiceAudioManager_StartCapture(VaAudioCaptureDataReceived callback, const va_audio_voice_capture_params_t *capture_config);

/*! \brief Stop capturing and sending mic data to the application
    \return TRUE on success else FALSE
*/
bool VoiceAudioManager_StopCapture(void);

/*! \brief Start Wake-Up-Word detection
    \param callback Callback function to call once the Wake-Up-Word has been detected
    \param audio_config Configuration related to Wake-Up-Word detection
    \return TRUE on success else FALSE
*/
bool VoiceAudioManager_StartDetection(VaAudioWakeUpWordDetected callback, const va_audio_wuw_detection_params_t *wuw_config);

/*! \brief Stop Wake-Up-Word detection
    \return TRUE on success else FALSE
*/
bool VoiceAudioManager_StopDetection(void);

/*@}*/

#endif /* _VOICE_AUDIO_MANAGER_ */
