/******************************************************************************
Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.


FILE NAME
    audio_output_pcm.c

DESCRIPTION
    Implements hardware specific routines for audio output via I2S.
*/

#include <stdlib.h>
#include <panic.h>
#include <pio.h>

#include "audio_output.h"
#include "audio_output_private.h"

/* Hardware Specific Details */
#define PIO_PCM2_IN        (4)
#define PIO_PCM2_OUT       (5)
#define PIO_PCM2_SYNC      (6)
#define PIO_PCM2_CLK       (7)

/******************************************************************************/
bool audioOutputPcmInitHardware(const audio_output_config_t* config)
{
    audio_output_t audio_out;  /* Loop variable */

    /* Loop through and check each output for usage of 2nd I2S instance. */
    for(audio_out=0; audio_out<audio_output_max; audio_out++)
    {
        if (config->mapping[audio_out].endpoint.type == audio_output_type_i2s &&
            config->mapping[audio_out].endpoint.instance == audio_output_hardware_instance_1)
        {
            /* 2nd instance found, no need to continue searching. Return success
               based on whether all PIOs can be set to the correct function. */
            return (PioSetFunction(PIO_PCM2_IN, PCM_IN) &&
                    PioSetFunction(PIO_PCM2_OUT, PCM_OUT) &&
                    PioSetFunction(PIO_PCM2_SYNC, PCM_SYNC) &&
                    PioSetFunction(PIO_PCM2_CLK, PCM_CLK));
        }
    }

    /* No 2nd instance found, so no need to do anything. */
    return TRUE;
}
