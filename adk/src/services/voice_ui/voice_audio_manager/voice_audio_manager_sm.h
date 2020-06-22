/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice audio manager state machine
*/

#ifndef VOICE_AUDIO_MANAGER_SM_H_
#define VOICE_AUDIO_MANAGER_SM_H_

typedef enum
{
    idle,
    live_capturing,
    wuw_detecting,
    live_capturing_detect_pending,
    wuw_capturing_detect_pending,
    wuw_capturing,
} states_t;

typedef enum
{
    live_capture_start,
    wuw_capture_start,
    capture_stop,
    wuw_detect_start,
    wuw_detect_stop,
} events_t;

/*! \brief Update the voice audio manager state
    \param event The incoming event
    \return TRUE if the transition is valid, FALSE otherwise
*/
bool VoiceAudioManager_UpdateState(events_t event);

/*! \brief Check if mic data is expected
    \return TRUE if it is expected, FALSE otherwise
*/
bool VoiceAudioManager_IsMicDataExpected(void);

/*! \brief Check if Wake-Up-Word detection is expected
    \return TRUE if it is expected, FALSE otherwise
*/
bool VoiceAudioManager_IsWuwDetectionExpected(void);

/*! \brief Reset for test purposes
*/
void VoiceAudioManager_SmTestReset(void);

#endif /* VOICE_AUDIO_MANAGER_SM_H_ */
