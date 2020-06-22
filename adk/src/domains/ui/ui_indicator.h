/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Abstracts a standard implementation for accessing configuration data for a UI
            Indicator, which the UI module has received from the Application.
*/
#ifndef UI_INDICATOR_H
#define UI_INDICATOR_H

#include <csrtypes.h>
#include <message.h>

#include "ui_leds_types.h"
#include "ui_tones_types.h"
#include "ui_prompts_types.h"
#include "ui_inputs.h"

#define UI_INDICATOR_WAIT_FOR_INDICATION_COMPLETION 0x1

// Includes times to message the operator, set up buffers and to apply volume fade in
#define AUDIO_SS_MSG_TO_OPERATOR                    (5*US_PER_MS)
#define AUDIO_SS_DATA_TO_BUFFERS                    (10*US_PER_MS)
#define AUDIO_SS_OPERATOR_GETS_SAMPLES_ON_SUBGRAPH  (10*US_PER_MS)
#define AUDIO_SS_VOLUME_RAMP_MAIN_TO_MIX_AUX        (10*US_PER_MS)
#define AUDIO_SS_DOWNSTREAM_LATENCY                 (30*US_PER_MS)

#define UI_SYNC_IND_AUDIO_SS_FIXED_DELAY \
                        (AUDIO_SS_MSG_TO_OPERATOR + \
                         AUDIO_SS_DATA_TO_BUFFERS + \
                         AUDIO_SS_OPERATOR_GETS_SAMPLES_ON_SUBGRAPH + \
                         AUDIO_SS_VOLUME_RAMP_MAIN_TO_MIX_AUX + \
                         AUDIO_SS_DOWNSTREAM_LATENCY)

// Includes time lost to message loop overhead and Kymera worst case chain set up time
#define UI_SYNC_IND_AUDIO_SS_CHAIN_CREATION_DELAY   (230*US_PER_MS)

// Total, non peer link, delay required for synchronised audio indications
#define UI_INDICATOR_DELAY_FOR_SYNCHRONISED_TTP_IN_MICROSECONDS \
                        (UI_SYNC_IND_AUDIO_SS_CHAIN_CREATION_DELAY +\
                         UI_SYNC_IND_AUDIO_SS_FIXED_DELAY)

typedef union
{
    ui_tone_data_t tone;
    ui_prompt_data_t prompt;
    ui_led_data_t led;

} ui_ind_data_t;

typedef struct
{
    /*! System Event that should cause a UI Event to be indicated */
    MessageId sys_event;

    /*! Data about the UI Indication associated with the System Event */
    ui_ind_data_t data;

    /*!< Determines whether the UI Indicator should receive notification
         once the indication has completed. */
    bool await_indication_completion;

} ui_event_indicator_table_t;

typedef struct
{
    /*! The UI Provider that shall provide the context */
    ui_providers_t provider;

    /*! The UI Provider Context on which to indicate */
    unsigned context;

    /*! Data about the UI Indication associated with the UI Provider Context */
    ui_ind_data_t data;

} ui_provider_context_consumer_indicator_table_t;

typedef struct {
    /*! System Event that should cause a UI Reminder Indication to be triggered */
    MessageId triggering_sys_event;

    /*! The period of time after the triggering system event at which the reminder shall be indicated, in seconds */
    unsigned reminder_period;

    /*! System Event that should cause a UI Reminder Indication to be cancelled */
    MessageId cancelling_sys_event;

    /*! Data about the UI Indication associated with the triggering System Event */
    ui_ind_data_t data;

} ui_repeating_indication_table_t;

/*! \brief Get the index in the table, if present, for the specified System Event.

    \param mapping_table The configuration table mapping System Events to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param id The System Event to lookup in the configuration table.
    \param index a pointer to the index of the UI Indication in the configuration table
    \return Boolean indicating whether the System Event was present in the configuration table
*/
bool UiIndicator_GetIndexFromMappingTable(
        const ui_event_indicator_table_t * mapping_table,
        uint16 mapping_table_size,
        MessageId id,
        uint16 * index);

/*! \brief Get the properties of a specific UI Indication from the configuration table.

    \param mapping_table The configuration table mapping System Events to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param index The index of the UI Indication in the configuration table
    \return a pointer to the configuration data for the UI Indication specified
*/
const ui_ind_data_t* UiIndicator_GetDataForIndex(
        const ui_event_indicator_table_t * mapping_table,
        uint16 mapping_table_size,
        uint16 index);

