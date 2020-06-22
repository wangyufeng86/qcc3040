/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The voice source interface implementation for mirror profile
*/

#ifndef MIRROR_PROFILE_VOICE_SOURCE_H_
#define MIRROR_PROFILE_VOICE_SOURCE_H_

#include "voice_sources_audio_interface.h"

/*! \brief Gets the mirror profile voice interface.

    \return The voice source interface for mirror profile
 */
const voice_source_audio_interface_t * MirrorProfile_GetVoiceInterface(void);

/*! \brief Starts the SCO audio

    This is only expected to be called on the Secondary.
*/
void MirrorProfile_StartScoAudio(void);

/*! \brief Stops the SCO audio

    This is only expected to be called on the Secondary.
*/
void MirrorProfile_StopScoAudio(void);

#endif /* MIRROR_PROFILE_VOICE_SOURCE_H_ */
