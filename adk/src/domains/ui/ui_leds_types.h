#ifndef UI_LEDS_TYPES_H
#define UI_LEDS_TYPES_H

#include <csrtypes.h>
#include <led_manager.h>

typedef enum
{
    LED_START_PATTERN,
    LED_STOP_PATTERN,
    LED_SET_FILTER,
    LED_CANCEL_FILTER,
    LED_NUM_ACTIONS
} led_action_t;

typedef union
{
    const led_pattern_t* pattern;
    led_filter_t filter;

} led_action_data_t;

typedef struct
{
    led_action_t action;
    led_action_data_t data;
    led_priority_t priority;
    /*! If an LED indication is local only, it shall not be forwarded externally, for e.g. to a Peer device. */
    bool local_only;

} ui_led_data_t;

#endif // UI_LEDS_TYPES_H
