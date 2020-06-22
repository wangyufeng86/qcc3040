/******************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.


FILE NAME
    CONFIG_QCC300X/audio_output_pcm.c

DESCRIPTION
    Implements hardware-specific routines for audio output via PCM.
*/

#include <stdlib.h>
#include <panic.h>
#include <pio_common.h>
#include <print.h>

#include "audio_output.h"
#include "audio_output_private.h"

/******************************************************************************/
bool audioOutputPcmInitHardware(const audio_output_config_t* config)
{
    bool ok = TRUE;
    audio_output_t audio_out;  /* Loop variable */

    /* Loop through and check each output for usage of PCM hardware. */
    for(audio_out=0; audio_out<audio_output_max; audio_out++)
    {
        audio_output_hardware_type_t type = config->mapping[audio_out].endpoint.type;
        audio_output_hardware_instance_t instance = config->mapping[audio_out].endpoint.instance;
        
        if (type == audio_output_type_i2s)
        {
            if (instance == audio_output_hardware_instance_0)
            {
            /*  Initialise first PCM block for I2S output  */
                ok = PioCommonEnableFunctionPins(pin_pcma_out | pin_pcma_sync | pin_pcma_clk);
            }
            else if (instance == audio_output_hardware_instance_1)
            {
            /*  Initialise second PCM block for I2S output  */
                ok = PioCommonEnableFunctionPins(pin_pcmb_out | pin_pcmb_sync | pin_pcmb_clk);
            }
        }
        else if (type == audio_output_type_spdif)
        {
            if (instance == audio_output_hardware_instance_0)
            {
            /*  Initialise first PCM block for S/PDIF output  */
                ok = PioCommonEnableFunctionPins(pin_spdifa_tx);
            }
            else if (instance == audio_output_hardware_instance_1)
            {
            /*  Initialise second PCM block for S/PDIF output  */
                ok = PioCommonEnableFunctionPins(pin_spdifb_tx);
            }
        }
        
        PRINT(("audioOutputPcmInitHardware: out=%d type=%d inst=%d ok=%u\n", audio_out, type, instance, ok));

        if (!ok)
        {
        /*  Something went wrong, we can stop searching  */
            break;
        }
    }

    return ok;
}
