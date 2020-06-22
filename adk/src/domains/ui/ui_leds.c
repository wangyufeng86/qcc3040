/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source File for the LED UI Inidcator. This converts System Events to corresponding
            LED UI Indications by table look-up, using a configuration table passed in by
            the Application. It then schedules these LED flashes and filters using the LED Manager.
*/
#include "ui_leds.h"

#include <ui.h>

#include <logging.h>
#include <led_manager.h>
#include <power_manager.h>

ui_leds_task_data_t the_leds;

static bool uiLeds_GetLedIndexFromMappingTable(MessageId id, uint16 * led_index)
{
    return UiIndicator_GetIndexFromMappingTable(
                the_leds.sys_event_to_led_data_mappings,
                the_leds.event_mapping_table_size,
                id,
                led_index);
}

static const ui_led_data_t * uiLeds_GetDataForLed(uint16 led_index)
{
    const ui_led_data_t * return_data = NULL;
    if (led_index & CONTEXT_INDICATION_MASK)
    {
        uint16 context_led_flash_index = led_index & ~CONTEXT_INDICATION_MASK;
        PanicFalse(context_led_flash_index < the_leds.context_mapping_table_size);

        return_data = &UiIndicator_GetContextDataForIndex(
                    the_leds.context_to_led_data_mappings,
                    the_leds.context_mapping_table_size,
                    context_led_flash_index)->led;
    }
    else
    {
        PanicFalse(led_index < the_leds.event_mapping_table_size);

        return_data = &UiIndicator_GetDataForIndex(
                        the_leds.sys_event_to_led_data_mappings,
                        the_leds.event_mapping_table_size,
                        led_index)->led;
    }
    return return_data;
}

static void uiLeds_DoSetPattern(const ui_led_data_t *config, uint16 led_index)
{
    uint16 *client_lock = NULL;
    uint16 client_lock_mask = 0;

    DEBUG_LOG_V_VERBOSE("uiLeds_DoSetPattern priority=%d", config->priority);

    bool is_context_indication = !!(led_index & CONTEXT_INDICATION_MASK);
    if (!is_context_indication)
    {
        UiIndicator_ScheduleIndicationCompletedMessage(
                the_leds.sys_event_to_led_data_mappings,
                the_leds.event_mapping_table_size,
                led_index,
                UI_INTERNAL_LED_FLASH_COMPLETED,
                &the_leds.task,
                &the_leds.led_event_flash_ongoing_mask,
                &client_lock,
                &client_lock_mask);

        if (the_leds.sys_event_to_led_data_mappings[led_index].await_indication_completion)
        {
            /* If the configured LED Flash event needs to be indicated prior to a shutdown,
               ensure the LED manager is enabled in order to show the LED flash. (The flash
               may need to be indicated in order to complete a shutdown procedure). */
            LedManager_Enable(TRUE);
        }
    }

    LedManager_SetPattern(config->data.pattern, config->priority, client_lock, client_lock_mask);
}

static void uiLed_PerformLedAction(const ui_led_data_t *config, uint16 led_index)
{
    switch(config->action)
    {
    case LED_START_PATTERN:
        uiLeds_DoSetPattern(config, led_index);
        break;
    case LED_STOP_PATTERN:
        DEBUG_LOG_V_VERBOSE("uiLed_PerformLedAction LED_STOP_PATTERN priority=%d", config->priority);
        LedManager_StopPattern(config->priority);
        break;
    case LED_SET_FILTER:
        if (LedManager_GetFilter(config->priority) != config->data.filter)
        {
            DEBUG_LOG_V_VERBOSE("uiLed_PerformLedAction LED_SET_FILTER priority=%d", config->priority);
            LedManager_SetFilter(config->data.filter, config->priority);
        }
        break;
    case LED_CANCEL_FILTER:
        if (LedManager_GetFilter(config->priority) != NULL)
        {
            DEBUG_LOG_V_VERBOSE("uiLed_PerformLedAction LED_CANCEL_FILTER priority=%d", config->priority);
            LedManager_CancelFilter(config->priority);
        }
        break;
    default:
        break;
    }
}

