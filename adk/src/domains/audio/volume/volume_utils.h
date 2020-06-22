/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   volume_utils Volume Utils
\ingroup    volume_group
\brief      volume_t related utility functions.
*/

#ifndef VOLUME_UTILS_H_
#define VOLUME_UTILS_H_

#include "volume_types.h"

/*\{*/

int16 VolumeUtils_ConvertToVolumeConfig(volume_t volume_to_convert, volume_config_t output_format);
bool VolumeUtils_VolumeIsEqual(volume_t volume_1, volume_t volume_2);

unsigned VolumeUtils_GetVolumeInPercent(volume_t volume);
int VolumeUtils_GetLimitedVolume(volume_t volume, volume_t limit);

int VolumeUtils_IncrementVolume(volume_t volume);
int VolumeUtils_DecrementVolume(volume_t volume);

int VolumeUtils_GetStepSize(volume_config_t config);
int VolumeUtils_LimitVolumeToRange(int volume, range_t range);

/*\}*/

#endif /* VOLUME_UTILS_H_ */
