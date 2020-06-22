/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Mirror profile audio source control.
*/

#ifndef MIRROR_PROFILE_AUDIO_SOURCE_H_
#define MIRROR_PROFILE_AUDIO_SOURCE_H_

#include "audio_sources.h"
#include "audio_sources_audio_interface.h"
#include "source_param_types.h"

/*! \brief Gets the mirror A2DP audio interface.

    \return The audio source audio interface for a mirror A2DP source
 */
const audio_source_audio_interface_t *MirrorProfile_GetAudioInterface(void);


/*! \brief Read the connect parameters from the source and store them in the
        mirror profile a2dp state.

    \param source The audio source.

    \return TRUE if connect parameters were valid, else FALSE.
 */
bool MirrorProfile_StoreAudioSourceParameters(audio_source_t source);

/*! \brief Start audio for A2DP. */
void MirrorProfile_StartAudio(void);

/*! \brief Stop audio for A2DP. */
void MirrorProfile_StopAudio(void);

/*! \brief Start audio synchronisation with the other earbud. */
void MirrorProfile_StartAudioSynchronisation(void);

/*! \brief Stop audio synchronisation with the other earbud. */
void MirrorProfile_StopAudioSynchronisation(void);


#endif
