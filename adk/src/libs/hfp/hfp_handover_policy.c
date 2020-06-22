/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_handover_policy.c

DESCRIPTION
    This file defines stream handover configuration for
    platforms that do not support handover
    
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/

#include <vmtypes.h>

#include "hfp_handover_policy.h"

bool hfpSourceConfigureHandoverPolicy(Source src, uint32 value)
{    
    UNUSED(src);
    UNUSED(value);
    /* Handover not supported so policy cannot be set, return true */
    return TRUE;
}





