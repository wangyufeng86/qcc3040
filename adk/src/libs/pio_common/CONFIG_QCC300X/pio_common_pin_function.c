/****************************************************************************
Copyright (c) 2017 Qualcomm Technologies International, Ltd.

FILE NAME
    CONFIG_QCC300X/pio_common_pin_function.c

DESCRIPTION
    Common PIO operations used by libraries and applications
    Enable pins for special functions

NOTES

*/

/****************************************************************************
    Header files
*/
#include <stdlib.h>
#include <pio.h>
#include <print.h>
#include "pio_common.h"

/****************************************************************************
    Hardware-specific PIOs
*/
#define PIO_PCM1_IN         (2)
#define PIO_PCM1_OUT        (3)
#define PIO_PCM1_SYNC       (4)
#define PIO_PCM1_CLK        (5)

#define PIO_PCM2_IN         (8)
#define PIO_PCM2_OUT        (6)
#define PIO_PCM2_SYNC       (7)
#define PIO_PCM2_CLK        (9)

/****************************************************************************
    Public functions
*/
bool PioCommonEnableFunctionPins(pio_common_pin_t pins)
{
    uint32 unmap_mask = 0;
    bool ok = TRUE;
    
/*  QCC300x devices use PCM blocks for S/PDIF  */
    if (pins & (pin_pcma_in | pin_spdifa_rx))
    {
        ok = ok && PioSetFunction(PIO_PCM1_IN, PCM_IN);
        unmap_mask |= (1UL << PIO_PCM1_IN);
    }
    
    if (pins & (pin_pcma_out | pin_spdifa_tx))
    {
        ok = ok && PioSetFunction(PIO_PCM1_OUT, PCM_OUT);
        unmap_mask |= (1UL << PIO_PCM1_OUT);
    }
    
    if (pins & pin_pcma_sync)
    {
        ok = ok && PioSetFunction(PIO_PCM1_SYNC, PCM_SYNC);
        unmap_mask |= (1UL << PIO_PCM1_SYNC);
    }
    
    if (pins & pin_pcma_clk)
    {
        ok = ok && PioSetFunction(PIO_PCM1_CLK, PCM_CLK);
        unmap_mask |= (1UL << PIO_PCM1_CLK);
    }
    
    if (pins & (pin_pcmb_in | pin_spdifb_rx))
    {
        ok = ok && PioSetFunction(PIO_PCM2_IN, PCM_IN);
        unmap_mask |= (1UL << PIO_PCM2_IN);
    }
    
    if (pins & (pin_pcmb_out | pin_spdifb_tx))
    {
        ok = ok && PioSetFunction(PIO_PCM2_OUT, PCM_OUT);
        unmap_mask |= (1UL << PIO_PCM2_OUT);
    }
    
    if (pins & pin_pcmb_sync)
    {
        ok = ok && PioSetFunction(PIO_PCM2_SYNC, PCM_SYNC);
        unmap_mask |= (1UL << PIO_PCM2_SYNC);
    }
    
    if (pins & pin_pcmb_clk)
    {
        ok = ok && PioSetFunction(PIO_PCM2_CLK, PCM_CLK);
        unmap_mask |= (1UL << PIO_PCM2_CLK);
    }
        
    if (unmap_mask)
    {
        ok = ok && (PioSetMapPins32(unmap_mask, 0) == 0);
    }
    
    PRINT(("PioCommonEnableFunctionPins: mask=0x%08lX ok=%u\n", unmap_mask, ok));
    return ok;
}