/*! \brief If required, schedule a message to be delivered to the UI Indicator once the indication is completed.

    \param mapping_table The configuration table mapping System Events to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param index The index of the UI Indication in the configuration table
*/
void UiIndicator_ScheduleIndicationCompletedMessage(
        const ui_event_indicator_table_t *mapping_table,
        uint16 mapping_table_size,
        uint16 index,
        MessageId id,
        Task task,
        uint16 *indication_ongoing_mask,
        uint16 **client_lock,
        uint16 *client_lock_mask);

/*! \brief For all the System Events in the specified configuration table, register
           the client task on the originating module's Task List.

    \param mapping_table The configuration table mapping System Events to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param client_task The task to insert on the module originating the System Events task_list
*/
void UiIndicator_RegisterInterestInConfiguredSystemEvents(
        const ui_event_indicator_table_t * mapping_table,
        uint16 mapping_table_size,
        Task client_task);

/*! \brief Get the index in the UI Reminder table, if present, for the specified triggering System Event.

    \param mapping_table The configuration table mapping System Events to UI Reminder Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Reminder Indications)
    \param id The System Event to lookup in the configuration table triggering Events column.
    \param index a pointer to the index of the UI Reminder Indication in the configuration table
    \return Boolean indicating whether the triggering System Event was present in the configuration table
*/
bool UiIndicator_GetStartIndexFromReminderMappingTable(
        const ui_repeating_indication_table_t *mapping_table,
        uint16 mapping_table_size,
        MessageId id,
        uint16 * index);

/*! \brief Get the index in the UI Reminder table, if present, for the specified cancelling System Event.

    \param mapping_table The configuration table mapping System Events to UI Reminder Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Reminder Indications)
    \param id The System Event to lookup in the configuration table cancelling Events column.
    \param index a pointer to the index of the UI Reminder Indication in the configuration table
    \return Boolean indicating whether the cancelling System Event was present in the configuration table
*/
bool UiIndicator_GetCancelIndexFromReminderMappingTable(
        const ui_repeating_indication_table_t *mapping_table,
        uint16 mapping_table_size,
        MessageId id,
        uint16 * index);

/*! \brief Get the properties of a specific UI Reminder Indication from the configuration table.

    \param mapping_table The configuration table mapping System Events to UI Reminder Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Reminder Indications)
    \param index The index of the UI Reminder Indication in the configuration table
    \return a pointer to the configuration data for the UI Reminder Indication specified
*/
const ui_ind_data_t* UiIndicator_GetDataForReminderIndex(
        const ui_repeating_indication_table_t *mapping_table,
        uint16 mapping_table_size,
        uint16 index);

/*! \brief For all the System Events in the specified configuration table, register
           the client task on the originating module's Task List.

    \param mapping_table The configuration table mapping System Events to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param client_task The task to insert on the module originating the System Events task_list
*/
void UiIndicator_RegisterInterestInConfiguredReminderSystemEvents(
        const ui_repeating_indication_table_t * mapping_table,
        uint16 mapping_table_size,
        Task client_task);

/*! \brief For all the UI Proivder Contexts in the specified configuration table, register
           the client task with the UI module using the UI Provider Context Consumer interface.

    \param mapping_table The configuration table mapping Provider contexts to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param client_task The task to register wit hthe UI module to recieve context changed updates
*/
void UiIndicator_RegisterInterestInConfiguredProviderContexts(
        const ui_provider_context_consumer_indicator_table_t *mapping_table,
        uint16 mapping_table_size,
        Task client_task);

/*! \brief Get the index in the table, if present, for the specified Provider Context.

    \param mapping_table The configuration table mapping UI Provider Contexts to UI Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Indications)
    \param provider The UI Provider to lookup in the configuration table.
    \param context The specific UI Provider Context to lookup in the configuration table.
    \param index a pointer to the index of the UI Indication in the configuration table
    \return Boolean indicating whether the UI Provider Context was present in the configuration table
*/
bool UiIndicator_GetIndexFromContextMappingTable(
        const ui_provider_context_consumer_indicator_table_t *mapping_table,
        uint16 mapping_table_size,
        ui_providers_t provider,
        unsigned context,
        uint16 * index);

/*! \brief Get the properties of a specific UI Provider Context Indication from the configuration table.

    \param mapping_table The configuration table mapping UI Provider Contexts to UI Reminder Indication data
    \param mapping_table_size The length of the configuration table (i.e. the number of configured UI Provider Context Indications)
    \param index The index of the UI Provider Context Indication in the configuration table
    \return a pointer to the configuration data for the UI Provider Context Indication specified
*/
const ui_ind_data_t* UiIndicator_GetContextDataForIndex(
        const ui_provider_context_consumer_indicator_table_t *mapping_table,
        uint16 mapping_table_size,
        uint16 index);


#endif // UI_INDICATOR_H
