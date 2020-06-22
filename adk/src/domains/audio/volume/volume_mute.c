/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#include "volume_mute.h"

static bool mute_state = FALSE;

bool Volume_GetMuteState(void)
{
    return mute_state;
}

void Volume_SetMuteState(bool state)
{
    mute_state = state;
}


