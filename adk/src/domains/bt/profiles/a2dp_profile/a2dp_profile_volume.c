/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   a2dp_profile
\brief      The audio source volume interface implementation for A2DP sources
*/

#include "a2dp_profile_volume.h"

#include "a2dp_profile.h"
#include "av.h"
#include "audio_sources_list.h"
#include "volume_types.h"

#define A2DP_VOLUME_MIN      0
#define A2DP_VOLUME_MAX      127
#define A2DP_VOLUME_STEPS    128
#define A2DP_VOLUME_CONFIG   { .range = { .min = A2DP_VOLUME_MIN, .max = A2DP_VOLUME_MAX }, .number_of_steps = A2DP_VOLUME_STEPS }
#define A2DP_VOLUME(step)    { .config = A2DP_VOLUME_CONFIG, .value = step }

static volume_t a2dpProfile_GetVolume(audio_source_t source);
static void a2dpProfile_SetVolume(audio_source_t source, volume_t volume);

static const audio_source_volume_interface_t a2dp_volume_interface =
{
    .GetVolume = a2dpProfile_GetVolume,
    .SetVolume = a2dpProfile_SetVolume
};

static volume_t a2dpProfile_GetVolume(audio_source_t source)
{
    volume_t volume = A2DP_VOLUME(A2DP_VOLUME_MIN);
    if(source == audio_source_a2dp_1)
    {
        volume.value = AvGetTaskData()->volume;
    }
    return volume;
}

static void a2dpProfile_SetVolume(audio_source_t source, volume_t volume)
{
    if(source == audio_source_a2dp_1)
    {
        AvGetTaskData()->volume = volume.value;
        appAvConfigStore();
    }
}

const audio_source_volume_interface_t * A2dpProfile_GetAudioSourceVolumeInterface(void)
{
    return &a2dp_volume_interface;
}



