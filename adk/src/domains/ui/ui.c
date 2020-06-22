/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      This is ui module which provides mechanism to find and send mapped ui
            input from ui configuration table for any incoming logical input.

    These apis are used by application as well as different ui providers, ui input
    consumers as well as ui provider context consumers to register/unregister themselves
    with the ui module as well as handling incoming logical input.This module sends mapped
    ui input for the logical input to interested ui input consumers.
*/

#include "ui.h"
#include "ui_inputs.h"
#include "ui_prompts.h"
#include "ui_tones.h"
#include "ui_leds.h"
#include "adk_log.h"

#include <stdlib.h>
#include <stdio.h>
#include <logging.h>
#include <panic.h>
#include <task_list.h>
#include <message_broker.h>

#define NUM_OF_UI_INPUT_CONSUMERS                      5
#define ERROR_UI_PROVIDER_NOT_PRESENT                  0xFF

/*! Enumerate the UI message group, so the total number is known */
enum UI_MESSAGE_GROUPS_COUNT
{
    FOREACH_UI_INPUTS_MESSAGE_GROUP(GENERATE_ENUM)
    NUMBER_OF_UI_INPUTS_MESSAGE_GROUPS,
};

#ifndef DISABLE_LOG
const char * const UI_INPUT_STRING[] = {
    FOREACH_UI_INPUT(GENERATE_STRING)
};
#endif

/*! \brief UI task structure */
typedef struct
{
    /*! The UI task. */
    TaskData task;

} uiTaskData;

/*!< UI data structure */
uiTaskData  app_ui;

/*! \brief Ui provider struct*/
typedef struct{

    ui_providers_t ui_provider_id;
    ui_provider_context_callback_t ui_provider_context_callback;
}registered_ui_provider_t;

registered_ui_provider_t *registered_ui_providers = NULL;

/*! \brief Ui provider context consumer struct*/
typedef struct{

    Task consumer_task;
    ui_providers_t ui_provider_id;

}ui_provider_context_consumer_t;

ui_provider_context_consumer_t* ui_provider_context_consumers = NULL;

/* One task-list per ui input group */
static task_list_t ui_input_consumers_task_list[NUMBER_OF_UI_INPUTS_MESSAGE_GROUPS];

static uint8 num_of_ui_providers = 0;
static uint8 num_of_ctxt_consumers = 0;

static const ui_config_table_content_t* ui_config_table;
static unsigned ui_config_size = 0;

/*! The default UI interceptor function */
inject_ui_input inject_ui_input_funcptr = NULL;

/*! The default UI Event sniffer function */
sniff_ui_event sniff_ui_event_funcptr = NULL;


/******************************************************************************
 * Internal functions
 ******************************************************************************/
static uint8 getUiProviderIndexInRegisteredList(ui_providers_t ui_provider_id)
{
    for(uint8 index=0; index<num_of_ui_providers; index++)
    {
        if(registered_ui_providers[index].ui_provider_id == ui_provider_id)
            return index;
    }
    return ERROR_UI_PROVIDER_NOT_PRESENT;
}

static ui_input_t getUiInput(unsigned logical_input)
{
    uint8 ui_provider_index_in_list = 0;
    unsigned ui_provider_ctxt = 0;

    for(uint8 index = 0; index < ui_config_size; index++)
    {
        if(ui_config_table[index].logical_input == logical_input)
        {
            /* find index of the ui provider registered for the logical input in the list*/
            ui_provider_index_in_list = getUiProviderIndexInRegisteredList(ui_config_table[index].ui_provider_id );

            if(ui_provider_index_in_list != ERROR_UI_PROVIDER_NOT_PRESENT)
            {
                /* get current context of this ui provider*/
                ui_provider_ctxt = registered_ui_providers[ui_provider_index_in_list].ui_provider_context_callback();

                /* if context is same then return corresponding ui input*/
                if(ui_provider_ctxt == ui_config_table[index].ui_provider_context)
                    return ui_config_table[index].ui_input;

            }
        }
    }
    return ui_input_invalid;
}

#ifndef DISABLE_LOG
static inline bool ui_IsNonContiguousEnumSymbol(unsigned symbol_index)
{
    return strchr(UI_INPUT_STRING[symbol_index],'=') ? TRUE : FALSE;
}

static inline bool ui_IsMsgGroupValid(unsigned msg_group)
{
    return (MSG_GRP_TO_ID(msg_group) != UI_INPUTS_BOUNDS_CHECK_MESSAGE_BASE) ? TRUE : FALSE;
}

static inline bool ui_IsUiInputStringIndexValid(unsigned index)
{
    return (index < sizeof(UI_INPUT_STRING)/sizeof(const char * const)) ? TRUE : FALSE;
}

