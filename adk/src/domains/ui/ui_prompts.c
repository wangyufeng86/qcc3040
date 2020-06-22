/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file converts System Events to corresponding Audio Prompt UI Events
            by table look-up, using a configuration table passed in by the Application.
            It then plays these Prompts when required using the Kymera audio framework
            Aux path.
*/

#include "ui_prompts.h"

#include "ui_inputs.h"
#include "pairing.h"
#include "ui.h"
#include "av.h"
#include <power_manager.h>

#include <domain_message.h>
#include <logging.h>
#include <panic.h>

#include <stdlib.h>

#include "system_clock.h"

#define DEFAULT_NO_REPEAT_DELAY         D_SEC(5)

ui_prompts_task_data_t the_prompts;

#define PROMPT_NONE                     0xFFFF

#define UI_PROMPTS_WAIT_FOR_PROMPT_COMPLETION 0x1

/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when a prompt is played. Until this message is delivered
        repeat prompts will not be played */
    UI_INTERNAL_CLEAR_LAST_PROMPT,
    UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED
};

static bool uiPrompts_GetPromptIndexFromMappingTable(MessageId id, uint16 * prompt_index)
{
    return UiIndicator_GetIndexFromMappingTable(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                id,
                prompt_index);
}

static const ui_prompt_data_t * uiPrompts_GetDataForPrompt(uint16 prompt_index)
{
    return &UiIndicator_GetDataForIndex(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                prompt_index)->prompt;
}

inline static bool uiPrompt_isNotARepeatPlay(uint16 prompt_index)
{
    return prompt_index != the_prompts.last_prompt_played_index;
}

/*! \brief Play prompt.

    \param prompt_index The prompt to play from the mappings table.
    \param time_to_play The microsecond at which to begin mixing of this audio prompt.
    \param config The prompt configuration data for the prompt to play.
*/
static void uiPrompts_PlayPrompt(uint16 prompt_index, rtime_t time_to_play, const ui_prompt_data_t *config)
{
    DEBUG_LOG("uiPrompts_PlayPrompt index=%d ttp=%d enabled=%d",
              prompt_index, time_to_play, the_prompts.prompt_playback_enabled );

    if (the_prompts.prompt_playback_enabled)
    {
        uint16 *client_lock = NULL;
        uint16 client_lock_mask = 0;

        UiIndicator_ScheduleIndicationCompletedMessage(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                prompt_index,
                UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED,
                &the_prompts.task,
                &the_prompts.prompt_playback_ongoing_mask,
                &client_lock,
                &client_lock_mask);

        FILE_INDEX *index = &the_prompts.prompt_file_indexes[prompt_index];

        if (*index == FILE_NONE)
        {
            const char* name = config->filename;
            *index = FileFind(FILE_ROOT, name, strlen(name));
            /* Prompt not found */
            PanicFalse(*index != FILE_NONE);
        }

        DEBUG_LOG("uiPrompts_PlayPrompt FILE_INDEX=%08x format=%d rate=%d", *index , config->format, config->rate );

        appKymeraPromptPlay(*index, config->format, config->rate, time_to_play,
                            config->interruptible, client_lock, client_lock_mask);

        if(the_prompts.no_repeat_period_in_ms != 0)
        {
            MessageCancelFirst(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT);
            MessageSendLater(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT, NULL,
                             the_prompts.no_repeat_period_in_ms);
            the_prompts.last_prompt_played_index = prompt_index;
        }
    }
}

static void uiPrompts_SchedulePromptPlay(uint16 prompt_index)
{
    const ui_prompt_data_t *config = uiPrompts_GetDataForPrompt(prompt_index);

    if (uiPrompt_isNotARepeatPlay(prompt_index) &&
        (config->queueable || (!appKymeraIsTonePlaying() && (the_prompts.prompt_playback_ongoing_mask == 0))))
    {
        /* Factor in the propagation latency through the various buffers for the aux channel and the time to start the file source */
        rtime_t time_now = SystemClockGetTimerTime();
        rtime_t time_to_play = rtime_add(time_now, UI_INDICATOR_DELAY_FOR_SYNCHRONISED_TTP_IN_MICROSECONDS);

        time_to_play = Ui_RaiseUiEvent(ui_indication_type_audio_prompt, prompt_index, time_to_play);

        uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
    }
}

