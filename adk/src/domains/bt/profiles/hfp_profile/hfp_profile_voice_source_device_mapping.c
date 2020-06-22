/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#include "hfp_profile_voice_source_device_mapping.h"

hfp_link_priority HfpProfile_VoiceSourceDeviceMappingGetIndexForSource(voice_source_t source)
{
    hfp_link_priority index = hfp_invalid_link;
    switch(source)
    {
        case voice_source_hfp_1:
            index = hfp_primary_link;
            break;
        default:
            index = hfp_invalid_link;
            break;
    }
    return index;
}

voice_source_t HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(hfp_link_priority priority)
{
    voice_source_t source = voice_source_none;
    switch(priority)
    {
        case hfp_primary_link:
            source = voice_source_hfp_1;
            break;
        default:
            source = voice_source_none;
            break;
    }
    return source;
}



