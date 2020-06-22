/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       led_manager.c
\brief      LED Manager
*/

#include <pio.h>
#include <vm.h>
#include <led.h>
#include <sink.h>
#include <panic.h>

#include "led_manager.h"
#include "adk_log.h"

/*!< LED data structure */
led_manager_task_data_t led_mgr;

/*! Macro to make LED manager messages. */
#define MAKE_LED_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*!  Internal messages */
enum
{
    LED_INTERNAL_UPDATE,            /*!< Update LEDs */
    LED_INTERNAL_SET_PATTERN,
    LED_INTERNAL_STOP_PATTERN,
    LED_INTERNAL_ENABLE,
};

typedef struct
{
    led_priority_t priority;
} LED_INTERNAL_SET_PRIORITY_T;

typedef struct
{
    led_priority_t priority;
    const led_pattern_t *pattern;
    uint16 *client_lock;
    uint16 client_lock_mask;

} LED_INTERNAL_SET_PATTERN_T;

typedef struct
{
    led_priority_t priority;
} LED_INTERNAL_STOP_PATTERN_T;

typedef struct
{
    bool enable;
} LED_INTERNAL_ENABLE_T;

static unsigned ledManager_getPioBank(unsigned led_pio)
{
    return led_pio >> 5;
}

static unsigned ledManager_getPioMask(unsigned led_pio)
{
    return 0x1UL << (led_pio & 31);
}

static uint16 ledManager_GetPioNumForLed(uint16 led_num)
{
    uint16 led_pio = 0;
    if (led_num == 0)
    {
        led_pio = led_mgr.hw_config->led0_pio;
    }
    else if (led_num == 1)
    {
        led_pio = led_mgr.hw_config->led1_pio;
    }
    else if (led_num == 2)
    {
        led_pio = led_mgr.hw_config->led2_pio;
    }
    return led_pio;
}

static void ledManager_SetPio(uint16 led_num, uint16 led_state)
{
    uint16 led_pio = ledManager_GetPioNumForLed(led_num);
    uint16 pio_mask = ledManager_getPioMask(led_pio);
    PioSet32Bank(ledManager_getPioBank(led_pio), pio_mask, (led_state & 0x1<<led_num) ? 0 : pio_mask);
}

/*! \brief Update LEDs

    This function is called to update the LEDs, it checks the current LED state and
    runs through the active filters modifying the LEDs state as specified by
    the filters.
*/
static void ledManager_Update(void)
{
    int filter;
    uint16 led_state = led_mgr.led_state;

    /* Run LEDs through filters */
    for (filter = 0; filter < LED_NUM_PRIORITIES; filter++)
    {
        led_filter_t filter_func = led_mgr.filter[filter];
        if (filter_func)
            led_state = filter_func(led_state);
    }

    /* Update LEDs */
    if (led_mgr.hw_config->leds_use_pio)
    {
        switch (led_mgr.hw_config->number_of_leds)
        {
            /* Jump then fall-through to set the correct number of LEDS */
            case 3: ledManager_SetPio(2, led_state);
            case 2: ledManager_SetPio(1, led_state);
            case 1: ledManager_SetPio(0, led_state);
            default: break;
        }
    }
    else
    {
        switch (led_mgr.hw_config->number_of_leds)
        {
            case 3: LedConfigure(2, LED_ENABLE, led_state & 0x04 ? 1 : 0);
            case 2: LedConfigure(1, LED_ENABLE, led_state & 0x02 ? 1 : 0);
            case 1: LedConfigure(0, LED_ENABLE, led_state & 0x01 ? 1 : 0);
            default: break;
        }
    }
}

