/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "ledctrl/ledctrl_private.h"

static void pwm_update(uint16 period_mul, uint16 bright_duty, uint16 dim_duty);
static void sync_group(uint16 group);
static void bankset_preload(led_id led);
static void bankset_postload(led_id led);
static void bankset_low_brightness(uint16 duty_cycle, uint16 period_mul);
static void bankset_high_brightness(uint16 duty_cycle, uint16 period_mul);
#ifdef DEBUG_LEDCTRL_INTERNAL
void ledctrl_dumpregs(void);
#endif

/* Array mapping logical LED id onto controller instance. */
static const uint8 led_id_to_ctrl[NUM_OF_LED_CTRLS] = {0,1,2,3,4,5};

/* Helper macros for status querying */
#define is_flashing(x) (ledcfg_flash_mask & (1<<(x)))
#define is_steady(x) (!is_flashing(x))
#define is_inverted(x) (ledcfg_invert_mask & (1<<(x)))

/*
 * Global storage
 */
static uint32 ledctrl_pio_cfg_mask[(NUMBER_OF_PIOS+PIOS_PER_BANK-1)/PIOS_PER_BANK];
static led_config led_cfg[NUM_OF_LED_CTRLS];
static uint16 ledcfg_flash_mask;
static uint16 ledcfg_invert_mask;


/******************************************************************************
 *
 * Initialise internal data structures and the LED controller hardware
 *
 * Default state for an LED is steady state max brightness.
 *
 * The LED controller MUST be clocked before this is called, by setting at
 * least one pio to 'LED' config (the system manager turns the controller on).
 */
void ledctrl_init(void)
{
#ifndef DESKTOP_TEST_BUILD
    uint8   i;

    /* Enable the clock and all state machines */
    hal_set_reg_led_ctrl_clk_enable(1);
    hal_set_reg_led_en(0);

    /* Toggle the soft reset - the docs fail to say that it needs this! */
    hal_set_reg_led_ctrl_soft_reset(0);
    hal_set_reg_led_ctrl_soft_reset(1);
    hal_set_reg_led_ctrl_soft_reset(0);

    /* Set the default hardware config in the banked regs */
    hal_set_reg_led_pin_config(LED_PIN_CONFIG_PUSH_PULL);
    bankset_low_brightness(0, LEDCTRL_MAX_PERIOD+1);
    bankset_high_brightness(LEDCTRL_MAX_DUTY_CYCLE, LEDCTRL_MAX_PERIOD+1);
    hal_set_reg_led_hold_low_config(0);
    hal_set_reg_led_hold_high_config(0);
    hal_set_reg_led_ramp_shift(0);
    hal_set_reg_led_ramp_config(1);
    hal_set_reg_led_start_up_state(LED_START_UP_STATE_COUNT_LOW_HOLD_MIN);
    hal_set_reg_led_counthold_value(0);
    hal_set_reg_led_logarithmic_en(0);
    hal_set_reg_led_logarithmic_offset_high(0);
    hal_set_reg_led_logarithmic_offset_low(0);

    /* Push this config into all the controllers */
    memset(led_cfg, 0, sizeof(led_cfg));
    for (i=0; i < NUM_OF_LED_CTRLS; i++)
    {
        hal_set_reg_led_index(i);
        hal_set_reg_led_configure(1);
        hal_set_reg_led_update(1<<i);
        led_cfg[i].init_state = LED_START_UP_STATE_COUNT_HIGH_RAMP_DOWN;
    }

    ledcfg_flash_mask = 0;
    ledcfg_invert_mask = 0;

    /* Let the state machines run */
    hal_set_reg_led_ctrl_clk_enable(1);
#endif /* DESKTOP_TEST_BUILD */
}


/******************************************************************************
 *
 * Configure a parameter of the LED controller, and update the LED state
 *
 * The LED controller MUST be clocked before this is called, by setting at
 * least one pio to 'LED' config (the system manager turns the controller on).
 */
