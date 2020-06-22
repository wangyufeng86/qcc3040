/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The audio source observer interface implementation provided by Mirror Profile
*/

#ifndef MIRROR_PROFILE_VOLUME_OBSERVER_H_
#define MIRROR_PROFILE_VOLUME_OBSERVER_H_

#include "audio_sources_observer_interface.h"

/*! \brief Gets the Mirror Profile observer interface.

    \return The audio source observer interface provided by Mirror Profile
 */
const audio_source_observer_interface_t * MirrorProfile_GetObserverInterface(void);

#endif /* MIRROR_PROFILE_VOLUME_OBSERVER_H_ */