/*! \brief Update LED pattern

    This function is called to update the LED pattern, it is called
    on reception of the internal LED_INTERNAL_UPDATE message.

    This function walks through the LED pattern definition running
    the specified actions and only exits when it encounters a delay
    action or the pattern is completed.

    When a delay is encountered, a delayed LED_INTERNAL_UPDATE message
    is sent to the LED task with the appropriate delay.
*/
static bool ledManager_HandleInternalUpdate(void)
{
    bool update_leds = FALSE;
    MessageCancelAll(&led_mgr.task, LED_INTERNAL_UPDATE);
    for (;;)
    {
        led_priority_state_t *state = &led_mgr.priority_state[led_mgr.priority];
        led_stack_t *stack = &state->stack[state->stack_ptr];
        const led_pattern_t *pattern = state->pattern + stack->position;

        switch (pattern->code)
        {
            case LED_PATTERN_END:
            {
                /* Stop this pattern */
                LedManager_StopPattern(led_mgr.priority);

                /* Exit loop */
                return FALSE;
            }

            case LED_PATTERN_ON:
            {
                /* Update LEDs on exit */
                update_leds = TRUE;

                /* Turn on LED */
                led_mgr.led_state |= pattern->data;

                /* Move to next instruction */
                stack->position++;
            }
            break;

            case LED_PATTERN_OFF:
            {
                /* Update LEDs on exit */
                update_leds = TRUE;

                /* Turn off LED */
                led_mgr.led_state &= ~pattern->data;

                /* Move to next instruction */
                stack->position++;
            }
            break;

            case LED_PATTERN_TOGGLE:
            {
                /* Update LEDs on exit */
                update_leds = TRUE;

                /* Toggle LED */
                led_mgr.led_state ^= pattern->data;

                /* Move to next instruction */
                stack->position++;
            }
            break;

            case LED_PATTERN_DELAY:
            {
                /* Send timed message if not infinite delay */
                if (pattern->data)
                    MessageSendLater(&led_mgr.task, LED_INTERNAL_UPDATE, 0, pattern->data);

                /* Move to next instruction */
                stack->position++;

                /* Exit loop */
                return update_leds;
            }

            case LED_PATTERN_SYNC:
            {
                /* Attempt to get wallclock from sink */
                wallclock_state_t wc_state;
                rtime_t wallclock;

                if (SinkIsValid(led_mgr.wallclock_sink) &&
                    (RtimeWallClockGetStateForSink(&wc_state, led_mgr.wallclock_sink) &&
                     RtimeLocalToWallClock(&wc_state, VmGetTimerTime(), &wallclock)) )
                {
                    uint32_t offset = wallclock % (pattern->data * 1000);
                    rtime_t local;

                    rtime_t sync_time = rtime_sub(wallclock, offset);
                    sync_time = rtime_add(sync_time, pattern->data * 1000);

                    if (RtimeWallClockToLocal(&wc_state, sync_time, &local))
                    {
                        /* Convert to milliseconds in the future */
                        int32_t delay = rtime_sub(local, VmGetTimerTime()) / 1000;

                        /* If value is negative, adjust by period to make in future again */
                        while (delay < 0)
                            delay += pattern->data;

                        /* Wait for specified delay */
                        MessageSendLater(&led_mgr.task, LED_INTERNAL_UPDATE, 0, delay);
                    }
                    else
                        Panic();
                }
                else
                {
                    /* We weren't able to use the wallclock to adjust the delay
                       so use the un-modified local delay. */
                    uint32 sync_delay = pattern->data - (VmGetClock() % pattern->data);
                    MessageSendLater(&led_mgr.task, LED_INTERNAL_UPDATE, 0, sync_delay);
                }

                /* Move to next instruction */
                stack->position++;

                /* Exit loop */
                return update_leds;
            }

            case LED_PATTERN_REPEAT:
            {
                /* Check if not at end off loop */
                if (stack->position != stack->loop_end)
                {
                    /* Push new loop onto stack */
                    led_stack_t *stack_new   = &state->stack[++state->stack_ptr];
                    stack_new->loop_count = pattern->data & 0x3F;
                    stack_new->loop_start = pattern->data >> 6;
                    stack_new->loop_end   = stack->position;
                    stack_new->position   = stack_new->loop_start;

                    /* Move to next instruction */
                    stack->position++;
                }
                else
                {
                    /* Check if loop is not infinite */
                    if (stack->loop_count != 0)
                    {
                        /* Decrement loop counter */
                        stack->loop_count--;

                        /* Pop top of stack if we have finished loop */
                        if (stack->loop_count == 0)
                            state->stack_ptr--;
                    }

                    /* Jump to start of loop */
                    stack->position = stack->loop_start;
                }
            }
            break;

            case LED_PATTERN_LOCK:
            {
                /* Update lock */
                led_mgr.lock = pattern->data;

                /* Move to next instruction */
                stack->position++;

                /* Check if we're not locked anymore */
                if (!led_mgr.lock)
                {
                    /* Unlocked, send update message to ourselves to allow any blocked messages to be delivered */
                    MessageSend(&led_mgr.task, LED_INTERNAL_UPDATE, 0);

                    /* Exit loop */
                    return update_leds;
                }
            }
            break;
        }
    }
}

/*! \brief Set priority level of active pattern

    This function is called internally to set the priority level of the active
    LED pattern.
*/
static void ledManager_SetPriority(int8 priority)
{
    DEBUG_LOG("LedManager_SetPriority %d", priority);

    /* Store new priority */
    led_mgr.priority = priority;

    /* Clear LEDs */
    led_mgr.led_state = 0;

    /* Cancel LED update message */
    MessageCancelFirst(&led_mgr.task, LED_INTERNAL_UPDATE);

    /* Post update message to LED task if active */
    if (led_mgr.priority >= 0)
        MessageSend(&led_mgr.task, LED_INTERNAL_UPDATE, 0);
    else
        ledManager_Update();
}