bool ledctrl_config_led(led_id led, led_config_key key, uint16 value)
{
    static uint8 hw_initialised = 0;
    bool rc = TRUE;
    uint16 led_en;
    uint16 tmp;

    if (led >= NUM_OF_LED_CTRLS)
    {
        return FALSE;
    }

    if (!hw_initialised)
    {
        ledctrl_init();
        hw_initialised=1;
    }

    /* Convert led from abstract ID to controller ID */
    led = (led_id)led_id_to_ctrl[led];

    /* Get the enabled state and disable 'our' LED */
    led_en = (uint16)hal_get_reg_led_en();
    hal_set_reg_led_en(led_en & ~(1<<led));

    bankset_preload(led); /* Initialise the bank with current values */

    switch (key)
    {
        case LED_ENABLE:
            if (value)
            {
                led_en |= (uint16)((1<<led));
            }
            else
            {
                led_en &= (uint16)(~(1<<led));
            }
            break;

        case LED_DUTY_CYCLE:
            /* Update bright duty cycle */
            if (value > LEDCTRL_MAX_DUTY_CYCLE) value = LEDCTRL_MAX_DUTY_CYCLE;
            pwm_update(LEDCTRL_INVALID_PERIOD, value, LEDCTRL_INVALID_DUTY_CYCLE);
            break;

        case LED_PERIOD:
            /* Set the overall period for PWM. Affects the duty cycle */
            if (value > LEDCTRL_MAX_PERIOD) value = LEDCTRL_MAX_PERIOD;
            pwm_update((uint16)(value+1), LEDCTRL_INVALID_DUTY_CYCLE, LEDCTRL_INVALID_DUTY_CYCLE);
            break;

        case LED_FLASH_ENABLE:
            /* Set whether the LED is flashing or steady state */
            if (value) value=1;
            ledcfg_flash_mask = (uint16)((ledcfg_flash_mask & ~(1<<led)) | (value << led));
            break;

        case LED_FLASH_RATE:
            /* Set the ramp up/down time - Ramp config = 2^(rate+1)-1 */
            if (value > LEDCTRL_MAX_FLASH_RATE) value = LEDCTRL_MAX_FLASH_RATE;
            hal_set_reg_led_ramp_config((1<<(value+1))-1);
            break;

        case LED_FLASH_MAX_HOLD:
            /* Set the 'on' time */
            if (is_inverted(led))
            {
                hal_set_reg_led_hold_high_config(value);
            }
            else
            {
                hal_set_reg_led_hold_low_config(value);
            }
            break;

        case LED_FLASH_MIN_HOLD:
            /* Set the 'off' time */
            if (is_inverted(led))
            {
                hal_set_reg_led_hold_low_config(value);
            }
            else
            {
                hal_set_reg_led_hold_high_config(value);
            }
            break;

        case LED_FLASH_SEL_INVERT:
            if (value) value = 1;
            if ( is_inverted(led) != (value<<led) )
            {
                /* Swap the on/off hold times */
                tmp = (uint16)hal_get_reg_led_hold_low_config();
                hal_set_reg_led_hold_low_config(hal_get_reg_led_hold_high_config());
                hal_set_reg_led_hold_high_config(tmp);

                ledcfg_invert_mask ^= (uint16)(1<<led);
            }
            break;

        case LED_FLASH_SEL_DRIVE:
        case LED_FLASH_SEL_TRISTATE:
            L2_DBG_MSG2("LedCtrl: LED %d. Config key %d not supported.", led, key);
            rc = FALSE;
            break;

        case LED_FLASH_LOW_DUTY_CYCLE:
            /* Update dim duty cycle */
            if (value > LEDCTRL_MAX_DUTY_CYCLE) value = LEDCTRL_MAX_DUTY_CYCLE;
            pwm_update(LEDCTRL_INVALID_PERIOD, LEDCTRL_INVALID_DUTY_CYCLE, value);
            break;

        case LED_LOGARITHMIC_MODE_ENABLE:
            hal_set_reg_led_logarithmic_en(value?1:0);
            break;

        case LED_LOGARITHMIC_MODE_OFFSET_HIGH:
            if (value > LEDCTRL_MAX_LOG_OFFSET)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. LOGARITHMIC_MODE_OFFSET_HIGH - Value out of range (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                hal_set_reg_led_logarithmic_offset_high(value);
            }
            break;

        case LED_LOGARITHMIC_MODE_OFFSET_LOW:
            if (value > LEDCTRL_MAX_LOG_OFFSET)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. LOGARITHMIC_MODE_OFFSET_LOW - Value out of range (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                hal_set_reg_led_logarithmic_offset_low(value);
            }
            break;

        case LED_ADD_TO_GROUP:
            if (value > LEDCTRL_MAX_GROUPS)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. ADD_TO_GROUP - Invalid group (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                led_cfg[led].groups |= 1<<value;
            } 
            break;

        case LED_REMOVE_FROM_GROUP:
            if (value > LEDCTRL_MAX_GROUPS)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. REMOVE_FROM_GROUP - Invalid group (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                led_cfg[led].groups &= ~(1<<value);
            }
            break;

        case LED_SYNCHRONISE_GROUP:
            if (value > LEDCTRL_MAX_GROUPS)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. SYNCHRONISE_GROUP - Invalid group (%d).", led, value);
                rc = FALSE;
            }
            else if (!(led_cfg[led].groups & (1<<value)))
            {
                L2_DBG_MSG2("LedCtrl: LED %d. SYNCHRONISE_GROUP - Not in group %d.", led, value);
                rc = FALSE;
            }
            else
            {
                sync_group(value);
            }
            break;

        case LED_INITIAL_STATE:
            if (value > LEDCTRL_MAX_INITIAL_STATE)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. LED_INITIAL_STATE - Invalid initial state (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                led_cfg[led].init_state = value;
            }
            break;

        case LED_COUNTHOLD:
            led_cfg[led].init_count = value;
            break;

        case LED_PIN_DRIVE_CONFIG:
            if (value > LEDCTRL_MAX_PIN_DRIVE_CONFIG)
            {
                L2_DBG_MSG2("LedCtrl: LED %d. LED_PIN_DRIVE_CONFIG - Invalid value (%d).", led, value);
                rc = FALSE;
            }
            else
            {
                hal_set_reg_led_pin_config(value);
            }
            break;

        default:
            L2_DBG_MSG2("LedCtrl: LED %d. Config key %d invalid.", led, key);
            rc = FALSE;
    }

    if (rc)
    {
        /* Update the config registers that we couldn't do at first */
        bankset_postload(led);

        /* Latch the new configuration into the controller */
        hal_set_reg_led_update(0);
        hal_set_reg_led_configure(1);
        hal_set_reg_led_update(1<<led); /* Write sensitive, despite reg docs... */

        hal_set_reg_led_single_shot_mode(~ledcfg_flash_mask & ~(1<<led));
        if (is_steady(led))
        {
            /* If the LED is not flashing, then we need to re-enter single-shot mode
             * to latch the config (again).
             */
            hal_set_reg_led_single_shot_mode(~ledcfg_flash_mask);
        }
    }

    /* Finally, re-enable the configured LED */
    hal_set_reg_led_en(led_en);

    return rc;
}


