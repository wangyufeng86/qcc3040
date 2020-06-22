/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file for the Earbud Application user interface LED indications.
*/
#include "earbud_led.h"
#include "earbud_ui.h"

const led_manager_hw_config_t earbud_led_config =
#if defined(HAVE_1_LED)
{
    .number_of_leds = 1,
    .leds_use_pio = TRUE,
    .led0_pio = CHIP_LED_0_PIO,
    .led1_pio = 0,
    .led2_pio = 0,
};
#elif defined(HAVE_3_LEDS)
{
    .number_of_leds = 3,
    .leds_use_pio = TRUE,
    .led0_pio = CHIP_LED_0_PIO,
    .led1_pio = CHIP_LED_1_PIO,
    .led2_pio = CHIP_LED_2_PIO,
};
#else
#error LED config not correctly defined.
#endif

/*!@{ \name Definition of LEDs, and basic colour combinations

    The basic handling for LEDs is similar, whether there are
    3 separate LEDs, a tri-color LED, or just a single LED.
 */
#if (HAVE_3_LEDS)
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 2)
#else
/* We only have 1 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 0)
#define LED_2_STATE  (1 << 0)
#endif

#define LED_GREEN   (LED_1_STATE)
#ifdef CORVUS_YD300
#define LED_BLUE    (LED_2_STATE)
#define LED_RED     (LED_0_STATE)
#else
#define LED_BLUE    (LED_0_STATE)
#define LED_RED     (LED_2_STATE)
#endif

#define LED_WHITE   (LED_0_STATE | LED_1_STATE | LED_2_STATE)
#define LED_YELLOW  (LED_RED | LED_GREEN)
/*!@} */

/*! \brief An LED filter used for battery low

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_battery_low(uint16 led_state)
{
    return (led_state) ? LED_RED : 0;
}

/*! \brief An LED filter used for low charging level

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_low(uint16 led_state)
{
    UNUSED(led_state);
    return LED_RED;
}

/*! \brief An LED filter used for charging level OK

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_ok(uint16 led_state)
{
    UNUSED(led_state);
    return LED_YELLOW;
}

/*! \brief An LED filter used for charging complete

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_complete(uint16 led_state)
{
    UNUSED(led_state);
    return LED_GREEN;
}

/*! \brief An LED filter used for indicating that the earbud has the Primary role

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_primary(uint16 led_state)
{
    return (led_state) ? LED_BLUE : 0;
}

/*! \brief An LED filter used for indicating that the earbud has the Secondary role

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_secondary(uint16 led_state)
{
    return (led_state) ? LED_RED : 0;
}

/*! \cond led_patterns_well_named
    No need to document these. The public interface is
    from public functions such as EarbudUi_PowerOn()
 */

const led_pattern_t app_led_pattern_power_on[] =
{
    LED_LOCK,
    LED_ON(LED_RED),    LED_WAIT(100),
    LED_ON(LED_GREEN),  LED_WAIT(100),
    LED_ON(LED_BLUE),   LED_WAIT(100),
    LED_OFF(LED_RED),   LED_WAIT(100),
    LED_OFF(LED_GREEN), LED_WAIT(100),
    LED_OFF(LED_BLUE),  LED_WAIT(100),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_power_off[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_error[] =
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_idle[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_idle_connected[] =
{
    LED_SYNC(1000),
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_pairing[] =
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

const led_pattern_t app_led_pattern_pairing_deleted[] =
{
    LED_LOCK,
    LED_ON(LED_YELLOW), LED_WAIT(100), LED_OFF(LED_YELLOW), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_peer_pairing[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(50), LED_OFF(LED_BLUE), LED_WAIT(50),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

#ifdef INCLUDE_DFU
const led_pattern_t app_led_pattern_dfu[] =
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_WAIT(400),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};
#endif

#ifdef INCLUDE_AV
const led_pattern_t app_led_pattern_streaming_aptx[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(50), LED_OFF(LED_GREEN), LED_WAIT(50),
    LED_REPEAT(2, 2),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};
#endif

#ifdef INCLUDE_AV
const led_pattern_t app_led_pattern_streaming[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(50), LED_OFF(LED_RED), LED_WAIT(50),
    LED_REPEAT(2, 2),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};
#endif

const led_pattern_t app_led_pattern_sco[] =
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_GREEN), LED_WAIT(50), LED_OFF(LED_GREEN), LED_WAIT(50),
    LED_REPEAT(2, 1),
    LED_WAIT(500),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_call_incoming[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_WHITE), LED_WAIT(50), LED_OFF(LED_WHITE), LED_WAIT(50),
    LED_REPEAT(2, 1),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_battery_empty[] =
{
    LED_LOCK,
    LED_ON(LED_RED),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_factory_reset[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN), LED_WAIT(100),
    LED_REPEAT(2, 2),
    LED_UNLOCK,
    LED_END
};

#ifdef PRODUCTION_TEST_MODE

const led_pattern_t app_led_pattern_dh5[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_GREEN), LED_WAIT(500), LED_OFF(LED_GREEN), LED_WAIT(1000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_2dh5[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_GREEN), LED_WAIT(200), LED_OFF(LED_GREEN), LED_WAIT(200),
    LED_ON(LED_GREEN), LED_WAIT(200), LED_OFF(LED_GREEN), LED_WAIT(1000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const led_pattern_t app_led_pattern_3dh5[] =
{
    LED_LOCK,
    LED_SYNC(1000),
    LED_ON(LED_GREEN), LED_WAIT(200), LED_OFF(LED_GREEN), LED_WAIT(200),
    LED_ON(LED_GREEN), LED_WAIT(200), LED_OFF(LED_GREEN), LED_WAIT(200),
    LED_ON(LED_GREEN), LED_WAIT(200), LED_OFF(LED_GREEN), LED_WAIT(1000),
    LED_UNLOCK,
	LED_REPEAT(0, 0),
};

#endif /*PRODUCTION_TEST_MODE*/

const led_pattern_t app_led_pattern_anc_on[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(500), LED_OFF(LED_BLUE),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_anc_off[] =
{
    LED_LOCK,
    LED_ON(LED_BLUE), LED_WAIT(500), LED_OFF(LED_BLUE),
    LED_UNLOCK,
    LED_END
};

const led_pattern_t app_led_pattern_anc_mode_changed[] =
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED),LED_WAIT(100),
    LED_ON(LED_GREEN), LED_WAIT(100), LED_OFF(LED_GREEN),LED_WAIT(100),
    LED_REPEAT(1, 1),
    LED_UNLOCK,
    LED_END
};

/*! \endcond led_patterns_well_named
 */