/*! \brief Message Handler

    This function is the message handler for the LED task, it only handles
    one message, LED_INTERNAL_UPDATE.
*/
static void ledManager_Handler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case LED_INTERNAL_UPDATE:
        {
            if (ledManager_HandleInternalUpdate())
            {
                /* We need to update LEDs */
                ledManager_Update();
            }
        }
        break;

        case LED_INTERNAL_SET_PATTERN:
        {
            LED_INTERNAL_SET_PATTERN_T *req = (LED_INTERNAL_SET_PATTERN_T *)message;
            led_priority_state_t *state = &led_mgr.priority_state[req->priority];

            if (req->priority == LED_PRI_EVENT)
            {
                led_mgr.client_lock = req->client_lock;
                led_mgr.client_lock_mask = req->client_lock_mask;
            }

            state->pattern = req->pattern;
            state->stack_ptr = 0;
            state->stack[0].position = 0;
            state->stack[0].loop_start = 0;
            state->stack[0].loop_end = 0;
            state->stack[0].loop_count = 0;

            /* Check if LEDs are enabled */
            if (led_mgr.enable)
            {
                if (req->priority >= led_mgr.priority)
                    ledManager_SetPriority(req->priority);
            }
        }
        break;

        case LED_INTERNAL_STOP_PATTERN:
        {
            LED_INTERNAL_STOP_PATTERN_T *req = (LED_INTERNAL_STOP_PATTERN_T *)message;
            int8 priority;

            /* Clear pattern */
            led_mgr.priority_state[req->priority].pattern = NULL;

            if (req->priority == LED_PRI_EVENT && led_mgr.client_lock != NULL)
            {
                *led_mgr.client_lock &= ~led_mgr.client_lock_mask;
                led_mgr.client_lock = NULL;
                led_mgr.client_lock_mask = 0;
            }

            /* Check if LEDs are enabled */
            if (led_mgr.enable)
            {
                /* Find highest priority */
                priority = LED_NUM_PRIORITIES;
                while (priority--)
                    if (led_mgr.priority_state[priority].pattern)
                        break;
            }
            else
            {
                /* LEDs are disabled */
                priority = -1;
            }

            /* Set highest priority pattern */
            ledManager_SetPriority(priority);
        }
        break;

        case LED_INTERNAL_ENABLE:
        {
            LED_INTERNAL_ENABLE_T *req = (LED_INTERNAL_ENABLE_T *)message;

            /* Check if we have enabled LEDs */
            if (req->enable & !led_mgr.enable)
            {
                /* Set enable flag */
                led_mgr.enable = req->enable;

                /* Find highest priority */
                int8 priority = LED_NUM_PRIORITIES;
                while (priority--)
                    if (led_mgr.priority_state[priority].pattern)
                        break;

                /* Set highest priority pattern */
                ledManager_SetPriority(priority);
            }
            else if (!req->enable & led_mgr.enable)
            {
                /* Clear enable flag */
                led_mgr.enable = req->enable;

                /* Set priority to -1 to disable updates and turn off LEDs */
                ledManager_SetPriority(-1);
            }
        }
        break;

        default:
            Panic();
    }
}

static void ledManager_setupSingleLedPioHw(unsigned led_pio)
{
    unsigned bank = ledManager_getPioBank(led_pio);
    unsigned mask = ledManager_getPioMask(led_pio);

    PioSetMapPins32Bank(bank, mask, mask);
    PioSetDir32Bank(bank, mask, mask);
}

static void ledManager_setupLedPioHw(void)
{
    switch (led_mgr.hw_config->number_of_leds)
    {
        // Jump then fall-through to setup the correct number of LEDS
        case 3:
            ledManager_setupSingleLedPioHw(led_mgr.hw_config->led2_pio);
        case 2:
            ledManager_setupSingleLedPioHw(led_mgr.hw_config->led1_pio);
        case 1:
            ledManager_setupSingleLedPioHw(led_mgr.hw_config->led0_pio);
        default:
            break;
    }
}

void LedManager_SetHwConfig(const led_manager_hw_config_t * config)
{
    PanicFalse(config != NULL);

    led_mgr.hw_config = config;
}

/*! \brief Initialise LED module

    This function is called at startup to initialise the LED module.
*/
bool LedManager_Init(Task init_task)
{
    int8 priority;
    uint8 filter;

    led_mgr.task.handler = ledManager_Handler;
    led_mgr.priority = -1;
    led_mgr.led_state = 0;
    led_mgr.enable = TRUE;
    led_mgr.lock = 0;

    /* Clear patterns */
    for (priority = 0; priority < LED_NUM_PRIORITIES; priority++)
        led_mgr.priority_state[priority].pattern = NULL;

    /* Clear filters */
    for (filter = 0; filter < LED_NUM_PRIORITIES; filter++)
        led_mgr.filter[filter] = NULL;

    /* Initialise LED hardware */
    PanicFalse(led_mgr.hw_config != NULL);
    if (led_mgr.hw_config->leds_use_pio)
    {
        ledManager_setupLedPioHw();
    }

    UNUSED(init_task);
    return TRUE;
}

