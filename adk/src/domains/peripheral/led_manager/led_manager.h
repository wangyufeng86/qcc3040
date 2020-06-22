/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       led_manager.h
\brief      Header file for the LED Manager

This file provides functionality for controlling LED flashes.

The LED flashing code supports the concept of priority levels where a
given priority can be assigne to each pattern so that a higher priority
LED indication can interrupt a lower priority one. By default the code
supports up to four LED priority levels.

This code also allows LED filters to be defined. When using these an
application author can define that on a certain condition (for example
low battery state) all LED indications are displayed using the red LED
rather than the blue one.
*/

#ifndef LED_MANAGER_H_
#define LED_MANAGER_H_

#include <message.h>
#include <rtime.h>

/*!@{ \name LED priority

    \brief Macros that can be used to define priority for an LED.

    Higher numbers are higher in priority.
*/
typedef enum
{
    LED_PRI_LOW,
    LED_PRI_MEDIUM,
    LED_PRI_HIGH,
    LED_PRI_EVENT,
    LED_NUM_PRIORITIES
} led_priority_t;
/*!@} */

/*!
	\brief Structure for defining each LED pattern.

	Each LED pattern is built up using a sequence of primitives that define
	when each LED pin is turned on or off. These basic building blocks are used
	to build up the sequence of actions that make up a given LED pattern.
*/
typedef struct
{
    unsigned int code:3;	/*!< Code to control the next sequence in the pattern */
    unsigned int data:13;	/*!< LED pins controlled by the code */
} led_pattern_t;

/*! Type of filter function used for LEDs. See \ref LedManager_SetFilter */
typedef uint16 (*led_filter_t)(uint16);


/*!@{
    \name LED Pattern macros

    \brief Macros used to control the LED pattern displayed

    The macros below provide a top level interface for
    specifying the individual components that can be used to
    build up an LED pattern. They can also be used to control
    how the patterns are displayed.

    See earbud_ui.c for examples
*/
#define LED_PATTERN_END     (0x0000)            /*!< Marker for the end of an LED pattern */
#define LED_PATTERN_ON      (0x0001)            /*!< Marker for an led on instruction */
#define LED_PATTERN_OFF     (0x0002)            /*!< Marker for an led off instruction */
#define LED_PATTERN_TOGGLE  (0x0003)            /*!< Marker for an led toggle instruction */
#define LED_PATTERN_REPEAT  (0x0004)            /*!< Marker for a repeat instruction */
#define LED_PATTERN_DELAY   (0x0005)            /*!< Marker for a delay instruction */
#define LED_PATTERN_SYNC    (0x0006)            /*!< Marker for a synchronisation pause instruction */
#define LED_PATTERN_LOCK    (0x0007)            /*!< Marker for a lock/unlock instruction */

#define LED_ON(pio)             {LED_PATTERN_ON,  (pio)}                        /*!< Turn on LEDs */
#define LED_OFF(pio)            {LED_PATTERN_OFF, (pio)}                        /*!< Turn off LEDs */
#define LED_TOGGLE(pio)         {LED_PATTERN_TOGGLE, (pio)}                     /*!< Toggle the LEDs */
#define LED_REPEAT(loop, count) {LED_PATTERN_REPEAT, (loop) << 6 | (count)}     /*!< Defines how many times the primitives above this statement should be repeated */
#define LED_WAIT(delay)         {LED_PATTERN_DELAY, (delay)}                    /*!< Defines a duration for which the pattern is not updated */
#define LED_SYNC(sync)          {LED_PATTERN_SYNC, (sync)}                      /*!< Wait until clock reaches sychronisation interval */
#define LED_END                 {LED_PATTERN_END, 0}                            /*!< Used to specify the end of the LED pattern */
#define LED_LOCK                {LED_PATTERN_LOCK, 1}                           /*!< Lock pattern, prevents any else from interrupting pattern */
#define LED_UNLOCK              {LED_PATTERN_LOCK, 0}                           /*!< Unlock pattern, allows something else to pattern */
/*!@} */

/*! Stack element */
typedef struct
{
    unsigned int loop_start:5;     /*!< Index into pattern for start of loop */
    unsigned int loop_end:5;       /*!< Index into pattern for end of loop */
    unsigned int position:5;       /*!< Current position index in pattern */
    unsigned int loop_count:6;     /*!< Number of loops remaining */
} led_stack_t;