/******************************************************************************
 * Determine whether the given logical LED is assigned to a pin.
 * Called by the trap api handler.
 */
bool ledctrl_led_has_pio(led_id led)
{
    uint16 pin, bank, pin_offset;

    /* First, check the led is a valid ledid */
    if (led >= NUM_OF_LED_CTRLS)
    {
        return FALSE;
    }

    pin_offset = (DEFAULT_LED0_PIO + led) % NUM_OF_LED_CTRLS;

    /* Search the pin table for that controller being allocated a pin */
    for (pin = pin_offset; pin < NUMBER_OF_PIOS; pin += NUM_OF_LED_CTRLS)
    {
        bank = (uint16)(pin / 32);
        if (ledctrl_pio_cfg_mask[bank] & (1<<(pin%32)))
        {
            return TRUE;
        }
    }
    return FALSE;
}


/******************************************************************************
 * Called by PIO code to tell us which pins are LEDs 
 */
void ledctrl_set_pio_mask(uint16 bank, uint32 mask, bool pin_is_led)
{
    if (pin_is_led)
    {
        ledctrl_pio_cfg_mask[bank] |= mask; 
    }
    else
    {
        ledctrl_pio_cfg_mask[bank] &= ~mask;
    }
}


/******************************************************************************
 * Given any of period multiplier/min_duty/max_duty, recalculate the actual
 * register values for on/off timings
 *
 * The 'on' time is in the low_config, and the 'off' time is implicitly the
 * rest of the period.
 */
