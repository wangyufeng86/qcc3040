/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile_volume_observer HFP Profile Volume Observer
\ingroup    hfp_profile
\brief      The voice source observer interface implementation for HFP sources
*/

#ifndef HFP_PROFILE_VOLUME_OBSERVER_H_
#define HFP_PROFILE_VOLUME_OBSERVER_H_

#include "voice_sources_observer_interface.h"

/*\{*/

/*! \brief Gets the HFP observer interface.

    \return The voice source observer interface for HFP sources
 */
const voice_source_observer_interface_t * HfpProfile_GetVoiceSourceObserverInterface(void);

/*\}*/

#endif /* HFP_PROFILE_VOLUME_OBSERVER_H_ */