static const char * const ui_GetUiInputAsString(ui_input_t supplied_ui_input)
{
    unsigned index = 0;
    unsigned msg_id_in_curr_msg_group = 0;
    message_group_t local_msg_group = UI_INPUTS_TELEPHONY_MESSAGE_GROUP;
    const char * ret_val = "Bad UI_Input";

    do
    {
        unsigned generated_ui_input = MSG_GRP_TO_ID(local_msg_group) + msg_id_in_curr_msg_group;

        if (generated_ui_input == supplied_ui_input)
        {
            ret_val = UI_INPUT_STRING[index];
            break;
        }

        index++;
        if (!ui_IsUiInputStringIndexValid(index))
            break;

        if (ui_IsNonContiguousEnumSymbol(index))
        {
            local_msg_group++;
            msg_id_in_curr_msg_group = 0;
        }
        else
        {
            msg_id_in_curr_msg_group++;
        }
    }
    while (ui_IsMsgGroupValid(local_msg_group));

    return ret_val;
}
#else
#define ui_GetUiInputAsString(x) (void)(x)
#endif

/*! \brief Convert message group to 0-based index and send input to the indexed
    task list */
static void ui_SendUiInputToConsumerGroupTaskList(ui_input_t ui_input, uint32 delay)
{
    message_group_t group = ID_TO_MSG_GRP(ui_input);
    PanicFalse(group >= UI_INPUTS_MESSAGE_GROUP_START);
    group -= UI_INPUTS_MESSAGE_GROUP_START;
    PanicFalse(group < NUMBER_OF_UI_INPUTS_MESSAGE_GROUPS);

    TaskList_MessageSendLaterWithSize(&ui_input_consumers_task_list[group], ui_input, NULL, 0, delay);
}

static void handleLogicalInput(unsigned logical_input)
{
    ui_input_t ui_input = getUiInput(logical_input);
    uint32 delay = D_IMMEDIATE;

    if(ui_input != ui_input_invalid)
    {
        DEBUG_PRINT("UI %s\n", ui_GetUiInputAsString(ui_input));

        PanicNull((void*)inject_ui_input_funcptr);
        inject_ui_input_funcptr(ui_input, delay);
    }
}

static void uiHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    handleLogicalInput(id);
}

/******************************************************************************
 * External API functions
 ******************************************************************************/
Task Ui_GetUiTask(void)
{
    return &app_ui.task;
}

void Ui_RegisterUiProvider(ui_providers_t ui_provider, ui_provider_context_callback_t ui_provider_context_callback)
{
    size_t new_size;
    registered_ui_provider_t *new_provider;

    PanicFalse((!num_of_ui_providers && !registered_ui_providers) ||
               ( num_of_ui_providers &&  registered_ui_providers));

    new_size = sizeof(registered_ui_provider_t) * (num_of_ui_providers + 1);
    registered_ui_providers = PanicNull(realloc(registered_ui_providers, new_size));

    new_provider = registered_ui_providers + num_of_ui_providers;
    new_provider->ui_provider_context_callback = ui_provider_context_callback;
    new_provider->ui_provider_id = ui_provider;

    num_of_ui_providers++;
}

void Ui_UnregisterUiProviders(void)
{
    free(registered_ui_providers);
    registered_ui_providers = NULL;
    num_of_ui_providers = 0;
}

void Ui_RegisterUiInputConsumer(Task ui_input_consumer_task,
                                const message_group_t * msg_groups_of_interest,
                                unsigned num_msg_groups)
{
    MessageBroker_RegisterInterestInMsgGroups(ui_input_consumer_task, msg_groups_of_interest, num_msg_groups);
}

static bool ui_taskIsAlreadyRegisteredForProviderContextChanges(
        ui_providers_t provider,
        Task client_task)
{
    bool is_already_registered = FALSE;
    for (int i=0; i < num_of_ctxt_consumers; i++)
    {
        if (provider == ui_provider_context_consumers[i].ui_provider_id &&
            client_task == ui_provider_context_consumers[i].consumer_task)
        {
            is_already_registered = TRUE;
        }
    }
    return is_already_registered;
}

void Ui_RegisterContextConsumers(
        ui_providers_t ui_provider,
        Task ui_provider_ctxt_consumer_task)
{
    if (!ui_taskIsAlreadyRegisteredForProviderContextChanges(ui_provider, ui_provider_ctxt_consumer_task))
    {
        size_t new_size;
        ui_provider_context_consumer_t *new_consumer;

        PanicFalse((!num_of_ctxt_consumers && !ui_provider_context_consumers) ||
                   ( num_of_ctxt_consumers &&  ui_provider_context_consumers));

        new_size = sizeof(ui_provider_context_consumer_t) * (num_of_ctxt_consumers + 1);
        ui_provider_context_consumers = PanicNull(realloc(ui_provider_context_consumers, new_size));

        new_consumer = ui_provider_context_consumers + num_of_ctxt_consumers;
        new_consumer->consumer_task = ui_provider_ctxt_consumer_task;
        new_consumer->ui_provider_id = ui_provider;

        num_of_ctxt_consumers++;
    }
}

void Ui_UnregisterContextConsumers(void)
{
    free(ui_provider_context_consumers);
    ui_provider_context_consumers = NULL;
    num_of_ctxt_consumers = 0;
}


