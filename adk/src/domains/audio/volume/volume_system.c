/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the System Volume.
*/

#include "volume_limit.h"
#include "volume_mute.h"
#include "volume_system.h"
#include "volume_utils.h"

static volume_t system_volume = FULL_SCALE_VOLUME;

volume_t Volume_GetSystemVolume(void)
{
    return system_volume;
}

void Volume_SetSystemVolume(volume_t volume)
{
    system_volume = volume;
}

static int volume_ApplyLimiterToVolume(volume_t volume)
{
    int limited_value = volume.value;
    if(Volume_IsLimitSet())
    {
        limited_value = VolumeUtils_GetLimitedVolume(volume, Volume_GetLimit());
    }
    return limited_value;
}

volume_t Volume_CalculateOutputVolume(volume_t trim_volume)
{
    volume_t output_volume = trim_volume;
    if(Volume_GetMuteState())
    {
        output_volume.value = trim_volume.config.range.min;
    }
    else
    {
        output_volume.value = ((trim_volume.value * VolumeUtils_GetVolumeInPercent(Volume_GetSystemVolume())) / 100);
        output_volume.value = volume_ApplyLimiterToVolume(output_volume);
    }
    return output_volume;
}