/*! \brief LED priority structure

    This structure hold the state for LED patterns at a particular priority.
    It contains a pointer to the current LED pattern and a 'stack' to allow for
    nesting of loops in the pattern definition.
*/
typedef struct
{
    const led_pattern_t *pattern;         /*!< Pointer to LED pattern */
    unsigned int         stack_ptr:2;     /*!< Index into stack array */
    led_stack_t          stack[3];        /*!< Array of stack elements */
} led_priority_state_t;

/*! \brief led configuration structure */
typedef struct
{
    unsigned number_of_leds:2;
    /*! Use PIOs for LED control, rather than the LED hardware blocks. PIOs were found to use lower power for the use cases here. */
    unsigned leds_use_pio:1;
    /*! The PIO that LED 0 is connected on. This is only required if leds_use_pio is set to TRUE. */
    unsigned led0_pio;
    /*! The PIO that LED 1 is connected on. This is only required if leds_use_pio is set to TRUE and number_of_leds is greater than 1. */
    unsigned led1_pio;
    /*! The PIO that LED 2 is connected on. This is only required if leds_use_pio is set to TRUE and number_of_leds is greater than 2. */
    unsigned led2_pio;

} led_manager_hw_config_t;

/*! LED Task Structure */
typedef struct
{
    TaskData               task;                                /*!< LED task */
    uint16                 led_state;                           /*!< Current LED PIO state */
    signed                 priority:3;                          /*!< Current priority level */
    unsigned               enable:1;                            /*!< Flag, set if LEDs are enabled */
    led_priority_state_t   priority_state[LED_NUM_PRIORITIES];  /*!< Array of LED priority structures */
    led_filter_t           filter[LED_NUM_PRIORITIES];          /*!< Array of LED filters */
    Sink                   wallclock_sink;                      /*!< Sink to get wallclock used for common timebase */
    uint16                 lock;                                /*!< If the current pattern cannot be interrupted */
    uint16 *               client_lock;
    uint16                 client_lock_mask;

    const led_manager_hw_config_t * hw_config;

} led_manager_task_data_t;

/*! \brief Inform the LED Manager of the Hardware configuration of the LEDs.

    This API is used by the Application to inform the LED Manager of how the LEDS are
    interface to the chip.

    \note This function must be called by the application BEFORE initialising the
          LED Manager module. I.e. calling LedManager_Init(). This is required because
          the LED Manager needs to know the connections that the LED hardware are
          present upon in order to configure and initialise itself.

    \param config - a pointer to a const structure describing the LED HW configuration.
*/
void LedManager_SetHwConfig(const led_manager_hw_config_t * config);

bool LedManager_Init(Task init_task);

void LedManager_Enable(bool enable);

/*! \brief Set LED pattern

    This function is called to set the LED pattern with the specified
    priority.  If the new priority is higher than the current priority
    then the new pattern is show immediately, otherwise the pattern will
    only be shown when the higher priority pattern has completed.

    \param[in]  pattern  Pointer to the LED pattern to be applied.
    \param      priority The priority of the LED pattern to be set. The range is 0
            to \ref LED_NUM_PRIORITIES -1.
    \param      client_lock The client's lock; in the case of EVENT priority LED flashes
            this shall be cleared when the LED flash is completed
    \param      client_lock_mask the bit in the client lock to clear

*/
void LedManager_SetPattern(
        const led_pattern_t *pattern,
        led_priority_t priority,
        uint16 *client_lock,
        uint16 client_lock_mask);
void LedManager_StopPattern(led_priority_t priority);

void LedManager_SetFilter(led_filter_t filter_func, led_priority_t priority);
led_filter_t LedManager_GetFilter(led_priority_t priority);
void LedManager_CancelFilter(led_priority_t filter_pri);

void LedManager_SetWallclock(Sink sink);

/*! \param pio - the number of the PIO to check
    \return boolean TRUE if PIO is an LED pin, otherwise FALSE */
bool LedManager_IsPioAnLed(unsigned pio);

/*! \param pio - the number of the PIO to check
    \return the LED number of any LED connected to the specified PIO number */
unsigned LedManager_GetLedNumberForPio(unsigned pio);

/*! \param pio - the number of the PIO to check
    \return boolean indicating if the specified PIO can wake the chip from dormant */
bool LedManager_PioCanWakeFromDormant(unsigned pio);

#endif /* LED_MANAGER_H_ */