void Ui_InformContextChange(ui_providers_t ui_provider,unsigned latest_ctxt)
{
    for(uint8 index=0; index<num_of_ctxt_consumers; index++)
    {
        if (ui_provider_context_consumers[index].ui_provider_id == ui_provider &&
            ui_provider_context_consumers[index].consumer_task != NULL)
        {

            MESSAGE_MAKE(message,UI_PROVIDER_CONTEXT_UPDATED_T);
            message->provider = ui_provider;
            message->context = latest_ctxt;
            MessageSend(ui_provider_context_consumers[index].consumer_task,
                        UI_PROVIDER_CONTEXT_UPDATED,
                        message);
        }
    }
}

void Ui_InjectUiInput(ui_input_t ui_input)
{
    PanicNull((void*)inject_ui_input_funcptr);
    inject_ui_input_funcptr(ui_input, D_IMMEDIATE);
}

void Ui_InjectUiInputWithDelay(ui_input_t ui_input, uint32 delay)
{
    ui_SendUiInputToConsumerGroupTaskList(ui_input, delay);
}

/*! brief Initialise UI module */
bool Ui_Init(Task init_task)
{
    message_group_t group;
    uiTaskData *theUi = &app_ui;
    /* Set up task handler */
    theUi->task.handler = uiHandleMessage;

    /* Set up the default UI interceptor function */
    inject_ui_input_funcptr = ui_SendUiInputToConsumerGroupTaskList;
    sniff_ui_event_funcptr = NULL;

    for (group = 0; group < NUMBER_OF_UI_INPUTS_MESSAGE_GROUPS; group++)
    {
        TaskList_Initialise(&ui_input_consumers_task_list[group]);
    }

    UNUSED(init_task);
    return TRUE;
}

void Ui_SetConfigurationTable(const ui_config_table_content_t* config_table,
                              unsigned config_size)
{
    ui_config_table = config_table;
    ui_config_size = config_size;
}

void Ui_RegisterUiInputsMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group >= UI_INPUTS_MESSAGE_GROUP_START);
    group -= UI_INPUTS_MESSAGE_GROUP_START;
    PanicFalse(group < NUMBER_OF_UI_INPUTS_MESSAGE_GROUPS);

    TaskList_AddTask(&ui_input_consumers_task_list[group], task);
}

void Ui_RegisterUiEventSniffer(sniff_ui_event ui_sniff_func)
{
    sniff_ui_event_funcptr = ui_sniff_func;
}

/*! brief Function to register the new UI interceptor function */
inject_ui_input Ui_RegisterUiInputsInterceptor(inject_ui_input ui_intercept_func)
{
    inject_ui_input old_ui_interceptor;

    /* Return NULL for invalid interceptor function */
    if(ui_intercept_func == NULL)
        return NULL;

    /* Preserve the active UI interceptor function */
    old_ui_interceptor = inject_ui_input_funcptr;

    /* Update the active UI interceptor function pointer with the supplied function */
    inject_ui_input_funcptr = ui_intercept_func;

    /* Return the old UI interceptor function */
    return old_ui_interceptor;
}

rtime_t Ui_RaiseUiEvent(ui_indication_type_t type, uint16 indication_index, rtime_t time_to_play)
{
    rtime_t inital_ttp = time_to_play;

    if (sniff_ui_event_funcptr != NULL)
    {
        time_to_play = sniff_ui_event_funcptr(type, indication_index, time_to_play);
    }

    DEBUG_LOG("Ui_RaiseUiEvent type=%d index=%d intial=%d final ttp=%d",
              type, indication_index, inital_ttp, time_to_play );

    return time_to_play;
}

void Ui_NotifyUiEvent(ui_indication_type_t ind_type, uint16 ind_index, uint32 timestamp)
{
    switch(ind_type)
    {
    case ui_indication_type_audio_prompt:
        {
            DEBUG_LOG("Ui_NotifyUiEvent send prompt_index %d at %d us", ind_index, timestamp);
            UiPrompts_NotifyUiIndication(ind_index, timestamp);
        }
        break;
    case ui_indication_type_audio_tone:
        {
            DEBUG_LOG("Ui_NotifyUiEvent send tone_index %d at %d us", ind_index, timestamp);
            UiTones_NotifyUiIndication(ind_index, timestamp);
        }
        break;
    case ui_indication_type_led:
        {
            DEBUG_LOG("Ui_NotifyUiEvent send led_index %d", ind_index);
            UiLeds_NotifyUiIndication(ind_index);
        }
        break;
    default:
        /* Can be extended in the future for other indication types */
        Panic();
        break;
    }
}

unsigned Ui_GetUiProviderContext(ui_providers_t ui_provider)
{
    uint8 provider_index = getUiProviderIndexInRegisteredList(ui_provider);

    PanicFalse(registered_ui_providers!=NULL);

    return registered_ui_providers[provider_index].ui_provider_context_callback();
}

#define DECLARE_MESSAGE_GROUP_REGISTRATION(group_name) \
MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(group_name, Ui_RegisterUiInputsMessageGroup, NULL);

FOREACH_UI_INPUTS_MESSAGE_GROUP(DECLARE_MESSAGE_GROUP_REGISTRATION)
