/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The voice source audio interface implementation for hfp voice sources
*/

#ifndef HFP_PROFILE_AUDIO_H_
#define HFP_PROFILE_AUDIO_H_

#include "voice_sources_audio_interface.h"

#include <hfp.h>

/*! \brief Gets the HFP audio interface.

    \return The voice source audio interface for an HFP source
 */
const voice_source_audio_interface_t * HfpProfile_GetAudioInterface(void);

void HfpProfile_StoreConnectParams(const HFP_AUDIO_CONNECT_CFM_T *cfm);

#endif /* HFP_PROFILE_AUDIO_H_ */