static void pwm_update(uint16 period_mul, uint16 bright_duty, uint16 dim_duty)
{
    uint16 max_lo, min_lo, oldperiod;

    /* Extract the current values from the register block - this assumes that
     * bankset_preload has previously been called.
     */
    max_lo = (uint16)hal_get_reg_led_max_low_config();
    min_lo = (uint16)hal_get_reg_led_min_low_config();
    oldperiod = (uint16)((hal_get_reg_led_max_high_config()+max_lo) / LEDCTRL_MAX_DUTY_CYCLE);

    L4_DBG_MSG3("ledctrl: pwm_update - max_lo = 0x%04x, min_lo = 0x%04x, oldperiod = 0x%02x",max_lo,min_lo,oldperiod);

    if (period_mul == LEDCTRL_INVALID_PERIOD)
    {
        period_mul = oldperiod;
    }

    if (bright_duty == LEDCTRL_INVALID_DUTY_CYCLE)
    {
        /* Work out the stored bright_duty */
        bright_duty = (uint16)(min_lo / oldperiod);
        L4_DBG_MSG1("ledctrl: pwm_update - stored bright duty = 0x%04x",bright_duty);
    }

    if (dim_duty == LEDCTRL_INVALID_DUTY_CYCLE)
    {
        /* Work out the stored dim_duty */
        dim_duty = (uint16)(max_lo / oldperiod);
        /* Ensure it's less than the bright duty */
        if (dim_duty > bright_duty)
        {
            dim_duty = bright_duty;
        }
        L4_DBG_MSG1("ledctrl: pwm_update - stored dim duty = 0x%04x",dim_duty);
    }

    L4_DBG_MSG3("ledctrl: pwm_update bright duty 0x%04x, dim duty 0x%04x, pmul 0x%02x",bright_duty,dim_duty,period_mul);

    /* Write the values back to the registers */
    bankset_high_brightness(bright_duty, period_mul);
    bankset_low_brightness(dim_duty, period_mul);
}


/******************************************************************************
 * Synchronise all LEDs that are in the given group
 */
static void sync_group(uint16 group)
{
    uint16  led_mask = 0;
    uint8   led;

    /* Build up the mask */
    for (led=0; led < NUM_OF_LED_CTRLS; led++)
    {
        if (led_cfg[led].groups & (1<<group))
        {
            led_mask |= (uint16)(1<<led);
        }
    }

    hal_set_reg_led_update(led_mask); /* Write-sensitive despite docs */
    hal_set_reg_led_en(hal_get_reg_led_en() | led_mask);
}


/******************************************************************************
 *
 * Helper functions to write values that require multiple register accesses.
 * 
 * THE CORRECT LEDINDEX MUST HAVE BEEN SELECTED BEFORE THESE ARE CALLED
 */

/******************************************************************************
 *
 * Prefill the configuration registers with the previous values for that LED.
 * Note that not all registers are filled, as some aren't known at this time
 * (or have no 'previous value' to read from).
 */
static void bankset_preload(led_id led)
{
    uint16  log_stat;

#ifndef DESKTOP_TEST_BUILD
    hal_set_led_index(led);
#else
    UNUSED(led);
#endif

    hal_set_reg_led_pin_config(hal_get_reg_led_pin_config_status());
    hal_set_reg_led_min_low_config(hal_get_reg_led_min_low_config_status());
    hal_set_reg_led_min_high_config(hal_get_reg_led_min_high_config_status());
    hal_set_reg_led_max_low_config(hal_get_reg_led_max_low_config_status());
    hal_set_reg_led_max_high_config(hal_get_reg_led_max_high_config_status());
    hal_set_reg_led_hold_low_config(hal_get_reg_led_hold_low_config_status());
    hal_set_reg_led_hold_high_config(hal_get_reg_led_hold_high_config_status());
    hal_set_reg_led_ramp_config(hal_get_reg_led_ramp_config_status());
    hal_set_reg_led_ramp_shift(0); /* We never use this */
    log_stat = (uint16)hal_get_reg_led_logarithmic_status();
    hal_set_reg_led_logarithmic_en( (log_stat>>LOGSTAT_EN_OFFSET) & LOGSTAT_EN_MASK);
    hal_set_reg_led_logarithmic_offset_high( (log_stat>>LOGSTAT_OFFHI_OFFSET) & LOGSTAT_OFFHI_MASK);
    hal_set_reg_led_logarithmic_offset_low( (log_stat>>LOGSTAT_OFFLO_OFFSET) & LOGSTAT_OFFLO_MASK);
}


/*****************************************************************************
 *
 * Postfill the config registers with statuses that we only know after
 * a config action has been processed.
 */
static void bankset_postload(led_id led)
{
    if (is_steady(led))
    {
#ifndef DESKTOP_TEST_BUILD
        /* Set the single-shot registers dependent on the inversion state */
        if (is_inverted(led))
        {
            hal_set_led_start_up_state(LED_START_UP_STATE_COUNT_HIGH_RAMP_UP);
            hal_set_reg_led_single_shot_low_config(hal_get_reg_led_max_low_config());
            hal_set_reg_led_single_shot_high_config(hal_get_reg_led_max_high_config());
        }
        else
        {
            hal_set_led_start_up_state(LED_START_UP_STATE_COUNT_HIGH_RAMP_DOWN);
            hal_set_reg_led_single_shot_low_config(hal_get_reg_led_min_low_config());
            hal_set_reg_led_single_shot_high_config(hal_get_reg_led_min_high_config());
        }
#endif
    }
    else
    {
        hal_set_reg_led_start_up_state(led_cfg[led].init_state);
    }
    hal_set_reg_led_counthold_value(led_cfg[led].init_count);

    /* The enable and single-shot mode registers are set in the config fn */
}


