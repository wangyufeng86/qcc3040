/* Copyright (c) 2016-2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 * Implementation of LED hardware control.
 */

#include <csrtypes.h>
#include <pio.h>
#include <led.h>
#include "ledctrl/ledctrl.h"
#include "hydra_log/hydra_log.h"

#ifndef DESKTOP_TEST_BUILD

/******************************************************************************
 *
 * Handle LED configuration. Make sure it's connected to a PIO (which means the
 * Curator will have clocked the LED controller block), and then pass control
 * to the ledctrl_ module.
 */
bool LedConfigure(led_id led, led_config_key key, uint16 value)
{
    static uint16 auto_leds_mask = 0;
    uint16 pio;
    bool   pio_allocated = FALSE;
    bool   rc;

    /* Ensure we must be using an LED controller instance */
    COMPILE_TIME_ASSERT(NUM_OF_LED_PADS <= NUM_OF_LED_CTRLS, not_enough_led_pads);

    if (led >= NUM_OF_LED_CTRLS)
    {
        L2_DBG_MSG1("LedConfigure: Invalid LED %d", led);
        return FALSE;
    }

    /* Is the LED already connected to a PIO pin? */
    if (!ledctrl_led_has_pio(led))
    {
        if (led < NUM_OF_LED_PADS)
        {
            /* To maintain backwards compatability, try allocating the LED to
             * its default PIO. Note that PioSetMapPins32Bank returns 0 in the
             * success case.
             */
            pio = (uint16)(DEFAULT_LED0_PIO + led);
    
            if (!PioSetMapPins32Bank((uint16)(pio/32), 1<<(pio%32), 0))
            {
                if(PioSetFunction(pio, LED))
                {
                    pio_allocated = TRUE;
                    L3_DBG_MSG2("LedConfigure: Auto-allocate LED %d to PIO %d", led, pio);
                    auto_leds_mask |= (uint16)(1<<led);
                }
            }
        }
        if (!pio_allocated)
        {
            L2_DBG_MSG1("LedConfigure: Failed to auto-allocate LED %d to a PIO.", led);
            return FALSE;
        }
    }

    /* We now know the LED is attached to a pin (and so the LED controller is
     * running). This means we can safely set the controller config.
     */
    rc = ledctrl_config_led(led, key, value);

    /* If the config was ENABLE=0, then if it was auto-allocated, deallocate it */
    if ((key == LED_ENABLE) && !value)
    {
        if (auto_leds_mask & (1<<led))
        {
            /* Deallocate the autoallocated LED */
            pio = (uint16)(DEFAULT_LED0_PIO + led);
            (void)PioSetFunction(pio, OTHER);
            L3_DBG_MSG2("LedConfigure: Auto-deallocate LED %d from PIO %d", led, pio);
            auto_leds_mask &= (uint16)(~(1<<led));
        }
    }
    return rc;
}

#endif /* DESKTOP_TEST_BUILD */
