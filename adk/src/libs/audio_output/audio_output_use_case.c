/******************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.


FILE NAME
    audio_output_use_case.c

DESCRIPTION
    Implements use-case specific routines for audio output.
*/

#include "audio_output.h"
#include "audio_output_private.h"
#include <vmtypes.h>

/******************************************************************************/
void audioOutputCheckUseCase(bool active, audio_output_hardware_type_t type)
{
    /* Nothing needs to be done for the standard configuration */
    UNUSED(active);
    UNUSED(type);
}
