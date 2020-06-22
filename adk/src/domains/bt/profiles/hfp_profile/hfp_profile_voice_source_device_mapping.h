/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#ifndef HFP_PROFILE_VOICE_SOURCE_DEVICE_MAPPING_H_
#define HFP_PROFILE_VOICE_SOURCE_DEVICE_MAPPING_H_

#include <hfp.h>
#include "voice_sources_list.h"

hfp_link_priority HfpProfile_VoiceSourceDeviceMappingGetIndexForSource(voice_source_t source);
voice_source_t HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_link_priority priority);

#endif /* HFP_PROFILE_VOICE_SOURCE_DEVICE_MAPPING_H_ */
