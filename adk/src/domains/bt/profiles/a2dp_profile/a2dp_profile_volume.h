/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   a2dp_profile_volume A2DP Profile Volume
\ingroup    a2dp_profile
\brief      The audio source volume interface implementation for A2DP sources
*/

#ifndef A2DP_PROFILE_VOLUME_H_
#define A2DP_PROFILE_VOLUME_H_

#include "audio_sources_volume_interface.h"

/*\{*/

/*! \brief Gets the A2DP volume interface.

    \return The audio source volume interface for an A2DP source
 */
const audio_source_volume_interface_t * A2dpProfile_GetAudioSourceVolumeInterface(void);

/*\}*/

#endif /* A2DP_PROFILE_VOLUME_H_ */
