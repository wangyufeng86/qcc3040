/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   avrcp_profile_volume_observer AVRCP Profile Volume Observer
\ingroup    avrcp_profile
\brief      The audio source observer interface implementation provided by AVRCP

            This is usually the observer interface for A2DP sources
*/

#ifndef AVRCP_PROFILE_VOLUME_OBSERVER_H_
#define AVRCP_PROFILE_VOLUME_OBSERVER_H_

#include "audio_sources_observer_interface.h"

/*\{*/

/*! \brief Gets the AVRCP observer interface.

    \return The audio source observer interface provided by AVRCP
 */
const audio_source_observer_interface_t * AvrcpProfile_GetObserverInterface(void);

/*\}*/

#endif /* AVRCP_PROFILE_VOLUME_OBSERVER_H_ */
