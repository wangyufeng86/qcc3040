/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Indicates if this device belongs to a pair of devices.

*/

#include "multidevice.h"

#include <panic.h>

typedef struct
{
    multidevice_type_t type;
    multidevice_side_t side;
} multidevice_ctx_t;

static multidevice_ctx_t ctx;

void Multidevice_SetType(multidevice_type_t type)
{
    ctx.type = type;
}

multidevice_type_t Multidevice_GetType(void)
{
    return ctx.type;
}

void Multidevice_SetSide(multidevice_side_t side)
{
    ctx.side = side;
}

multidevice_side_t Multidevice_GetSide(void)
{
    return ctx.side;
}

bool Multidevice_IsLeft(void)
{
    if(ctx.type == multidevice_type_pair)
    {
        if(ctx.side == multidevice_side_left)
        {
            return TRUE;
        }
        else if(ctx.side == multidevice_side_right)
        {
            return FALSE;
        }
    }

    Panic();
    return FALSE;
}
