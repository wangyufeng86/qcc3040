/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      volume_t related utility functions.
*/

#include "volume_utils.h"

int VolumeUtils_LimitVolumeToRange(int volume, range_t range)
{
    int volume_result = volume;

    if(volume >= range.max)
    {
        volume_result = range.max;
    }
    if(volume <= range.min)
    {
        volume_result = range.min;
    }

    return volume_result;
}

int VolumeUtils_GetStepSize(volume_config_t config)
{
    unsigned steps = ((config.number_of_steps > 1) ? (config.number_of_steps-1) : 1);
    return ((config.range.max - config.range.min) / steps);
}

int16 VolumeUtils_ConvertToVolumeConfig(volume_t volume_to_convert, volume_config_t output_format)
{
    int16 converted_value = ((volume_to_convert.value - volume_to_convert.config.range.min) * (output_format.range.max - output_format.range.min))
                            / (volume_to_convert.config.range.max - volume_to_convert.config.range.min);
    int16 output_step_size = VolumeUtils_GetStepSize(output_format);
    
    if ((converted_value % output_step_size) != 0)
    {
        int16 output_step_level = (converted_value / output_step_size);
        if ((2 * (converted_value % output_step_size)) > output_step_size)
        {
            output_step_level++;
        }
        converted_value = output_step_level * output_step_size;
    }
    
    converted_value += output_format.range.min;
    
    return VolumeUtils_LimitVolumeToRange(converted_value, output_format.range);
}

bool VolumeUtils_VolumeIsEqual(volume_t volume_1, volume_t volume_2)
{
    return ((volume_1.value == volume_2.value) &&
            (volume_1.config.number_of_steps == volume_2.config.number_of_steps) &&
            (volume_1.config.range.min == volume_1.config.range.min) &&
            (volume_1.config.range.max == volume_1.config.range.max));
}

int VolumeUtils_IncrementVolume(volume_t volume)
{
    return VolumeUtils_LimitVolumeToRange((volume.value + VolumeUtils_GetStepSize(volume.config)), volume.config.range);
}

int VolumeUtils_DecrementVolume(volume_t volume)
{
    return VolumeUtils_LimitVolumeToRange((volume.value - VolumeUtils_GetStepSize(volume.config)), volume.config.range);
}

unsigned VolumeUtils_GetVolumeInPercent(volume_t volume)
{
    return (((volume.value - volume.config.range.min) * 100) / (volume.config.range.max - volume.config.range.min));
}

int VolumeUtils_GetLimitedVolume(volume_t volume, volume_t limit)
{
    int value = volume.value;
    int limited_value = VolumeUtils_ConvertToVolumeConfig(limit, volume.config);
    if(volume.value > limited_value)
    {
        value = limited_value;
    }
    return value;
}