static void uiPrompts_HandleMessage(Task task, MessageId id, Message message)
{
    uint16 prompt_index = 0;

    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("uiPrompts_HandleMessage Id=%04x", id);

    bool prompt_found = uiPrompts_GetPromptIndexFromMappingTable(id, &prompt_index);
    if (prompt_found)
    {
        if (the_prompts.generate_ui_events)
        {
            uiPrompts_SchedulePromptPlay(prompt_index);
        }
        else
        {
            /* Prompts that need to be indicated before shutdown should always be played,
               regardless of whether we are rendering indications based on the current
               device topology role and any other gating factors. */
            ui_event_indicator_table_t prompt_to_play = the_prompts.sys_event_to_prompt_data_mappings[prompt_index];
            bool indicate_on_shutdown = prompt_to_play.await_indication_completion;
            if (indicate_on_shutdown)
            {
                uiPrompts_SchedulePromptPlay(prompt_index);
            }
        }
    }
    else if (id == UI_INTERNAL_CLEAR_LAST_PROMPT)
    {
        DEBUG_LOG("UI_INTERNAL_CLEAR_LAST_PROMPT");
        the_prompts.last_prompt_played_index = PROMPT_NONE;
    }
    else if (id == UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED)
    {
        DEBUG_LOG("UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED indicate=%d", the_prompts.indicate_when_power_shutdown_prepared);
        if (the_prompts.indicate_when_power_shutdown_prepared)
        {
            appPowerShutdownPrepareResponse(&the_prompts.task);
        }
    }
    else if (id == APP_POWER_SHUTDOWN_PREPARE_IND)
    {
        the_prompts.indicate_when_power_shutdown_prepared = TRUE;
    }
    else if (id == APP_POWER_SLEEP_PREPARE_IND)
    {
        appPowerSleepPrepareResponse(&the_prompts.task);
    }
    else
    {
        // Ignore message
    }
}

/*! \brief brief Set/reset play_prompt flag. This is flag is used to check if prompts
  can be played or not. Application will set and reset the flag. Scenarios like earbud
  is in ear or not and etc.

    \param play_prompt If TRUE, prompt can be played, if FALSE, the prompt can not be
    played.
*/
void UiPrompts_SetPromptPlaybackEnabled(bool play_prompt)
{
    the_prompts.prompt_playback_enabled = play_prompt;
}

Task UiPrompts_GetUiPromptsTask(void)
{
    return &the_prompts.task;
}

void UiPrompts_SetPromptConfiguration(const ui_event_indicator_table_t *table, uint8 size)
{
    the_prompts.sys_event_to_prompt_data_mappings = table;
    the_prompts.mapping_table_size = size;

    the_prompts.prompt_file_indexes = PanicUnlessMalloc(size*sizeof(FILE_INDEX));
    memset(the_prompts.prompt_file_indexes, FILE_NONE, size*sizeof(FILE_INDEX));

    UiIndicator_RegisterInterestInConfiguredSystemEvents(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                &the_prompts.task);
}

void UiPrompts_SetNoRepeatPeriod(const Delay no_repeat_period_in_ms)
{
    the_prompts.no_repeat_period_in_ms = no_repeat_period_in_ms;
}

void UiPrompts_NotifyUiIndication(uint16 prompt_index, rtime_t time_to_play)
{
    const ui_prompt_data_t *config = uiPrompts_GetDataForPrompt(prompt_index);
    uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
}

/*! brief Initialise Ui prompts module */
bool UiPrompts_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("UiPrompts_Init");

    memset(&the_prompts, 0, sizeof(ui_prompts_task_data_t));

    the_prompts.last_prompt_played_index = PROMPT_NONE;
    the_prompts.task.handler = uiPrompts_HandleMessage;
    the_prompts.no_repeat_period_in_ms = DEFAULT_NO_REPEAT_DELAY;
    the_prompts.generate_ui_events = TRUE;
    the_prompts.prompt_playback_enabled = TRUE;

    return TRUE;
}


/*! brief de-initialise Ui prompts module */
bool UiPrompts_DeInit(void)
{
    DEBUG_LOG("UiPrompts_DeInit");

    the_prompts.sys_event_to_prompt_data_mappings = NULL;
    the_prompts.mapping_table_size = 0;

    free(the_prompts.prompt_file_indexes);
    the_prompts.prompt_file_indexes = NULL;

    return TRUE;
}

void UiPrompts_GenerateUiEvents(bool generate)
{
    the_prompts.generate_ui_events = generate;
}
