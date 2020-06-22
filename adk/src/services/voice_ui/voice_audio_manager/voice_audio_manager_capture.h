/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of functions to handle capture source
*/

#ifndef VOICE_AUDIO_MANAGER_CAPTURE_H_
#define VOICE_AUDIO_MANAGER_CAPTURE_H_

#include "voice_audio_manager.h"

/*! \brief Must be called when a start capture is requested
    \param callback Will be called whenever mic data becomes available
*/
void VoiceAudioManager_CaptureStarting(VaAudioCaptureDataReceived callback);

/*! \brief Must be called when a capture has started
    \param source The capture/mic source
*/
void VoiceAudioManager_CaptureStarted(Source source);

/*! \brief Must be called when a stop capture is requested
*/
void VoiceAudioManager_CaptureStopping(void);

/*! \brief Reset for test purposes
*/
void VoiceAudioManager_CaptureTestReset(void);

#endif /* VOICE_AUDIO_MANAGER_CAPTURE_H_ */
