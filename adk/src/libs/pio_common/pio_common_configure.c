/****************************************************************************
Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.

FILE NAME
    pio_common_pin_function.c

DESCRIPTION
    Common PIO operations used by libraries and applications
    Enable pins for special functions

NOTES

*/

/****************************************************************************
    Header files
*/
#include <vmtypes.h>
#include <pio.h>
#include "pio_common.h"
#include "pio_common_private.h"

/****************************************************************************
    Public functions
*/

void PioCommonConfigureSpdifInput(uint8 spdif_input_pin)
{
    PioSetFunction(spdif_input_pin, SPDIF_RX);
}

void PioCommonConfigureSpdifOutput(uint8 spdif_output_pin)
{
    PioSetFunction(spdif_output_pin, SPDIF_TX);
}

void PioCommonSetStrongBias(pio_common_allbits mask, pio_common_allbits bits)
{
    UNUSED(mask);   
    UNUSED(bits);   
}

bool pioCommonSetConvertedFunction(uint16 pin, pio_common_pin_function_id function)
{
    switch (function)
    {
        /* PIO functions provided through pio.h. */
        case pin_function_uart_rx:
            return PioSetFunction(pin, UART_RX);
        case pin_function_uart_tx:
            return PioSetFunction(pin, UART_TX);
        case pin_function_uart_rts:
            return PioSetFunction(pin, UART_RTS);
        case pin_function_uart_cts:
            return PioSetFunction(pin, UART_CTS);
        case pin_function_pcm_in:
            return PioSetFunction(pin, PCM_IN);
        case pin_function_pcm_out:
            return PioSetFunction(pin, PCM_OUT);
        case pin_function_pcm_sync:
            return PioSetFunction(pin, PCM_SYNC);
        case pin_function_pcm_clk:
            return PioSetFunction(pin, PCM_CLK);
        case pin_function_sqif:
            return PioSetFunction(pin, SQIF);
        case pin_function_led:
            return PioSetFunction(pin, LED);
        case pin_function_lcd_segment:
            return PioSetFunction(pin, LCD_SEGMENT);
        case pin_function_lcd_common:
            return PioSetFunction(pin, LCD_COMMON);
        case pin_function_pio:
            return PioSetFunction(pin, PIO);
        case pin_function_spdif_rx:
            return PioSetFunction(pin, SPDIF_RX);
        case pin_function_spdif_tx:
            return PioSetFunction(pin, SPDIF_TX);
        default:
            return FALSE;
    }
}
