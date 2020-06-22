/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Public API for LED UI Inidcator. This converts System Events to corresponding
            LED UI Indications by table look-up, using a configuration table passed in by
            the Application.
            It then schedules these LED flashes and filters using the LED Manager.
*/
#ifndef UI_LEDS_H
#define UI_LEDS_H

#include <csrtypes.h>
#include "ui_indicator.h"

#include <led_manager.h>

#define UI_LEDS_WAIT_FOR_FLASH_COMPLETION 0x1

/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when an LED event flash is indicated. */
    UI_INTERNAL_LED_FLASH_COMPLETED,
};

#define LED_NONE                    0xFFFF
#define CONTEXT_INDICATION_MASK     (0x1 << 15)

/*! \brief ui_led task structure */
typedef struct
{
    /*! The task. */
    TaskData task;

    /*! The configuration table of System Event to LED flash patterns, passed from the Application. */
    const ui_event_indicator_table_t * sys_event_to_led_data_mappings;
    uint8 event_mapping_table_size;

    /*! The configuration table of System Event to LED flash patterns, passed from the Application. */
    const ui_provider_context_consumer_indicator_table_t * context_to_led_data_mappings;
    uint8 context_mapping_table_size;
    uint16 curr_indicated_context_index;
    uint16 curr_indicated_provider;

    /*! Used to trigger a conditional message send when the current requested led flash has been indicated. */
    uint16 led_event_flash_ongoing_mask;

    /*! Needed to indicate back to the Power Manager UI LED flash completion, if a flash was configured to be played on Power Off. */
    bool indicate_when_power_shutdown_prepared;

} ui_leds_task_data_t;

/*! \brief Get a pointer to the LED UI Indicator task data.

    \returns LED UI Indicator task
 */
Task UiLeds_GetUiLedsTask(void);

/*! \brief Set the LED UI Indicator configuration.

    This API is used by the Application to configure the LED UI Indications that shall be presented to the user.

    \param table The configuration table mapping System Events to LED UI Indication data
    \param size The length of the configuration table (i.e. the number of configured LED UI Indications)
*/
void UiLeds_SetLedConfiguration(
        const ui_event_indicator_table_t *ui_event_table,
        uint8 ui_event_table_size,
        const ui_provider_context_consumer_indicator_table_t * ui_context_table,
        uint8 ui_context_table_size);

/*! \brief Initialise the LED UI Indicator module. */
bool UiLeds_Init(Task init_task);

/*! \brief Notify this module of a UI Indication received from another source, for e.g. a Peer device.

    This API is used by the CAA framework UI Component to inform the LED UI Indicator of an externally
    generated UI Event that shall be presented to the user.

    Normally this will be from a Peer device. An example might be an HFP ring indication from the
    Primary Earbud to the Secondary to display on the Secondary device's LEDs.

    \param led_index - the index of the LED flash in the LED Configuration Table to indicate
*/
void UiLeds_NotifyUiIndication(uint16 led_index);

#endif // UI_LEDS_H
