/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile_volume HFP Profile Volume
\ingroup    hfp_profile
\brief      The voice source volume interface implementation for HFP sources
*/

#ifndef HFP_PROFILE_VOLUME_H_
#define HFP_PROFILE_VOLUME_H_

#include "voice_sources_volume_interface.h"

/*\{*/

/*! \brief Gets the HFP volume interface.

    \return The voice source volume interface for an HFP source
 */
const voice_source_volume_interface_t * HfpProfile_GetVoiceSourceVolumeInterface(void);

/*\}*/

#endif /* HFP_PROFILE_VOLUME_H_ */
