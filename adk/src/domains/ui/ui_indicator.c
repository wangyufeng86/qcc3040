/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Abstracts a standard implementation for accessing configuration data for a UI
            Indicator, which the UI module has received from the Application.
*/
#include "ui_indicator.h"
#include "ui.h"

#include <logging.h>
#include <domain_message.h>

bool UiIndicator_GetIndexFromMappingTable(const ui_event_indicator_table_t *mapping_table, uint16 mapping_table_size, MessageId id, uint16 * index)
{
    bool found = FALSE;
    for(*index = 0 ; *index < mapping_table_size; (*index)++)
    {
        if(id == mapping_table[*index].sys_event)
        {
            found = TRUE;
            break;
        }
    }
    return found;
}

const ui_ind_data_t* UiIndicator_GetDataForIndex(const ui_event_indicator_table_t *mapping_table, uint16 mapping_table_size, uint16 index)
{
    const ui_ind_data_t* data = NULL;

    PanicFalse(index < mapping_table_size);

    data = &mapping_table[index].data;

    PanicFalse( data != NULL );

    return data;
}

void UiIndicator_ScheduleIndicationCompletedMessage(
        const ui_event_indicator_table_t *mapping_table,
        uint16 mapping_table_size,
        uint16 index,
        MessageId id,
        Task task,
        uint16 *indication_ongoing_mask,
        uint16 **client_lock,
        uint16 *client_lock_mask)
{
    PanicFalse(index < mapping_table_size);

    if (mapping_table[index].await_indication_completion)
    {
        *indication_ongoing_mask |= UI_INDICATOR_WAIT_FOR_INDICATION_COMPLETION;
        MessageSendConditionally(task, id, NULL, indication_ongoing_mask);
        *client_lock = indication_ongoing_mask;
        *client_lock_mask = UI_INDICATOR_WAIT_FOR_INDICATION_COMPLETION;
    }
}

void UiIndicator_RegisterInterestInConfiguredSystemEvents(const ui_event_indicator_table_t *mapping_table, uint16 mapping_table_size, Task client_task)
{
    for(uint8 index = 0 ; index < mapping_table_size; index++)
    {
        MessageId sys_event = mapping_table[index].sys_event;
        message_group_t group = ID_TO_MSG_GRP(sys_event);

        MessageBroker_RegisterInterestInMsgGroups( client_task, &group, 1);
    }
}

bool UiIndicator_GetStartIndexFromReminderMappingTable(const ui_repeating_indication_table_t *mapping_table, uint16 mapping_table_size, MessageId id, uint16 * index)
{
    bool found = FALSE;
    for(*index = 0 ; *index < mapping_table_size; (*index)++)
    {
        if(id == mapping_table[*index].triggering_sys_event)
        {
            DEBUG_LOG("UiIndicator_GetStartIndexFromReminderMappingTable matched Id=%04x Index=%d", id, *index);
            found = TRUE;
            break;
        }
    }
    return found;
}

bool UiIndicator_GetCancelIndexFromReminderMappingTable(const ui_repeating_indication_table_t *mapping_table, uint16 mapping_table_size, MessageId id, uint16 * index)
{
    bool found = FALSE;
    for(*index = 0 ; *index < mapping_table_size; (*index)++)
    {
        if(id == mapping_table[*index].cancelling_sys_event)
        {
            DEBUG_LOG("UiIndicator_GetCancelIndexFromReminderMappingTable matched Id=%04x", id);
            found = TRUE;
            break;
        }
    }
    return found;
}

const ui_ind_data_t* UiIndicator_GetDataForReminderIndex(const ui_repeating_indication_table_t *mapping_table, uint16 mapping_table_size, uint16 index)
{
    const ui_ind_data_t* data = NULL;

    PanicFalse(index < mapping_table_size);

    data = &mapping_table[index].data;

    PanicFalse( data != NULL );

    return data;
}

void UiIndicator_RegisterInterestInConfiguredReminderSystemEvents(const ui_repeating_indication_table_t *mapping_table, uint16 mapping_table_size, Task client_task)
{
    for(uint8 index = 0 ; index < mapping_table_size; index++)
    {
        MessageId sys_event = mapping_table[index].triggering_sys_event;
        message_group_t group = ID_TO_MSG_GRP(sys_event);
        MessageBroker_RegisterInterestInMsgGroups(client_task, &group, 1);

        sys_event = mapping_table[index].cancelling_sys_event;
        group = ID_TO_MSG_GRP(sys_event);
        MessageBroker_RegisterInterestInMsgGroups(client_task, &group, 1);
    }
}

void UiIndicator_RegisterInterestInConfiguredProviderContexts(const ui_provider_context_consumer_indicator_table_t *mapping_table, uint16 mapping_table_size, Task client_task)
{
    for(uint8 index = 0 ; index < mapping_table_size; index++)
    {
        ui_providers_t provider = mapping_table[index].provider;

        Ui_RegisterContextConsumers(provider, client_task);
    }
}

bool UiIndicator_GetIndexFromContextMappingTable(const ui_provider_context_consumer_indicator_table_t *mapping_table, uint16 mapping_table_size, ui_providers_t provider, unsigned context, uint16 * index)
{
    bool found = FALSE;
    for(*index = 0 ; *index < mapping_table_size; (*index)++)
    {
        if (mapping_table[*index].provider == provider &&
            mapping_table[*index].context == context)
        {
            found = TRUE;
            break;
        }
    }
    return found;
}

const ui_ind_data_t* UiIndicator_GetContextDataForIndex(const ui_provider_context_consumer_indicator_table_t *mapping_table, uint16 mapping_table_size, uint16 index)
{
    const ui_ind_data_t* data = NULL;

    PanicFalse(index < mapping_table_size);

    data = &mapping_table[index].data;

    PanicFalse( data != NULL );

    return data;
}