static bool uiLeds_getMatchingContextIndex(ui_providers_t provider, unsigned context, uint16 *led_index)
{
    bool event_found = FALSE;
    event_found = UiIndicator_GetIndexFromContextMappingTable(
                the_leds.context_to_led_data_mappings,
                the_leds.context_mapping_table_size,
                provider,
                context,
                led_index);
    if (event_found)
    {
        *led_index |= CONTEXT_INDICATION_MASK;
    }
    return event_found;
}

static void uiLeds_DoFlashIndication(uint16 led_index)
{
    const ui_led_data_t *config = uiLeds_GetDataForLed(led_index);

    DEBUG_LOG_V_VERBOSE("uiLeds_DoFlashIndication index=%04x", led_index);

    if (!config->local_only)
    {
        Ui_RaiseUiEvent(ui_indication_type_led, led_index, 0);
    }

    uiLed_PerformLedAction(config, led_index);
}

static void uiLeds_DoContextFlashIndication(uint16 context_index, ui_providers_t provider)
{
    uiLeds_DoFlashIndication(context_index);

    the_leds.curr_indicated_context_index = context_index;
    the_leds.curr_indicated_provider = provider;
}

static bool uiLeds_CheckForSupercedingContext(uint16 led_index)
{
    /* Need to check the contexts in between the last indicated one and the one being
     * updated, in case there is something more aposite to indicate */
    bool was_superceded = FALSE;
    uint16 initial_index = the_leds.curr_indicated_context_index & ~CONTEXT_INDICATION_MASK;
    uint16 curr_context_index = initial_index + 1;
    uint16 index_to_indicate = led_index & ~CONTEXT_INDICATION_MASK;

    if (curr_context_index < the_leds.context_mapping_table_size)
    {
        DEBUG_LOG_V_VERBOSE("uiLeds_CheckForSupercedingContext initial_index=%02x, index_to_indicate=%02x",
                  initial_index, index_to_indicate);

        do
        {
            uint16 curr_provider = the_leds.context_to_led_data_mappings[curr_context_index].provider;
            uint16 curr_context = the_leds.context_to_led_data_mappings[curr_context_index].context;

            DEBUG_LOG_V_VERBOSE("uiLeds_CheckForSupercedingContext curr_context_index=%d, provider=%02x, context=%02x",
                      curr_context_index, curr_provider, curr_context);

            /* Check if an intervening table entry context matches, i.e. from another provider. */
            if (curr_provider != the_leds.curr_indicated_provider &&
                curr_context == Ui_GetUiProviderContext(curr_provider))
            {
                uiLeds_DoContextFlashIndication((curr_context_index | CONTEXT_INDICATION_MASK), curr_provider);
                was_superceded = TRUE;

                DEBUG_LOG("uiLeds_CheckForSupercedingContext was superceded by provider=%02x", curr_provider);
                break;
            }
            curr_context_index++;
        }
        while (curr_context_index < index_to_indicate);
    }

    return was_superceded;
}

