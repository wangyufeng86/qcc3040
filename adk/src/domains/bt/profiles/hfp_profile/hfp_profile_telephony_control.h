/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile_telephony_control HFP Profile Telephony Control
\ingroup    hfp_profile
\brief      The voice source telephony control interface implementation for HFP sources
*/

#ifndef HFP_PROFILE_TELEPHONY_CONTROL_H_
#define HFP_PROFILE_TELEPHONY_CONTROL_H_

#include "voice_sources_telephony_control_interface.h"

/*! \brief Gets the HFP telephony control interface.

    \return The voice source telephony control interface for an HFP source
 */
const voice_source_telephony_control_interface_t * HfpProfile_GetTelephonyControlInterface(void);

#endif /* HFP_PROFILE_TELEPHONY_CONTROL_H_ */