/*****************************************************************************
 *
 * Writes the 'low brightness' into the MAX register pair (see ledctrl.h)
 */
static void bankset_low_brightness(uint16 duty_cycle, uint16 period_mul)
{
    hal_set_reg_led_max_low_config(duty_cycle * period_mul);
    hal_set_reg_led_max_high_config((LEDCTRL_MAX_DUTY_CYCLE - duty_cycle) * period_mul);
}

/*****************************************************************************
 *
 * Writes the 'high brightness' into the MIN register pair (see ledctrl.h)
 */
static void bankset_high_brightness(uint16 duty_cycle, uint16 period_mul)
{
    hal_set_reg_led_min_low_config(duty_cycle * period_mul);
    hal_set_reg_led_min_high_config((LEDCTRL_MAX_DUTY_CYCLE - duty_cycle) * period_mul);
}


#ifdef DEBUG_LEDCTRL_INTERNAL
void ledctrl_dumpregs(void)
{    
    L0_DBG_MSG2("LED_EN:                    0x%08x | LED_UPDATE:                 0x%08x", hal_get_reg_led_en(), hal_get_reg_led_update());
    L0_DBG_MSG2("LED_INDEX:                 0x%08x | LED_PIN_CONFIG:             0x%08x", hal_get_reg_led_index(), hal_get_reg_led_pin_config());
    L0_DBG_MSG2("LED_MIN_LOW_CONFIG:        0x%08x | LED_MIN_HIGH_CONFIG:        0x%08x", hal_get_reg_led_min_low_config(), hal_get_reg_led_min_high_config());
    L0_DBG_MSG2("LED_MAX_LOW_CONFIG:        0x%08x | LED_MAX_HIGH_CONFIG:        0x%08x", hal_get_reg_led_max_low_config(), hal_get_reg_led_max_high_config());
    L0_DBG_MSG2("LED_HOLD_LOW_CONFIG:       0x%08x | LED_HOLD_HIGH_CONFIG:       0x%08x", hal_get_reg_led_hold_low_config(), hal_get_reg_led_hold_high_config());
    L0_DBG_MSG2("LED_RAMP_CONFIG:           0x%08x | LED_RAMP_SHIFT:             0x%08x", hal_get_reg_led_ramp_config(), hal_get_reg_led_ramp_shift());
    L0_DBG_MSG2("LED_START_UP_STATE:        0x%08x | LED_COUNTHOLD_VALUE:        0x%08x", hal_get_reg_led_start_up_state(), hal_get_reg_led_counthold_value());
    L0_DBG_MSG2("LED_RAMP_CURR_LOW_CONFIG:  0x%08x | LED_RAMP_CURR_HIGH_CONFIG:  0x%08x", hal_get_reg_led_ramp_current_low_config(), hal_get_reg_led_ramp_current_high_config());
    L0_DBG_MSG1("LED_SINGLESHOT_MODE:       0x%08x", hal_get_reg_led_single_shot_mode());
    L0_DBG_MSG2("LED_SINGLESHOT_LOW_CONFIG: 0x%08x | LED_SINGLESHOT_HIGH_CONFIG: 0x%08x", hal_get_reg_led_single_shot_low_config(), hal_get_reg_led_single_shot_high_config());
    L0_DBG_MSG2("LED_MIN_LOW_CONFIG_STATUS: 0x%08x | LED_MIN_HIGH_CONFIG_STATUS: 0x%08x", hal_get_reg_led_min_low_config_status(), hal_get_reg_led_min_high_config_status());
    L0_DBG_MSG2("LED_MAX_LOW_CONFIG_STATUS: 0x%08x | LED_MAX_HIGH_CONFIG_STATUS: 0x%08x", hal_get_reg_led_max_low_config_status(), hal_get_reg_led_max_high_config_status());
    L0_DBG_MSG2("LED_HOLD_LOW_CONFIG_STATUS:0x%08x | LED_HOLD_HIGH_CONFIG_STATUS:0x%08x", hal_get_reg_led_hold_low_config_status(), hal_get_reg_led_hold_high_config_status());
    L0_DBG_MSG2("LED_RAMP_CONFIG_STATUS:    0x%08x | LED_PIN_CONFIG_STATUS:      0x%08x", hal_get_reg_led_ramp_config_status(), hal_get_reg_led_pin_config_status());
}
#endif