static void uiLeds_DoContextUpdate(UI_PROVIDER_CONTEXT_UPDATED_T * msg)
{
    uint16 led_index = 0;

    DEBUG_LOG("uiLeds_DoContextUpdate Provider=%02x Context=%02x", msg->provider, msg->context);

    if (uiLeds_getMatchingContextIndex(msg->provider, msg->context, &led_index))
    {
        /* Conditions for update:
         *  1) The context has a higher priority level than that currently indicated. */
        if (led_index < the_leds.curr_indicated_context_index)
        {
            uiLeds_DoContextFlashIndication(led_index, msg->provider);
        }
        /* or 2) the currently indicated context is no longer valid (i.e. the same provider has updated the context) */
        else if (led_index > the_leds.curr_indicated_context_index &&
                 msg->provider == the_leds.curr_indicated_provider)
        {
            /* If a provider has changed to a lower priority context, it may have been superceded by a
               higher priority context from a different provider. */
            bool was_superceded = uiLeds_CheckForSupercedingContext(led_index);

            if (!was_superceded)
            {
                DEBUG_LOG_V_VERBOSE("uiLeds_DoContextUpdate not superceded.");
                uiLeds_DoContextFlashIndication(led_index, msg->provider);
            }
        }
    } else {
        /* If the configuration doesn't include an entry for the specified provider/context
           and we aren't already indicating a different provider context, the existing flash
           needs to be cancelled and the next highest priority selected. */
        if (the_leds.curr_indicated_provider == msg->provider)
        {
            if (!uiLeds_CheckForSupercedingContext(the_leds.context_mapping_table_size-1))
            {
                DEBUG_LOG_V_VERBOSE("uiLeds_DoContextUpdate no matching contexts");
                LedManager_StopPattern(LED_PRI_LOW);

                the_leds.curr_indicated_context_index = LED_NONE;
                the_leds.curr_indicated_provider = LED_NONE;
            }
        }
    }
}

static void uiLeds_HandleMessage(Task task, MessageId id, Message message)
{
    uint16 led_index = 0;

    UNUSED(task);

    if (id == UI_PROVIDER_CONTEXT_UPDATED)
    {
        uiLeds_DoContextUpdate((UI_PROVIDER_CONTEXT_UPDATED_T *)message);
    }
    if (uiLeds_GetLedIndexFromMappingTable(id, &led_index))
    {
        uiLeds_DoFlashIndication(led_index);
    }
    else if (id == UI_INTERNAL_LED_FLASH_COMPLETED)
    {
        DEBUG_LOG("UI_INTERNAL_LED_FLASH_COMPLETED indicate=%d", the_leds.indicate_when_power_shutdown_prepared);
        if (the_leds.indicate_when_power_shutdown_prepared)
        {
            appPowerShutdownPrepareResponse(&the_leds.task);
        }
    }
    else if (id == APP_POWER_SHUTDOWN_PREPARE_IND)
    {
        DEBUG_LOG("uiLeds_HandleMessage APP_POWER_SHUTDOWN_PREPARE_IND");
        the_leds.indicate_when_power_shutdown_prepared = TRUE;
    }
    else if (id == APP_POWER_SLEEP_PREPARE_IND)
    {
        appPowerSleepPrepareResponse(&the_leds.task);
    }
    else
    {
        // Ignore message
    }
}

Task UiLeds_GetUiLedsTask(void)
{
    return &the_leds.task;
}

void UiLeds_SetLedConfiguration(const ui_event_indicator_table_t *ui_event_table,
                                uint8 ui_event_table_size,
                                const ui_provider_context_consumer_indicator_table_t* ui_context_table,
                                uint8 ui_context_table_size)
{
    the_leds.sys_event_to_led_data_mappings = ui_event_table;
    the_leds.event_mapping_table_size = ui_event_table_size;

    UiIndicator_RegisterInterestInConfiguredSystemEvents(
                the_leds.sys_event_to_led_data_mappings,
                the_leds.event_mapping_table_size,
                &the_leds.task);

    the_leds.context_to_led_data_mappings = ui_context_table;
    the_leds.context_mapping_table_size = ui_context_table_size;

    UiIndicator_RegisterInterestInConfiguredProviderContexts(
                the_leds.context_to_led_data_mappings,
                the_leds.context_mapping_table_size,
                &the_leds.task);
}

/*! brief Initialise Ui Leds module */
bool UiLeds_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("UiLeds_Init");

    memset(&the_leds, 0, sizeof(ui_leds_task_data_t));

    the_leds.task.handler = uiLeds_HandleMessage;
    the_leds.curr_indicated_context_index = LED_NONE;
    the_leds.curr_indicated_provider = LED_NONE;

    return TRUE;
}

void UiLeds_NotifyUiIndication(uint16 led_index)
{
    const ui_led_data_t *config = NULL;

    config = uiLeds_GetDataForLed(led_index);

    uiLed_PerformLedAction(config, led_index);
}
