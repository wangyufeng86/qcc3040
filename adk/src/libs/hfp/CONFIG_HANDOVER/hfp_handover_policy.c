/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_handover_policy.c

DESCRIPTION
    This file defines stream handover policy configuration for
    platforms that support handover
    
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/

#include "hfp_handover_policy.h"
#include <source.h>

bool hfpSourceConfigureHandoverPolicy(Source src, uint32 value)
{    
    return SourceConfigure(src, STREAM_SOURCE_HANDOVER_POLICY, value);
}





