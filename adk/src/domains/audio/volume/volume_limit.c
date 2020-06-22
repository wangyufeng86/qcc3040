/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file

\brief      Provides a mechanism for limiting the output volume.
*/

#include "volume_limit.h"

#include "volume_types.h"
#include "volume_utils.h"

static volume_t limit = VOLUME_LIMIT_DISABLED;

void Volume_SetLimit(volume_t volume_limit)
{
    limit = volume_limit;
}

void Volume_RemoveLimit(void)
{
    volume_t disabled_limit = VOLUME_LIMIT_DISABLED;
    limit = disabled_limit;
}

volume_t Volume_GetLimit(void)
{
    return limit;
}

bool Volume_IsLimitSet(void)
{
    volume_t limit_disabled = VOLUME_LIMIT_DISABLED;
    
    return (VolumeUtils_VolumeIsEqual(Volume_GetLimit(), limit_disabled) ? FALSE : TRUE);
}
