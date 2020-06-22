/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile
\brief      The voice source volume interface implementation for HFP sources
*/

#include "hfp_profile_volume.h"

#include "hfp_profile.h"
#include "voice_sources_list.h"
#include "volume_types.h"

#define HFP_VOLUME_MIN      0
#define HFP_VOLUME_MAX      15
#define HFP_VOLUME_CONFIG   { .range = { .min = HFP_VOLUME_MIN, .max = HFP_VOLUME_MAX }, .number_of_steps = ((HFP_VOLUME_MAX - HFP_VOLUME_MIN) + 1) }
#define HFP_VOLUME(step)    { .config = HFP_VOLUME_CONFIG, .value = step }

static volume_t hfpProfile_GetVolume(voice_source_t source);
static void hfpProfile_SetVolume(voice_source_t source, volume_t volume);

static const voice_source_volume_interface_t hfp_volume_interface =
{
    .GetVolume = hfpProfile_GetVolume,
    .SetVolume = hfpProfile_SetVolume
};

static volume_t hfpProfile_GetVolume(voice_source_t source)
{
    volume_t hfp_volume = HFP_VOLUME(HFP_VOLUME_MIN);
    if(source == voice_source_hfp_1)
    {
        hfp_volume.value = appGetHfp()->volume;
    }
    return hfp_volume;
}

static void hfpProfile_SetVolume(voice_source_t source, volume_t volume)
{
    if(source == voice_source_hfp_1)
    {
        appGetHfp()->volume = volume.value;
        appHfpConfigStore();
    }
}

const voice_source_volume_interface_t * HfpProfile_GetVoiceSourceVolumeInterface(void)
{
    return &hfp_volume_interface;
}