/*! \brief Enable/Disable LEDs

    This function is called to enable or disable all LED indications.

    \param enable Whether to enable or disable.
*/
void LedManager_Enable(bool enable)
{
    MAKE_LED_MESSAGE(LED_INTERNAL_ENABLE);

    DEBUG_LOG("LedManager_Enable, enable %d", enable);

    message->enable = enable;
    MessageSendConditionally(&led_mgr.task, LED_INTERNAL_ENABLE, message, &led_mgr.lock);
}

void LedManager_SetPattern(
        const led_pattern_t *pattern,
        led_priority_t priority,
        uint16 *client_lock,
        uint16 client_lock_mask)
{
    MAKE_LED_MESSAGE(LED_INTERNAL_SET_PATTERN);

    DEBUG_LOG("LedManager_SetPattern pattern %p, priority %d", pattern, priority);

    PanicFalse(priority < LED_NUM_PRIORITIES);

    message->pattern = pattern;
    message->priority = priority;
    message->client_lock = client_lock;
    message->client_lock_mask = client_lock_mask;

    MessageSendConditionally(&led_mgr.task, LED_INTERNAL_SET_PATTERN, message, &led_mgr.lock);
}

/*! \brief Stop the LED pattern running at the specified priority.

    \param priority The priority of the LED pattern to be cancelled. The range is 0
                to \ref LED_NUM_PRIORITIES -1.
*/
void LedManager_StopPattern(led_priority_t priority)
{
    MAKE_LED_MESSAGE(LED_INTERNAL_STOP_PATTERN);

    DEBUG_LOG("LedManager_StopPattern, priority %d", priority);

    PanicFalse(priority < LED_NUM_PRIORITIES);

    message->priority = priority;
    MessageSendConditionally(&led_mgr.task, LED_INTERNAL_STOP_PATTERN, message, &led_mgr.lock);
}

/*! \brief Set LED filter

    This function is called to set a LED filter, a priority can
    be specified to control the order in which filters are applied.

    \param filter_func  The filter function to be used
    \param filter_pri   The filter to be set. Range is 0 to \ref LED_NUM_PRIORITIES - 1
*/
void LedManager_SetFilter(led_filter_t filter_func, led_priority_t filter_pri)
{
    PanicFalse(filter_pri < LED_NUM_PRIORITIES);

    /* Store new filter and update LEDs if filter changed */
    if (led_mgr.filter[filter_pri] != filter_func)
    {
        led_mgr.filter[filter_pri] = filter_func;
        ledManager_Update();
    }
}

/*! \brief Get the LED filter active at the specified priority

    \param filter_pri The filter to get. Range is 0 to \ref LED_NUM_PRIORITIES -1

    \return The filter function currently in use
*/
led_filter_t LedManager_GetFilter(led_priority_t filter_pri)
{
    PanicFalse(filter_pri < LED_NUM_PRIORITIES);

    return led_mgr.filter[filter_pri];
}

/*! \brief Cancel LED filter

    Cancel previously applied filter at the specified filter priority.

    \param filter_pri The filter to be cancelled. Range is 0 to \ref LED_NUM_PRIORITIES -1
*/
void LedManager_CancelFilter(led_priority_t filter_pri)
{
    PanicFalse(filter_pri < LED_NUM_PRIORITIES);

    /* Cancel filter and update LEDS if filter active */
    if (led_mgr.filter[filter_pri] != NULL)
    {
        led_mgr.filter[filter_pri] = NULL;
        ledManager_Update();
    }
}

/*! \brief Set the time base for the LEDs to match a sink

    Stores the supplied sink for later use by the LED manager.

    \param sink The sink to be used as a time base.
*/
void LedManager_SetWallclock(Sink sink)
{
    led_mgr.wallclock_sink = sink;
}

bool LedManager_IsPioAnLed(unsigned pio)
{
    PanicFalse(led_mgr.hw_config != NULL);
    return pio >= led_mgr.hw_config->led0_pio &&
           pio <= led_mgr.hw_config->led2_pio;
}

unsigned LedManager_GetLedNumberForPio(unsigned pio)
{
    PanicFalse(led_mgr.hw_config != NULL);
    return pio - led_mgr.hw_config->led0_pio;
}

bool LedManager_PioCanWakeFromDormant(unsigned pio)
{
    return pio >= 1 && pio <= 8;
}
